#include "AyonCppApi.h"

#include <cmath>
#include <cstdint>
#include <cstdlib>

#include <algorithm>
#include <chrono>
#include <filesystem>
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
#include <string_view>
#include <thread>
#include <unordered_map>
#include <utility>
#include <vector>

#include <sys/types.h>

#ifdef _WIN32
    #include <windows.h>
    #include <wincrypt.h>
#else
    #include <dlfcn.h>
#endif

#include "backward.hpp"
#include "httplib.h"
#include "nlohmann/json.hpp"
#include "nlohmann/json_fwd.hpp"

#include "devMacros.h"
#include "perfPrinter.h"

// TODO implement the better Crash handler
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
    : m_numThreads(concurrency.value_or(std::max(int(std::thread::hardware_concurrency() / 2), 1))),
      m_authKey(authKey),
      m_serverUrl(serverUrl),
      m_ayonProjectName(ayonProjectName),
      m_siteId(siteId) {
    PerfTimer("AyonApi::AyonApi");

    // ----------- Resolve Log Path
    std::filesystem::path logPath;
    if (logFilePos && !logFilePos->empty()) {
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

    // ----------- Init m_log (Singleton Logic)
    std::cout << "[AyonApi] Retrieving AyonLogger Singleton..." << std::endl;
    
    AyonLogger& loggerRef = AyonLogger::getInstance();
    
    if (!logPath.empty()) {
        loggerRef.initFileLogger(logPath.string());
    }

    m_log = std::shared_ptr<AyonLogger>(&loggerRef, [](AyonLogger*){});
    m_log->registerLoggingKey("AyonApi");
    m_log->setLogLevelFromEnv();
    m_log->info(m_log->key("AyonApi"), "Init AyonServer httplib::Client");
    
    m_ayonServer = std::make_unique<httplib::Client>(m_serverUrl);
    // Reuse the TCP/TLS connection across resolves instead of a fresh handshake per
    // request. Over a WAN link the handshake dominates per-call latency, so this is
    // a large win for the serial resolve path and the prewarm batched resolves.
    // Gated so the keep-alive contribution can be benchmarked: set
    // AYON_RESOLVER_NO_KEEPALIVE=1 to fall back to a fresh handshake per request.
    if (!std::getenv("AYON_RESOLVER_NO_KEEPALIVE")) {
        m_ayonServer->set_keep_alive(true);
    }
    m_log->info(m_log->key("AyonApi"), "After creating httplib::Client - {}", m_serverUrl);

    if (isSSL()) {
        std::string ayonSSLPath = std::getenv("AYON_SSL_CERT_PATH") ? std::getenv("AYON_SSL_CERT_PATH") : "";
        if (!ayonSSLPath.empty()) {
            m_log->info(m_log->key("AyonApi"), "Using AYON_SSL_CERT_PATH: {}", ayonSSLPath);
            m_ayonServer->set_ca_cert_path(ayonSSLPath.c_str());
        } else {
            m_log->warn(m_log->key("AyonApi"), "No AYON_SSL_CERT_PATH set, trying to get OpenSSL dir");
            try {
                setSSL();
            } catch (const std::exception &e) {
                m_log->error("Failed to get OpenSSL directory: {}", e.what());
                m_ayonServer->set_ca_cert_path(nullptr); 
            }
        }
        m_ayonServer->enable_server_certificate_verification(true);
    }

    if (!m_ayonServer) {
        m_log->error("m_ayonServer is null. serverUrl='{}'", m_serverUrl);
        throw std::runtime_error("AyonApi: HTTP client not initialized");
    }

    httplib::Result res;
    try {
        res = m_ayonServer->Get("/api/info");
    } catch (const std::exception& e) {
        m_log->error("Exception during GET /api/info: {}", e.what());
        throw;
    }

    if (!res) {
        m_log->error("Failed to connect to the Ayon server.");
    } else {
        m_log->info(m_log->key("AyonApi"), "Ayon server info: {}", res->body);
        m_log->info(m_log->key("AyonApi"), "Status code: {}", res->status);

        m_headers = {
            {"X-Api-Key", m_authKey}
        };
        // Only advertise a site id when we actually have one. A service-account
        // setup (or any machine without a registered AYON site) has no valid
        // site, and the server returns 400 "Invalid site id" if the header is
        // present but empty/unknown. Omitting it lets resolution proceed.
        if (!m_siteId.empty()) {
            m_headers.emplace("X-ayon-site-id", m_siteId);
        }

        auto resMe = m_ayonServer->Get("/api/users/me", m_headers);
        if (resMe && resMe->status != 200) {
            m_headers.erase("X-Api-Key");
            m_ayonServer->set_bearer_token_auth(m_authKey);
        }
    }

    m_log->info(m_log->key("AyonApi"), "Constructor Getting Site Roots");
    getSiteRoots();
}

AyonApi::~AyonApi() {
    m_log->info(m_log->key("AyonApi"), "AyonApi::~AyonApi()");
};

const std::unordered_map<std::string, std::string>&
AyonApi::getSiteRoots() {
    m_log->info(m_log->key("AyonApi"), "AyonApi::getSiteRoots()");
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
            m_log->error("AyonApi::getSiteRoots response is empty");
            return m_siteRoots;
        } else {
            m_siteRoots = response;
        }
        
    }
    if (m_log->isKeyActive(m_log->key("AyonApi"))) {
        m_log->info(m_log->key("AyonApi"), "found site Roots: ");
        for (auto &e: m_siteRoots) {
            m_log->info("{}, {}", e.first, e.second);
        }
    }

    return m_siteRoots;
};

