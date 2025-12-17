#include "AyonCppApi.h"
#include <sys/types.h>
#include "httplib.h"
#include "nlohmann/json.hpp"
#include "nlohmann/json_fwd.hpp"
#include "devMacros.h"
#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <fstream>
#include <functional>
#include <future>
#include <memory>
#include <mutex>
#include <numeric>
#include <optional>
#include <regex>
#include <stdexcept>
#include <string>
#include <filesystem>
#include <string_view>
#include <thread>
#include <unordered_map>
#include <utility>
#include <vector>

#include <cstdlib>
#include <dlfcn.h> 
#include <filesystem>
#include "backward.hpp"
#include "perfPrinter.h"

// TODO implement the better Crash hanlder
backward::StackTrace st;

// ------------------------------------------------
// helper functions for getting the ca cert path
// ------------------------------------------------
std::string parseOutput(std::string& output) {
    std::string::size_type start = output.find('"');
    std::string::size_type end = output.find('"', start + 1);
    if (start != std::string::npos && end != std::string::npos) {
        return output.substr(start + 1, end - start - 1);
    } else {
        throw std::runtime_error("Failed to parse OpenSSL directory from command output.");
    }
}

std::string getOpenSSLDirByCLI() {
    std::array<char, 128> buffer;
    std::string result;
    auto pipeDeleter = [](FILE* pipe) { 
    #ifdef _WIN32
        _pclose(pipe);
    #else
        pclose(pipe); 
    #endif
    };
    std::unique_ptr<FILE, decltype(pipeDeleter)> pipe(
    #ifdef _WIN32
        _popen("openssl version -d", "r"),
    #else
        popen("openssl version -d", "r"),
    #endif
        pipeDeleter
    );
    if (!pipe) {
        throw std::runtime_error("popen() failed!");
    }
    while (fgets(buffer.data(), static_cast<int>(buffer.size()), pipe.get()) != nullptr) {
        result += buffer.data();
    }
    return parseOutput(result);
}

std::string getOpenSSLDir() {
#if OPENSSL_VERSION_NUMBER >= 0x10100000L
    const char* sslVersion = OpenSSL_version(OPENSSL_DIR);
    std::string sslVersionStr(sslVersion);
    return parseOutput(sslVersionStr);
#else
    return parseOutput(SSLeay_version(SSLEAY_DIR));
#endif
}
// ------------------------------------------------

AyonApi::AyonApi(const std::optional<std::string> &logFilePos,
                 const std::string &authKey,
                 const std::string &serverUrl,
                 const std::string &ayonProjectName,
                 const std::string &siteId,
                 std::optional<int> concurrency)
    : m_num_threads(concurrency.value_or(std::max(int(std::thread::hardware_concurrency() / 2), 1))),
      m_authKey(authKey),
      m_serverUrl(serverUrl),
      m_ayonProjectName(ayonProjectName),
      m_siteId(siteId) {
    PerfTimer("AyonApi::AyonApi");

    // ----------- Resolve Log Path
    std::filesystem::path logPath;
    if (logFilePos.has_value()) {
        try {
            std::filesystem::path inPath(logFilePos.value());
            std::cout << "Input log path: " << inPath << std::endl;

            if (inPath.is_relative()) {
                logPath = std::filesystem::weakly_canonical(inPath);
            } else {
                logPath = inPath;
            }

            if (!inPath.has_parent_path()) {
                logPath = std::filesystem::temp_directory_path() / inPath;
            }
            
            // Validate / Create directories
            if (std::filesystem::exists(logPath)) {
                logPath = std::filesystem::canonical(logPath);
            } else {
                if (logPath.has_parent_path()) {
                    std::filesystem::create_directories(logPath.parent_path());
                }
            }
        } 
        catch (const std::exception& e) {
            std::cerr << "[AyonApi] Path error: " << e.what() << std::endl;
        }
    }

    // ----------- Init m_Logger (Singleton Logic)
    std::cout << "[AyonApi] Retrieving AyonLogger Singleton..." << std::endl;
    
    AyonLogger& loggerRef = AyonLogger::getInstance();
    
    if (!logPath.empty()) {
        loggerRef.initFileLogger(logPath.string());
    }

    m_Log = std::shared_ptr<AyonLogger>(&loggerRef, [](AyonLogger*){});
    
    m_Log->registerLoggingKey("AyonApi");

    m_Log->LogLevelInfo();
    m_Log->info(m_Log->key("AyonApi"), "Init AyonServer httplib::Client");
    
    m_AyonServer = std::make_unique<httplib::Client>(m_serverUrl);
    m_Log->info(m_Log->key("AyonApi"), "After creating httplib::Client - {}", m_serverUrl);

    if (isSSL()) {
        std::string ayonSSLPath = std::getenv("AYON_SSL_CERT_PATH") ? std::getenv("AYON_SSL_CERT_PATH") : "";
        if (!ayonSSLPath.empty()) {
            m_Log->info(m_Log->key("AyonApi"), "Using AYON_SSL_CERT_PATH: {}", ayonSSLPath);
            m_AyonServer->set_ca_cert_path(ayonSSLPath.c_str());
        } else {
            m_Log->warn(m_Log->key("AyonApi"), "No AYON_SSL_CERT_PATH set, trying to get OpenSSL dir");
            try {
                setSSL();
            } catch (const std::exception &e) {
                m_Log->error("Failed to get OpenSSL directory: {}", e.what());
                m_AyonServer->set_ca_cert_path(nullptr); 
            }
        }
        m_AyonServer->enable_server_certificate_verification(true);
    }

    if (!m_AyonServer) {
        m_Log->error("m_AyonServer is null. serverUrl='{}'", m_serverUrl);
        throw std::runtime_error("AyonApi: HTTP client not initialized");
    }

    httplib::Result res;
    try {
        res = m_AyonServer->Get("/api/info");
    } catch (const std::exception& e) {
        m_Log->error("Exception during GET /api/info: {}", e.what());
        throw;
    }

    if (!res) {
        m_Log->error("Failed to connect to the Ayon server.");
    } else {
        m_Log->info(m_Log->key("AyonApi"), "Ayon server info: {}", res->body);
        m_Log->info(m_Log->key("AyonApi"), "Status code: {}", res->status);

        m_headers = {
            {"X-Api-Key", m_authKey},
        };
        auto resMe = m_AyonServer->Get("/api/users/me", m_headers);
        if (resMe && resMe->status != 200) {
            m_headers = {};
            m_AyonServer->set_bearer_token_auth(m_authKey);
        }
    }

    m_Log->info(m_Log->key("AyonApi"), "Constructor Getting Site Roots");
    getSiteRoots();
}

AyonApi::~AyonApi() {
    m_Log->info(m_Log->key("AyonApi"), "AyonApi::~AyonApi()");
};

std::unordered_map<std::string, std::string>*
AyonApi::getSiteRoots() {
    m_Log->info(m_Log->key("AyonApi"), "AyonApi::getSiteRoots()");
        if (m_siteRoots.size() < 1) {
            std::string platform;
            #ifdef _WIN32
                platform = "windows";
            #elif __linux__
                platform = "linux";
            #endif
            nlohmann::json response = GET(std::make_shared<std::string>("/api/projects/" + m_ayonProjectName + "/siteRoots?platform=" + platform),
                  std::make_shared<httplib::Headers>(m_headers), 200);
        
        if (response.empty()) {
            m_Log->error("AyonApi::getSiteRoots response is empty");
            return &m_siteRoots;
        } else {
            m_siteRoots = response;
        }
        
    }
    if (m_Log->isKeyActive(m_Log->key("AyonApi"))) {
        m_Log->info(m_Log->key("AyonApi"), "found site Roots: ");
        for (auto &e: m_siteRoots) {
            m_Log->info("{}, {}", e.first, e.second);
        }
    }

    return &m_siteRoots;
};