std::string
AyonApi::rootReplace(const std::string &rootLessPath) {
    m_log->info(m_log->key("AyonApi"), "AyonApi::rootReplace({})", rootLessPath);
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
                m_log->info(m_log->key("AyonApi"), "AyonApi::rootReplace({}) rooted", rootedPath);
                return rootedPath;
            }
            catch (std::out_of_range &e) {
                m_log->warn("AyonApi::rootedPath error occurred {}, list of available root replace str: ", e.what());
                for (auto &g: m_siteRoots) {
                    m_log->warn("Key: {}, replacement: {}", g.first, g.second);
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
    m_log->info(m_log->key("AyonApi"), "AyonApi::GET({})", *endPoint);

    httplib::Result response;
    int responseStatus;
    uint8_t retries = 0;
    while (retries <= m_maxCallRetries) {
        try {
            response = m_ayonServer->Get(*endPoint, *headers);

            if (!response) {
                m_log->warn("AyonApi::GET response is null: {}", httplib::to_string(response.error()));
                return nlohmann::json();
            }

            responseStatus = response->status;
            retries++;

            if (responseStatus == successStatus) {
                return nlohmann::json::parse(response->body);
            } else {
                m_log->info("AyonApi::GET wrong status code: {} expected: {}", responseStatus, successStatus);
                if (responseStatus == 401) {
                    m_log->warn("not logged in 401 ");
                    return nlohmann::json();
                }
                if (responseStatus == 500) {
                    m_log->warn("internal server error ");
                    return nlohmann::json();
                }
                std::this_thread::sleep_for(std::chrono::milliseconds(
                    responseStatus == m_serverBusyCode ? m_requestDelayWhenServerBusy : m_retryWait));
            }

        }
        catch (const httplib::Error &e) {
            m_log->warn("Request Failed because: {}", httplib::to_string(e));
            break;
        }
        m_log->warn("The connection failed. Retry now.");
    }
    return nlohmann::json();
};

nlohmann::json
AyonApi::SPOST(const std::shared_ptr<std::string> endPoint,
               const std::shared_ptr<httplib::Headers> headers,
               nlohmann::json jsonPayload,
               const std::shared_ptr<uint8_t> successStatus) {
    PerfTimer("AyonApi::SPOST");
    m_log->info(m_log->key("AyonApi"), "AyonApi::SPOST endPoint: {}, jsonPayload: {}, successStatus: {}", *endPoint,
                jsonPayload.dump(), *successStatus);

    nlohmann::json jsonResponse;

    if (jsonPayload.empty()) {
        m_log->info("JSON payload is empty. No request created");
        return jsonResponse;
    }

    if (endPoint == nullptr || headers == nullptr || successStatus == nullptr) {
        m_log->error("One or more of the provided pointers are null: endPoint, headers, successStatus.");

        return jsonResponse;
    }

    std::string payload = jsonPayload.dump();
    std::string rawResponse;
    
    {
        std::lock_guard<std::mutex> lock(m_ayonServerMutex);
        rawResponse = serialCorePost(*endPoint, *headers, payload, *successStatus);
    }

    if (!rawResponse.empty()) {
        try {
            jsonResponse = nlohmann::json::parse(rawResponse)[0];
        }
        catch (const nlohmann::json::exception &e) {
            m_log->error("SPOST JSON parse failed: {}", e.what());
        }
    }
    else {
        m_log->warn("SPOST can't parse JSON // response empty");
    }
    
    return jsonResponse;
};

nlohmann::json
AyonApi::CPOST(const std::shared_ptr<std::string> endPoint,
               const std::shared_ptr<httplib::Headers> headers,
               nlohmann::json jsonPayload,
               const std::shared_ptr<uint8_t> successStatus) {
    PerfTimer("AyonApi::CPOST");
    m_log->info(m_log->key("AyonApi"), "AyonApi::CPOST endPoint: {}, jsonPayload: {}, successStatus: {}", *endPoint,
                jsonPayload.dump(), *successStatus);
    nlohmann::json jsonResponse;

    if (jsonPayload.empty()) {
        m_log->info("JSON payload is empty. No request created");
        return jsonResponse;
    }

    if (endPoint == nullptr || headers == nullptr || successStatus == nullptr) {
        m_log->error("One or more of the provided pointers are null: endPoint, headers, successStatus");

        return jsonResponse;
    }

    std::string payload = jsonPayload.dump();
    std::string rawResponse = generativeCorePost(*endPoint, *headers, payload, *successStatus);
    if (!rawResponse.empty()) {
        jsonResponse = nlohmann::json::parse(rawResponse);
    }
    else {
        m_log->warn("CPOST can't parse JSON // response empty");
    }
    return jsonResponse;
};

// TODO change the pointer work in here because the pointers consume more data that copying would
std::pair<std::string, std::string>
AyonApi::resolvePath(const std::string &uriPath) {
    PerfTimer("AyonApi::resolvePath");
    m_log->info(m_log->key("AyonApi"), "AyonApi::resolvePath({})", uriPath);

    if (uriPath.empty()) {
        m_log->info("Path was empty: {}", uriPath.c_str());
        return {};
    }
    std::pair<std::string, std::string> resolvedAsset;
    nlohmann::json jsonPayload = {{"resolveRoots", false}, {"uris", nlohmann::json::array({uriPath})}};
    uint8_t successStatus = 200;

    nlohmann::json response
        = SPOST(std::make_shared<std::string>(m_uriResolverEndpoint + m_uriResolverEndpointPathOnlyVar),
                std::make_shared<httplib::Headers>(m_headers), jsonPayload, std::make_shared<uint8_t>(successStatus));

    resolvedAsset = getAssetIdent(response);
    return resolvedAsset;
};

std::unordered_map<std::string, std::string>
AyonApi::batchResolvePath(std::vector<std::string> &uriPaths) {
    PerfTimer("AyonApi::batchResolvePath");
    m_log->info(m_log->key("AyonApi"), "AyonApi::batchResolvePath({})",
                std::accumulate(
                    uriPaths.begin(), uriPaths.end(), std::string(),
                    [](const std::string &a, const std::string &b) { return a + (a.length() > 0 ? " " : "") + b; }));

    if (uriPaths.size() < 1) {
        m_log->warn("AyonApi::batchResolvePath Got empty vector stopped resolution");
        return {};
    }

    if (m_batchResolveOptimizeVector) {
        {
            PerfTimer("AyonApi::batchResolvePath::sanitizeVector");
            std::set<std::string> s;
            size_t size = uriPaths.size();

            for (size_t i = 0; i < size; ++i) s.insert(uriPaths[i]);
            uriPaths.assign(s.begin(), s.end());
            m_log->info("Make sure that the vector has no duplicates. vecSize before: {} after: {}", size,
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
        groupSize = std::ceil(static_cast<double>(uriPathsVecSize) / m_numThreads);
        if (groupSize > m_minGroupSizeForAsyncRequests) {
            // the group size is large enough to build groups from them
            if (groupSize < m_maxGroupSizeForAsyncRequests) {
                // now it's bigger than 5 and smaller than 500
                // now we can just generate a group per thread and set the group amount
                groupSize = std::ceil(static_cast<double>(uriPathsVecSize) / m_numThreads);
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
    m_log->info(
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
            m_log->info("the group with the threadId: {} It is too close to the end. This group will be extended. ",
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

std::unordered_map<std::string, std::string>
AyonApi::batchResolvePathSerial(const std::vector<std::string> &uriPaths) {
    PerfTimer("AyonApi::batchResolvePathSerial");
    m_log->info(m_log->key("AyonApi"), "AyonApi::batchResolvePathSerial({} uris)", uriPaths.size());

    std::unordered_map<std::string, std::string> assetIdentGrp;
    if (uriPaths.empty()) {
        return assetIdentGrp;
    }

    // Drop empties up front so chunk boundaries are stable and we never POST blank uris.
    std::vector<std::string> cleanUris;
    cleanUris.reserve(uriPaths.size());
    for (const auto &uri: uriPaths) {
        if (!uri.empty()) {
            cleanUris.push_back(uri);
        }
    }
    if (cleanUris.empty()) {
        return assetIdentGrp;
    }

    const std::string endPoint
        = m_pathOnlyResolution ? m_uriResolverEndpoint + m_uriResolverEndpointPathOnlyVar : m_uriResolverEndpoint;

    // Split large frontiers into capped chunks (see m_maxSerialBatchSize). Each chunk is its
    // own POST over the keep-alive client, so the server processes and releases a bounded
    // batch at a time. For typical frontiers (<= m_maxSerialBatchSize) this is a single
    // request, identical to the un-chunked path.
    const size_t chunkSize = m_maxSerialBatchSize;
    for (size_t start = 0; start < cleanUris.size(); start += chunkSize) {
        const size_t end = std::min(start + chunkSize, cleanUris.size());

        nlohmann::json uriArray = nlohmann::json::array();
        for (size_t i = start; i < end; ++i) {
            uriArray.push_back(cleanUris[i]);
        }
        nlohmann::json jsonPayload = {{"resolveRoots", false}, {"uris", uriArray}};
        std::string payload = jsonPayload.dump();

        std::string rawResponse;
        {
            std::lock_guard<std::mutex> lock(m_ayonServerMutex);
            rawResponse = serialCorePost(endPoint, m_headers, payload, 200);
        }
        if (rawResponse.empty()) {
            m_log->warn("AyonApi::batchResolvePathSerial empty response for chunk [{}, {})", start, end);
            continue;
        }

        try {
            nlohmann::json responseArray = nlohmann::json::parse(rawResponse);
            for (const auto &assetRaw: responseArray) {
                assetIdentGrp.emplace(getAssetIdent(assetRaw));
            }
        }
        catch (const nlohmann::json::exception &e) {
            m_log->error("AyonApi::batchResolvePathSerial JSON parse failed: {}", e.what());
        }
    }

    return assetIdentGrp;
};

// TODO make it so that hero version is chosen if available
std::pair<std::string, std::string>
AyonApi::getAssetIdent(const nlohmann::json &uriResolverResponse) {
    PerfTimer("AyonApi::getAssetIdent");
    m_log->info(m_log->key("AyonApi"), "AyonApi::getAssetIdent({})", uriResolverResponse.dump());

    std::pair<std::string, std::string> AssetIdent;
    if (uriResolverResponse.empty()) {
        return AssetIdent;
    }
    try {
        AssetIdent.first = uriResolverResponse.at("uri");
        if (uriResolverResponse.at("entities").size() > 1) {
            m_log->warn("Uri resolution returned more than one path: {}", uriResolverResponse.at("entities").dump());
        }
        AssetIdent.second = rootReplace(
            uriResolverResponse.at("entities").at(uriResolverResponse.at("entities").size() - 1).at("filePath"));
    }
    catch (const nlohmann::json::exception &e) {
        m_log->warn("asset identification cant be generated {}", uriResolverResponse.dump());
    }
    return AssetIdent;
};

std::string
AyonApi::getKey() {
    PerfTimer("AyonApi::getKey");
    m_log->info(m_log->key("AyonApi"), "AyonApi::getKey");
    return m_authKey;
};

std::string
AyonApi::getUrl() {
    PerfTimer("AyonApi::getUrl");
    m_log->info(m_log->key("AyonApi"), "AyonApi::getUrl");
    return m_serverUrl;
}

//--------------------- Internal Funcs
std::string
AyonApi::serialCorePost(const std::string &endPoint,
                        httplib::Headers headers,
                        std::string &Payload,
                        const int &successStatus) {
    PerfTimer("AyonApi::serialCorePost");
    m_log->info(m_log->key("AyonApi"), "AyonApi::serialCorePost() endPoint: {}, Payload: {}, successStatus: {}",
                endPoint, Payload, successStatus);

    httplib::Result response;
    int responseStatus;
    uint8_t retries = 0;
    while (retries <= m_maxCallRetries) {
        try {
            response = m_ayonServer->Post(endPoint, headers, Payload, "application/json");
            responseStatus = response->status;
            retries++;

            if (responseStatus == successStatus) {
                return response->body;
            }
            else {
                m_log->info("AyonApi::serialCorePost wrong status code: {} expected: {}", responseStatus, successStatus);
                if (responseStatus == 401) {
                    m_log->warn("not logged in 401 ");
                    return "";
                }
                if (responseStatus == 500) {
                    m_log->warn("internal server error ");
                    return "";
                }
                std::this_thread::sleep_for(std::chrono::milliseconds(
                    responseStatus == m_serverBusyCode ? m_requestDelayWhenServerBusy : m_retryWait));
            }
        }
        catch (const httplib::Error &e) {
            m_log->warn("Request Failed because: {}", httplib::to_string(e));
            break;
        }
        m_log->warn("The connection failed. Retry now.");
    }
    return "";
};

std::string
AyonApi::generativeCorePost(const std::string &endPoint,
                            httplib::Headers headers,
                            std::string &payload,
                            const int &successStatus) {
    PerfTimer("AyonApi::generativeCorePost");
    m_log->info(m_log->key("AyonApi"), 
                "AyonApi::generativeCorePost() endPoint: {}, payload: {}, successStatus: {}",
                endPoint, payload, successStatus);

    httplib::Client ayonServerClient(m_serverUrl);
    ayonServerClient.set_bearer_token_auth(m_authKey);
    ayonServerClient.set_connection_timeout(m_connectionTimeoutMax);
    ayonServerClient.set_read_timeout(m_readTimeoutMax);

    bool serverBusyMode = false;
    uint16_t totalAttempts = 0;
    const uint16_t maxTotalAttempts = m_generativeCorePostMaxLoopIterations;

    while (totalAttempts < maxTotalAttempts) {
        totalAttempts++;
        m_log->info("AyonApi::generativeCorePost thread {} attempt {} of {}", 
                    std::hash<std::thread::id>{}(std::this_thread::get_id()),
                    totalAttempts, maxTotalAttempts);

        // Rate limiting when server is busy
        if (serverBusyMode) {
            std::unique_lock<std::mutex> lock(m_concurrentRequestAfterServerBusyMutex);
            
            // Wait for available slot
            m_serverBusyCondVar.wait(lock, [this] {
                return m_maxConcurrentRequestsAfterServerBusy >= 1;
            });
            
            m_maxConcurrentRequestsAfterServerBusy--;
            m_log->info("AyonApi::generativeCorePost acquired slot, {} remaining",
                        m_maxConcurrentRequestsAfterServerBusy);
        }

        // Make HTTP request
        httplib::Result response;
        int responseStatus = 0;
        bool requestSucceeded = false;
        
        try {
            m_log->info("AyonApi::generativeCorePost sending request");
            response = ayonServerClient.Post(endPoint, headers, payload, "application/json");
            
            if (response) {
                responseStatus = response->status;
                requestSucceeded = true;
            } else {
                m_log->warn("AyonApi::generativeCorePost request returned null response");
            }
        }
        catch (const httplib::Error &e) {
            m_log->error("AyonApi::generativeCorePost HTTP error: {}", httplib::to_string(e));
        }

        // Release slot if in rate-limited mode (RAII-style cleanup)
        if (serverBusyMode) {
            std::lock_guard<std::mutex> lock(m_concurrentRequestAfterServerBusyMutex);
            m_maxConcurrentRequestsAfterServerBusy++;
            m_serverBusyCondVar.notify_one();
        }

        // If request failed, retry with backoff
        if (!requestSucceeded) {
            auto backoff = std::min(m_retryWait * totalAttempts, 10000);
            std::this_thread::sleep_for(std::chrono::milliseconds(backoff));
            continue;
        }

        // Handle successful request with various status codes
        if (responseStatus == successStatus) {
            m_log->info("AyonApi::generativeCorePost success after {} attempts", totalAttempts);
            return response->body;
        }
        
        // Fatal errors - don't retry
        if (responseStatus == 401) {
            m_log->error("AyonApi::generativeCorePost authentication failed (401)");
            return "";
        }
        
        if (responseStatus == 500) {
            m_log->error("AyonApi::generativeCorePost internal server error (500)");
            return "";
        }
        
        // Server busy - enable rate limiting
        if (responseStatus == m_serverBusyCode) {
            m_log->warn("AyonApi::generativeCorePost server busy ({}), enabling rate limiting", 
                        m_serverBusyCode);
            serverBusyMode = true;
            std::this_thread::sleep_for(std::chrono::milliseconds(m_requestDelayWhenServerBusy));
            continue;
        }
        
        // Other errors - retry with backoff
        m_log->warn("AyonApi::generativeCorePost unexpected status {}, expected {}",
                    responseStatus, successStatus);
        std::this_thread::sleep_for(std::chrono::milliseconds(m_retryWait));
    }

    m_log->error("AyonApi::generativeCorePost max attempts ({}) reached for endpoint: {}", 
                 maxTotalAttempts, endPoint);
    return "";
}

std::string
AyonApi::convertUriVecToString(const std::vector<std::string> &uriVec) {
    PerfTimer("AyonApi::convertUriVecToString");
    m_log->info(m_log->key("AyonApi"), "AyonApi::convertUriVecToString({})",
                std::accumulate(uriVec.begin(), uriVec.end(), std::string()));

    nlohmann::json payload = {
        {"resolveRoots", true},
        {"uris", uriVec}
    };

    return payload.dump();
};

std::shared_ptr<AyonLogger>
AyonApi::logPointer() {
    m_log->info(m_log->key("AyonApi"), "AyonApi::logPointer()");
    return m_log;
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
            m_log->info("Using cert based on env variable (SSL_CERT_FILE): {}", envCertFile);
            m_ayonServer->set_ca_cert_path(envCertFile);
            return;
        }
    }

    std::filesystem::path opensslDirCLI(getOpenSSLDirByCLI());
    opensslDirCLI /= "cert.pem";
    std::string certFileCLI = opensslDirCLI.string();

    if (std::filesystem::exists(certFileCLI)) {
        m_log->info("Using cert based on CLI var: {}", certFileCLI);
        m_ayonServer->set_ca_cert_path(certFileCLI.c_str());
        return;
    } 

    std::filesystem::path opensslDirSSLEAY(getOpenSSLDir());
    opensslDirSSLEAY /= "cert.pem";
    std::string certFileSSLEAY = opensslDirSSLEAY.string();

    if (std::filesystem::exists(certFileSSLEAY)) {
        m_log->info("Using cert based on SSLEAY_DIR: {}", certFileSSLEAY);
        m_ayonServer->set_ca_cert_path(certFileSSLEAY.c_str());
        return;
    }

    m_log->info("Failed to determine the OpenSSL directory or load system CAs. Falling back to bundled certificate path.");                        
    
    std::filesystem::path soPath;

#ifdef _WIN32
    char path_buffer[MAX_PATH];
    HMODULE hm = NULL;

    if (GetModuleHandleExA(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | 
                          GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
                          reinterpret_cast<LPCSTR>(&parseOutput), &hm)) {
        
        if (GetModuleFileNameA(hm, path_buffer, sizeof(path_buffer)) != 0) {
            soPath = path_buffer;
        }
    }
#else
    Dl_info dl_info;
    if (dladdr(reinterpret_cast<const void*>(&parseOutput), &dl_info) && dl_info.dli_fname) {
        soPath = dl_info.dli_fname;
    }
#endif

    if (!soPath.empty()) {
        // Resolve parent directories (e.g., .../lib/ayonUsdResolver.dll -> .../lib -> ...)
        std::filesystem::path resolverRoot = soPath.parent_path().parent_path();
        
        std::filesystem::path bundledPath = (
            resolverRoot / "certs" / "cacert.pem"
        );

        std::string certPath = bundledPath.string();

        if (std::filesystem::exists(certPath)) {
            m_log->info("Using bundled certificate (via library path): {}", certPath);
            m_ayonServer->set_ca_cert_path(certPath.c_str());
            return;
        }

        m_log->error("Bundled cacert.pem file not found at expected runtime path: {}", certPath);
    } else {
        m_log->error("Failed to determine the path of the loaded shared library.");
    }

    throw std::runtime_error("Failed to set SSL certificate path. No valid certificate found.");
}