std::string
AyonApi::rootReplace(const std::string &rootLessPath) {
    m_Log->info(m_Log->key("AyonApi"), "AyonApi::rootReplace({})", rootLessPath);
    std::string rootedPath;

    std::smatch matchesA;
    std::regex rootFindPattern("\\{root\\[.*?\\]\\}");
    if (std::regex_search(rootLessPath, matchesA, rootFindPattern)) {
        std::string siteRootOverwriteName = matchesA.str(0);

        std::smatch matchesB;
        std::regex rootBracketPattern("\\[(.*?)\\]");
        if (std::regex_search(rootLessPath, matchesB, rootBracketPattern)) {
            std::string bracketedString = matchesB.str(0);
            bracketedString = bracketedString.substr(1, bracketedString.length() - 2);
            try {
                std::string replacement = m_siteRoots.at(bracketedString);
                rootedPath = std::regex_replace(rootLessPath, rootFindPattern, replacement);
                m_Log->info(m_Log->key("AyonApi"), "AyonApi::rootReplace({}) rooted", rootedPath);
                return rootedPath;
            }
            catch (std::out_of_range &e) {
                m_Log->warn("AyonApi::rootedPath error acured {}, list off available root replace str: ");
                for (auto &g: m_siteRoots) {
                    m_Log->warn("Key: {}, replacement: {}", g.first, g.second);
                }
                return rootLessPath;
            }
        }
    }

    return rootLessPath;
};

nlohmann::json
AyonApi::GET(const std::shared_ptr<std::string> endPoint,
             const std::shared_ptr<httplib::Headers> headers,
             uint8_t successStatus) {
    PerfTimer("AyonApi::GET");
    m_Log->info(m_Log->key("AyonApi"), "AyonApi::GET({})", *endPoint);

    httplib::Result response;
    int responseStatus;
    uint8_t retries = 0;
    while (retries <= m_maxCallRetries) {
        try {
            response = m_AyonServer->Get(*endPoint, *headers);
            responseStatus = response->status;
            retries++;

            if (responseStatus == successStatus) {
                return nlohmann::json::parse(response->body);
            }
            else {
                m_Log->info("AyonApi::serialCorePost wrong status code: {} expected: {}", responseStatus, successStatus);
                if (responseStatus == 401) {
                    m_Log->warn("not logged in 401 ");
                    return nlohmann::json();
                }
                if (responseStatus == 500) {
                    m_Log->warn("internal server error ");
                    return nlohmann::json();
                }
                std::this_thread::sleep_for(std::chrono::milliseconds(
                    responseStatus == m_ServerBusyCode ? m_RequestDelayWhenServerBusy : m_retryWait));
            }

        }
        catch (const httplib::Error &e) {
            m_Log->warn("Request Failed because: {}", httplib::to_string(e));
            break;
        }
        m_Log->warn("The connection failed. Retry now.");
    }
    return nlohmann::json();
};

nlohmann::json
AyonApi::SPOST(const std::shared_ptr<std::string> endPoint,
               const std::shared_ptr<httplib::Headers> headers,
               nlohmann::json jsonPayload,
               const std::shared_ptr<uint8_t> successStatus) {
    PerfTimer("AyonApi::SPOST");
    m_Log->info(m_Log->key("AyonApi"), "AyonApi::SPOST endPoint: {}, jsonPayload: {}, successStatus: {}", *endPoint,
                jsonPayload.dump(), *successStatus);

    nlohmann::json jsonResponse;

    if (jsonPayload.empty()) {
        m_Log->info("JSON payload is empty. No request created");
        return jsonResponse;
    }

    if (endPoint == nullptr || headers == nullptr || successStatus == nullptr) {
        m_Log->error("One or more of the provided pointers are null: endPoint, headers, successStatus.");

        return jsonResponse;
    }

    m_AyonServerMutex.lock();

    std::string payload = jsonPayload.dump();
    std::string rawResponse = serialCorePost(*endPoint, *headers, payload, *successStatus);

    if (!rawResponse.empty()) {
        jsonResponse = nlohmann::json::parse(rawResponse)[0];   // TODO figure out why this is isnt the same as CPOST and
                                                              // find a better way to make sure its not an array
    }
    else {
        m_Log->warn("SPOST can't parse JSON // response empty");
    }
    m_AyonServerMutex.unlock();
    return jsonResponse;
};

nlohmann::json
AyonApi::CPOST(const std::shared_ptr<std::string> endPoint,
               const std::shared_ptr<httplib::Headers> headers,
               nlohmann::json jsonPayload,
               const std::shared_ptr<uint8_t> successStatus) {
    PerfTimer("AyonApi::CPOST");
    m_Log->info(m_Log->key("AyonApi"), "AyonApi::CPOST endPoint: {}, jsonPayload: {}, successStatus: {}", *endPoint,
                jsonPayload.dump(), *successStatus);
    nlohmann::json jsonResponse;

    if (jsonPayload.empty()) {
        m_Log->info("JSON payload is empty. No request created");
        return jsonResponse;
    }

    if (endPoint == nullptr || headers == nullptr || successStatus == nullptr) {
        m_Log->error("One or more of the provided pointers are null: endPoint, headers, successStatus");

        return jsonResponse;
    }

    std::string payload = jsonPayload.dump();
    std::string rawResponse = GenerativeCorePost(*endPoint, *headers, payload, *successStatus);
    if (!rawResponse.empty()) {
        jsonResponse = nlohmann::json::parse(rawResponse);
    }
    else {
        m_Log->warn("CPOST can't parse JSON // response empty");
    }
    return jsonResponse;
};
// TODO change the pointer work in here because the pointers consume more data that coping would
std::pair<std::string, std::string>
AyonApi::resolvePath(const std::string &uriPath) {
    PerfTimer("AyonApi::resolvePath");
    m_Log->info(m_Log->key("AyonApi"), "AyonApi::resolvePath({})", uriPath);

    if (uriPath.empty()) {
        m_Log->info("Path was empty: {}", uriPath.c_str());
        return {};
    }
    std::pair<std::string, std::string> resolvedAsset;
    nlohmann::json jsonPayload = {{"resolveRoots", false}, {"uris", nlohmann::json::array({uriPath})}};
    httplib::Headers headers = {{"X-ayon-site-id", m_siteId}};
    uint8_t sucsessStatus = 200;

    nlohmann::json response
        = SPOST(std::make_shared<std::string>(m_uriResolverEndpoint + m_uriResolverEndpointPathOnlyVar),
                std::make_shared<httplib::Headers>(headers), jsonPayload, std::make_shared<uint8_t>(successStatus));

    resolvedAsset = getAssetIdent(response);
    return resolvedAsset;
};

std::unordered_map<std::string, std::string>
AyonApi::batchResolvePath(std::vector<std::string> &uriPaths) {
    PerfTimer("AyonApi::batchResolvePath");
    m_Log->info(m_Log->key("AyonApi"), "AyonApi::batchResolvePath({})",
                std::accumulate(
                    uriPaths.begin(), uriPaths.end(), std::string(),
                    [](const std::string &a, const std::string &b) { return a + (a.length() > 0 ? " " : "") + b; }));

    if (uriPaths.size() < 1) {
        m_Log->warn("AyonApi::batchResolvePath Got empty vector stopped resolution");
        return {};
    }

    if (m_batchResolveOptimizeVector) {
        {
            PerfTimer("AyonApi::batchResolvePath::sanitizeVector");
            std::set<std::string> s;
            size_t size = uriPaths.size();

            for (size_t i = 0; i < size; ++i) s.insert(uriPaths[i]);
            uriPaths.assign(s.begin(), s.end());
            m_Log->info("Make sure that the vector has no duplicates. vecSize before: {} after: {}", size,
                        uriPaths.size());
        }
    }
    std::unordered_map<std::string, std::string> assetIdentGrp;
    std::vector<std::future<nlohmann::json>> futures;

    std::shared_ptr<httplib::Headers> headers
        = std::make_shared<httplib::Headers>(m_headers);

    std::shared_ptr<std::string> batchResolveEndpoint;
    if (m_pathOnlyResolution) {
        batchResolveEndpoint
            = std::make_shared<std::string>(std::string_view(m_uriResolverEndpoint + m_uriResolverEndpointPathOnlyVar));
    }
    else {
        batchResolveEndpoint = std::make_shared<std::string>(m_uriResolverEndpoint);
    }

    std::shared_ptr<uint8_t> expectedResponseStatus = std::make_shared<uint8_t>(200);

    std::string grpReason;
    int uriPathsVecSize = uriPaths.size();
    int groupSize;
    int groupAmount;

    // set defaults for the grouping in case the vector is too small
    groupSize = uriPathsVecSize;
    groupAmount = 1;
    grpReason = "The vector is too small.";

    // check what scaling the groups should have
    if (uriPathsVecSize > m_minVecSizeForGroupSplitAsyncRequests) {
        // vector size is large enough to build groups
        // double result = static_cast<double>(uriPathsVecSize) / num_threads;
        groupSize = std::ceil(static_cast<double>(uriPathsVecSize) / m_num_threads);
        if (groupSize > m_minGrpSizeForAsyncRequests) {
            // the group size is large enough to build groups from them
            if (groupSize < m_maxGroupSizeForAsyncRequests) {
                // now it's bigger than 5 and smaller than 500
                // now we can just generate a group per thread and set the group amount
                groupSize = std::ceil(static_cast<double>(uriPathsVecSize) / m_num_threads);
                groupAmount = std::floor(static_cast<double>(uriPathsVecSize) / groupSize);

                // TODO explicit rounding .x group amount
                grpReason = "5> <500 build group amount by size";
            }
            else {
                // the groups are too big
                // we have to generate more groups than we have threads
                groupSize = m_regroupSizeForAsyncRequests;
                groupAmount = std::floor(static_cast<double>(uriPathsVecSize) / m_regroupSizeForAsyncRequests);
                grpReason = "The groups are too big. We will build more than we have CPU cores.";
            }
        }
        else {
            // the groups are too small so we build groups by size

            groupSize = std::min((int)m_regroupSizeForAsyncRequests, uriPathsVecSize);
            groupAmount = std::floor(static_cast<double>(uriPathsVecSize) / groupSize);
            grpReason = "If groups are too small, we will build them by size.";
        }
    }
    m_Log->info(
        "AyonApi::batchResolvePath Build groups with grpSize: {} grpAmount: {} groupingReason: {} vectorSize: {}",
        groupSize, groupAmount, grpReason, uriPathsVecSize);

    int groupStartPos = 0;
    int groupEndPos;
    for (int thread = 0; thread < groupAmount; thread++) {
        groupEndPos = groupSize * (thread + 1);
        std::string perTimerLoopName = "AyonApi::batchResolvePath Thread Loop: " + std::to_string(thread);
        PerfTimer(perTimerLoopName.c_str());

        // check if we are too close to the end and extend the group to catch all the data and end the loop

        if (uriPathsVecSize - groupEndPos < groupSize + (groupSize / 2)) {
            m_Log->info("the group with the threadId: {} It is too close to the end. This group will be extended. ",
                        thread);
            groupEndPos = uriPathsVecSize - 1;
            thread = groupAmount;
        }
        nlohmann::json threadLocalUriPathsJsonArray;

        for (int i = groupStartPos; i <= groupEndPos; i++) {
            threadLocalUriPathsJsonArray.push_back(uriPaths[i]);
        }
        nlohmann::json threadLocalJsonPayload = {{"resolveRoots", false}, {"uris", threadLocalUriPathsJsonArray}};

        futures.push_back(std::async(std::launch::async, &AyonApi::CPOST, this, std::ref(batchResolveEndpoint),
                                     std::ref(headers), std::move(threadLocalJsonPayload),
                                     std::ref(expectedResponseStatus)));

        groupStartPos = groupEndPos + 1;
    }

    std::vector<nlohmann::json> jsonResponses;
    for (auto &future: futures) {
        for (const auto &assetRaw: future.get()) {
            assetIdentGrp.emplace(getAssetIdent(assetRaw));
        }
    }

    return assetIdentGrp;
};
// TODO make it so that hero version is chosen if available
std::pair<std::string, std::string>
AyonApi::getAssetIdent(const nlohmann::json &uriResolverResponse) {
    PerfTimer("AyonApi::getAssetIdent");
    m_Log->info(m_Log->key("AyonApi"), "AyonApi::getAssetIdent({})", uriResolverResponse.dump());

    std::pair<std::string, std::string> AssetIdent;
    if (uriResolverResponse.empty()) {
        return AssetIdent;
    }
    try {
        AssetIdent.first = uriResolverResponse.at("uri");
        if (uriResolverResponse.at("entities").size() > 1) {
            m_Log->warn("Uri resolution returned more than one path (%s)", uriResolverResponse.at("entities").dump());
        }
        AssetIdent.second = rootReplace(
            uriResolverResponse.at("entities").at(uriResolverResponse.at("entities").size() - 1).at("filePath"));
    }
    catch (const nlohmann::json::exception &e) {
        m_Log->warn("asset identification cant be generated {}", uriResolverResponse.dump());
    }
    return AssetIdent;
};

std::string
AyonApi::getKey() {
    PerfTimer("AyonApi::getKey");
    m_Log->info(m_Log->key("AyonApi"), "AyonApi::getKey");
    return m_authKey;
};

std::string
AyonApi::getUrl() {
    PerfTimer("AyonApi::getUrl");
    m_Log->info(m_Log->key("AyonApi"), "AyonApi::getUrl");
    return m_serverUrl;
}

//--------------------- Internal Funcs
std::string
AyonApi::serialCorePost(const std::string &endPoint,
                        httplib::Headers headers,
                        std::string &Payload,
                        const int &successStatus) {
    PerfTimer("AyonApi::serialCorePost");
    m_Log->info(m_Log->key("AyonApi"), "AyonApi::serialCorePost() endPoint: {}, Payload: {}, successStatus: {}",
                endPoint, Payload, successStatus);

    httplib::Result response;
    int responseStatus;
    uint8_t retries = 0;
    while (retries <= m_maxCallRetries) {
        try {
            response = m_AyonServer->Post(endPoint, headers, Payload, "application/json");
            responseStatus = response->status;
            retries++;

            if (responseStatus == successStatus) {
                return response->body;
            }
            else {
                m_Log->info("AyonApi::serialCorePost wrong status code: {} expected: {}", responseStatus, successStatus);
                if (responseStatus == 401) {
                    m_Log->warn("not logged in 401 ");
                    return "";
                }
                if (responseStatus == 500) {
                    m_Log->warn("internal server error ");
                    return "";
                }
                std::this_thread::sleep_for(std::chrono::milliseconds(
                    responseStatus == m_ServerBusyCode ? m_RequestDelayWhenServerBusy : m_retryWait));
            }
        }
        catch (const httplib::Error &e) {
            m_Log->warn("Request Failed because: {}", httplib::to_string(e));
            break;
        }
        m_Log->warn("The connection failed. Retry now.");
    }
    return "";
};

std::string
AyonApi::GenerativeCorePost(const std::string &endPoint,
                            httplib::Headers headers,
                            std::string &Payload,
                            const int &successStatus) {
    PerfTimer("AyonApi::GenerativeCorePost");
    m_Log->info(m_Log->key("AyonApi"), "AyonApi::GenerativeCorePost() endPoint: {}, Payload: {}, successStatus: {}",
                endPoint, Payload, successStatus);

    httplib::Client AyonServerClient(m_serverUrl);
    AyonServerClient.set_bearer_token_auth(m_authKey);
    AyonServerClient.set_connection_timeout(m_connectionTimeoutMax);
    AyonServerClient.set_read_timeout(m_readTimeoutMax);

    httplib::Result response;
    int responseStatus;
    uint8_t retries = 0;
    bool ffoLock = false;
    uint16_t loopIteration = 0;
    while (retries <= m_maxCallRetries || m_GenerativeCorePostMaxLoopIterations > loopIteration) {
        loopIteration++;
        m_Log->info("AyonApi::GenerativeCorePost while loop thread {} iteration {}",
                    std::hash<std::thread::id>{}(std::this_thread::get_id()), loopIteration);

        if (ffoLock) {
            m_ConcurrentRequestAfterffoMutex.lock();
            m_Log->info("AyonApi::GenerativeCorePost ffoLock enabled");
            if (m_maxConcurrentRequestAfterffo >= 1) {
                m_maxConcurrentRequestAfterffo--;

                m_Log->info("AyonApi::GenerativeCorePost thread pool open available: {}",
                            m_maxConcurrentRequestAfterffo);

                m_ConcurrentRequestAfterffoMutex.unlock();
            }
            else {
                m_Log->info("AyonApi::GenerativeCorePost Thread pool closed");

                m_ConcurrentRequestAfterffoMutex.unlock();
                std::this_thread::sleep_for(std::chrono::milliseconds(800));
                continue;
            }
        }

        m_Log->info("AyonApi::GenerativeCorePost sending request");

        try {
            response = AyonServerClient.Post(endPoint, headers, Payload, "application/json");
            responseStatus = response->status;
            retries++;
            if (ffoLock) {
                m_ConcurrentRequestAfterffoMutex.lock();
                m_maxConcurrentRequestAfterffo++;
                m_ConcurrentRequestAfterffoMutex.unlock();
            }
            if (responseStatus == successStatus) {
                m_Log->info("AyonApi::GenerativeCorePost The request worked, unlocking and returning. ");

                return response->body;
            }
            else {
                if (responseStatus == m_ServerBusyCode) {
                    m_Log->warn("AyonApi::GenerativeCorePost The server responded with 503");

                    retries = 0;
                    ffoLock = true;
                    continue;
                }
                if (responseStatus == 401) {
                    m_Log->warn("not logged in 401 ");
                    return "";
                }
                if (responseStatus == 500) {
                    m_Log->warn("internal server error ");
                    return "";
                }
                m_Log->info("AyonApi::GenerativeCorePost wrong status code: {} expected: {} retrying", responseStatus,
                            successStatus);
                std::this_thread::sleep_for(std::chrono::milliseconds(m_retryWait));
                continue;
            }
        }
        catch (const httplib::Error &e) {
            m_Log->warn("AyonApi::GenerativeCorePost Request Failed because: {}", httplib::to_string(e));
            break;
        }
    }

    m_Log->warn(
        "AyonApi::GenerativeCorePost Too many resolve retries without the correct response code for: {}, on: {}",
        Payload, endPoint);
    return "";
};

std::string
AyonApi::convertUriVecToString(const std::vector<std::string> &uriVec) {
    PerfTimer("AyonApi::convertUriVecToString");
    m_Log->info(m_Log->key("AyonApi"), "AyonApi::convertUriVecToString({})",
                std::accumulate(uriVec.begin(), uriVec.end(), std::string()));

    std::string payload = R"({{"resolveRoots": true,"uris": [)";

    for (size_t i = 0; i <= uriVec.size(); i++) {
        payload += uriVec[i];
    }

    payload += "]}";

    return payload;
};

std::shared_ptr<AyonLogger>
AyonApi::logPointer() {
    m_Log->info(m_Log->key("AyonApi"), "AyonApi::logPointer()");
    return m_Log;
};

bool
AyonApi::isSSL() const {
    return m_serverUrl.rfind("https://", 0) == 0;
}


void
AyonApi::setSSL() {
    const char* envCertFile = getenv("SSL_CERT_FILE");
    if (envCertFile) {
        if (std::filesystem::exists(envCertFile)) {
            m_Log->info("Using cert based on env variable (SSL_CERT_FILE): {}", envCertFile);
            m_AyonServer->set_ca_cert_path(envCertFile);
            return;
        }
    }

    std::filesystem::path opensslDirCLI(getOpenSSLDirByCLI());
    opensslDirCLI /= "cert.pem";
    std::string certFileCLI = opensslDirCLI.string();

    if (std::filesystem::exists(certFileCLI)) {
        m_Log->info("Using cert based on CLI var: {}", certFileCLI);
        m_AyonServer->set_ca_cert_path(certFileCLI.c_str());
        return;
    } 

    std::filesystem::path opensslDirSSLEAY(getOpenSSLDir());
    opensslDirSSLEAY /= "cert.pem";
    std::string certFileSSLEAY = opensslDirSSLEAY.string();

    if (std::filesystem::exists(certFileSSLEAY)) {
        m_Log->info("Using cert based on SSLEAY_DIR: {}", certFileSSLEAY);
        m_AyonServer->set_ca_cert_path(certFileSSLEAY.c_str());
        return;
    }

    m_Log->info("Failed to determine the OpenSSL directory or load system CAs. Falling back to bundled certificate path.");                        
    
    std::filesystem::path soPath;
    Dl_info dl_info;

    if (dladdr(reinterpret_cast<const void*>(&parseOutput), &dl_info) && dl_info.dli_fname) {
        soPath = dl_info.dli_fname;
    }

    if (!soPath.empty()) {
        std::filesystem::path resolverRoot = soPath.parent_path().parent_path();
        
        std::filesystem::path bundledPath = (
            resolverRoot / "certs" / "cacert.pem"
        );

        std::string certPath = bundledPath.string();

        if (std::filesystem::exists(certPath)) {
            m_Log->info("Using bundled certificate (via SO path): {}", certPath);
            m_AyonServer->set_ca_cert_path(certPath.c_str());
            return;
        }

        m_Log->error("Bundled cacert.pem file not found at expected runtime path: {}", certPath);
    } else {
        m_Log->error("Failed to determine the path of the loaded shared library (dladdr failed).");
    }

    throw std::runtime_error("Failed to set SSL certificate path. No valid certificate found.");
}
