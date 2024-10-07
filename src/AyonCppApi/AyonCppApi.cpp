
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
#include <filesystem>
#include "backward.hpp"
#include "perfPrinter.h"

// TODO implement the better Crash hanlder
backward::StackTrace st;

AyonApi::AyonApi(const std::string &logFilePos,
                 const std::string &authKey,
                 const std::string &serverUrl,
                 const std::string &ayonProjectName,
                 const std::string &siteId,
                 std::optional<int> concurrency):
    m_num_threads(concurrency.value_or(std::max(int(std::thread::hardware_concurrency() / 2), 1))),
    m_authKey(authKey),
    m_serverUrl(serverUrl),
    m_ayonProjectName(ayonProjectName),
    m_siteId(siteId) {
    PerfTimer("AyonApi::AyonApi");

    // ----------- Init m_Logger
    std::filesystem::path logFileName = "logFile.json";
    std::filesystem::path basePath = logFilePos;
    std::filesystem::path logFilePath = std::filesystem::absolute(basePath) / logFileName;

    if (std::filesystem::exists(logFilePath)) {
        logFilePath = std::filesystem::canonical(logFilePath);
    }
    else {
        std::filesystem::create_directories(logFilePath.parent_path());
    }

    m_Log = std::make_shared<AyonLogger>(AyonLogger::getInstance(logFilePath.string()));
    m_Log->LogLevlWarn();

    m_Log->info(m_Log->key("AyonApi"), "Init AyonServer httplib::Client");
    m_AyonServer = std::make_unique<httplib::Client>(m_serverUrl);
    m_AyonServer->set_bearer_token_auth(m_authKey);

    m_Log->info(m_Log->key("AyonApi"), "Constructor Getting Site Roots");
    getSiteRoots();
};
AyonApi::~AyonApi() {
    m_Log->info(m_Log->key("AyonApi"), "AyonApi::~AyonApi()");
};

std::unordered_map<std::string, std::string>*
AyonApi::getSiteRoots() {
    m_Log->info(m_Log->key("AyonApi"), "AyonApi::getSiteRoots()");
    if (m_siteRoots.size() < 1) {
        httplib::Headers headers = {{"X-ayon-site-id", m_siteId}};
        nlohmann::json response
            = GET(std::make_shared<std::string>("/api/projects/" + m_ayonProjectName + "/siteRoots"),
                  std::make_shared<httplib::Headers>(headers), 200);
        m_siteRoots = response;
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

    std::smatch matchea;
    std::regex rootFindPattern("\\{root\\[.*?\\]\\}");
    if (std::regex_search(rootLessPath, matchea, rootFindPattern)) {
        std::string siteRootOverwriteName = matchea.str(0);

        std::smatch matcheb;
        std::regex rootBraketPattern("\\[(.*?)\\]");
        if (std::regex_search(rootLessPath, matcheb, rootBraketPattern)) {
            std::string breakedString = matcheb.str(0);
            breakedString = breakedString.substr(1, breakedString.length() - 2);
            try {
                std::string replacement = m_siteRoots.at(breakedString);
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
             uint8_t sucsessStatus) {
    PerfTimer("AyonApi::GET");
    m_Log->info(m_Log->key("AyonApi"), "AyonApi::GET({})", *endPoint);

    httplib::Result response;
    int responeStatus;
    uint8_t retryes = 0;
    while (retryes <= m_maxCallRetrys) {
        try {
            response = m_AyonServer->Get(*endPoint, *headers);
            responeStatus = response->status;
            retryes++;

            if (responeStatus == sucsessStatus) {
                return nlohmann::json::parse(response->body);
            }
            else {
                m_Log->info("AyonApi::serialCorePost wrong status code: {} expected: {}", responeStatus, sucsessStatus);
                if (responeStatus == 401) {
                    m_Log->warn("not logged in 401 ");
                    return nlohmann::json();
                }
                if (responeStatus == 500) {
                    m_Log->warn("internal server error ");
                    return nlohmann::json();
                }
                std::this_thread::sleep_for(std::chrono::milliseconds(
                    responeStatus == m_ServerBusyCode ? m_RequestDelayWhenServerBusy : m_retryWaight));
            }
        }   // TODO error reason not printed
        catch (const httplib::Error &e) {
            m_Log->warn("Request Failed because: {}");
            break;
        }
        m_Log->warn("The connection failed Rety now.");
    }
    return nlohmann::json();
};

nlohmann::json
AyonApi::SPOST(const std::shared_ptr<std::string> endPoint,
               const std::shared_ptr<httplib::Headers> headers,
               nlohmann::json jsonPayload,
               const std::shared_ptr<uint8_t> sucsessStatus) {
    PerfTimer("AyonApi::SPOST");
    m_Log->info(m_Log->key("AyonApi"), "AyonApi::SPOST endPoint: {}, jsonPayload: {}, sucsessStatus: {}", *endPoint,
                jsonPayload.dump(), *sucsessStatus);

    nlohmann::json jsonRespne;

    if (jsonPayload.empty()) {
        m_Log->info("JSON payload is empty. No request created");
        return jsonRespne;
    }

    if (endPoint == nullptr || headers == nullptr || sucsessStatus == nullptr) {
        m_Log->error("One or more of the provided pointers are null: endPoint, headers, sucsessStatus.");

        return jsonRespne;
    }

    m_AyonServerMutex.lock();

    std::string payload = jsonPayload.dump();
    std::string rawResponse = serialCorePost(*endPoint, *headers, payload, *sucsessStatus);

    if (!rawResponse.empty()) {
        jsonRespne = nlohmann::json::parse(rawResponse)[0];   // TODO figure out why this is isnt the same as CPOST and
                                                              // find a better way to make shure its not a array
    }
    else {
        m_Log->warn("SPOST cant phrase JSON // response empty");
    }
    m_AyonServerMutex.unlock();
    return jsonRespne;
};

nlohmann::json
AyonApi::CPOST(const std::shared_ptr<std::string> endPoint,
               const std::shared_ptr<httplib::Headers> headers,
               nlohmann::json jsonPayload,
               const std::shared_ptr<uint8_t> sucsessStatus) {
    PerfTimer("AyonApi::CPOST");
    m_Log->info(m_Log->key("AyonApi"), "AyonApi::CPOST endPoint: {}, jsonPayload: {}, sucsessStatus: {}", *endPoint,
                jsonPayload.dump(), *sucsessStatus);
    nlohmann::json jsonRespne;

    if (jsonPayload.empty()) {
        m_Log->info("JSON payload is empty. No request created");
        return jsonRespne;
    }

    if (endPoint == nullptr || headers == nullptr || sucsessStatus == nullptr) {
        m_Log->error("One or more of the provided pointers are null: endPoint, headers, sucsessStatus");

        return jsonRespne;
    }

    std::string payload = jsonPayload.dump();
    std::string rawResponse = GenerativeCorePost(*endPoint, *headers, payload, *sucsessStatus);
    if (!rawResponse.empty()) {
        jsonRespne = nlohmann::json::parse(rawResponse);
    }
    else {
        m_Log->warn("CPOST cant phrase JSON // response empty");
    }
    return jsonRespne;
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
                std::make_shared<httplib::Headers>(headers), jsonPayload, std::make_shared<uint8_t>(sucsessStatus));

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
            PerfTimer("AyonApi::batchResolvePath::sanatizeVector");
            std::set<std::string> s;
            unsigned size = uriPaths.size();

            for (unsigned i = 0; i < size; ++i) s.insert(uriPaths[i]);
            uriPaths.assign(s.begin(), s.end());
            m_Log->info("Make sure that the vector has no duplicates. vecSize before: {} after: {}", size,
                        uriPaths.size());
        }
    }
    std::unordered_map<std::string, std::string> assetIdentGrp;
    std::vector<std::future<nlohmann::json>> futures;

    std::shared_ptr<httplib::Headers> headers
        = std::make_shared<httplib::Headers>(httplib::Headers{{"X-ayon-site-id", m_siteId}});

    std::shared_ptr<std::string> batchResolveEndpoint;
    if (m_pathOnlyReselution) {
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

    // set defaults for the grouping incase the vector is to small
    groupSize = uriPathsVecSize;
    groupAmount = 1;
    grpReason = "The vector is too small.";

    // check what scaling the groups schould have
    if (uriPathsVecSize > m_minVecSizeForGroupSplitAsyncRequests) {
        // vector size is large eonught to build groups
        // double result = static_cast<double>(uriPathsVecSize) / num_threads;
        groupSize = std::ceil(static_cast<double>(uriPathsVecSize) / m_num_threads);
        if (groupSize > m_minGrpSizeForAsyncRequests) {
            // the group size is lagre enought to build groups from them
            if (groupSize < m_maxGroupSizeForAsyncRequests) {
                // now its bigger than 5 and smaller than 500
                // now we can just generate a group per thread and set the group amount
                groupSize = std::ceil(static_cast<double>(uriPathsVecSize) / m_num_threads);
                groupAmount = std::floor(static_cast<double>(uriPathsVecSize) / groupSize);

                // TODO explicit rounding .x group amount
                grpReason = "5> <500 build group amount by size";
            }
            else {
                // the groups are to beig
                // we have to generate more groups than we have threads
                groupSize = m_regroupSizeForAsyncRequests;
                groupAmount = std::floor(static_cast<double>(uriPathsVecSize) / m_regroupSizeForAsyncRequests);
                grpReason = "The groups are too big. We will build more than we have CPU cores.";
            }
        }
        else {
            // the groups are to small so we build groups by size

            groupSize = std::min((int)m_regroupSizeForAsyncRequests, uriPathsVecSize);
            groupAmount = std::floor(static_cast<double>(uriPathsVecSize) / groupSize);
            grpReason = "If groups are too small, we will build them by size.";
        }
    }
    m_Log->info(
        "AyonApi::batchResolvePath Build groups with grpSize: {} grpAmount: {} grouingReason: {} vectorSize: {}",
        groupSize, groupAmount, grpReason, uriPathsVecSize);

    int groupStartPos = 0;
    int groupEndPos;
    for (int thread = 0; thread < groupAmount; thread++) {
        groupEndPos = groupSize * (thread + 1);
        std::string perTimerLoopName = "AyonApi::batchResolvePath Thread Loop: " + std::to_string(thread);
        PerfTimer(perTimerLoopName.c_str());

        // check if we are to close to the end and extend the group to catch all the data and end the loop

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
AyonApi::getAssetIdent(const nlohmann::json &uriResolverRespone) {
    PerfTimer("AyonApi::getAssetIdent");
    m_Log->info(m_Log->key("AyonApi"), "AyonApi::getAssetIdent({})", uriResolverRespone.dump());

    std::pair<std::string, std::string> AssetIdent;
    if (uriResolverRespone.empty()) {
        return AssetIdent;
    }
    try {
        AssetIdent.first = uriResolverRespone.at("uri");
        if (uriResolverRespone.at("entities").size() > 1) {
            m_Log->warn("Uri reselution returned more than one path (%s)", uriResolverRespone.at("entities").dump());
        }
        AssetIdent.second = rootReplace(
            uriResolverRespone.at("entities").at(uriResolverRespone.at("entities").size() - 1).at("filePath"));
    }
    catch (const nlohmann::json::exception &e) {
        m_Log->warn("asset identification cant be generated {}", uriResolverRespone.dump());
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
                        const int &sucsessStatus) {
    PerfTimer("AyonApi::serialCorePost");
    m_Log->info(m_Log->key("AyonApi"), "AyonApi::serialCorePost() endPoint: {}, Payload: {}, sucsessStatus: {}",
                endPoint, Payload, sucsessStatus);

    httplib::Result response;
    int responeStatus;
    uint8_t retryes = 0;
    while (retryes <= m_maxCallRetrys) {
        try {
            response = m_AyonServer->Post(endPoint, headers, Payload, "application/json");
            responeStatus = response->status;
            retryes++;

            if (responeStatus == sucsessStatus) {
                return response->body;
            }
            else {
                m_Log->info("AyonApi::serialCorePost wrong status code: {} expected: {}", responeStatus, sucsessStatus);
                if (responeStatus == 401) {
                    m_Log->warn("not logged in 401 ");
                    return "";
                }
                if (responeStatus == 500) {
                    m_Log->warn("internal server error ");
                    return "";
                }
                std::this_thread::sleep_for(std::chrono::milliseconds(
                    responeStatus == m_ServerBusyCode ? m_RequestDelayWhenServerBusy : m_retryWaight));
            }
        }   // TODO error reason not printed
        catch (const httplib::Error &e) {
            m_Log->warn("Request Failed because: {}");
            break;
        }
        m_Log->warn("The connection failed Rety now.");
    }
    return "";
};

std::string
AyonApi::GenerativeCorePost(const std::string &endPoint,
                            httplib::Headers headers,
                            std::string &Payload,
                            const int &sucsessStatus) {
    PerfTimer("AyonApi::GenerativeCorePost");
    m_Log->info(m_Log->key("AyonApi"), "AyonApi::GenerativeCorePost() endPoint: {}, Payload: {}, sucsessStatus: {}",
                endPoint, Payload, sucsessStatus);

    httplib::Client AyonServerClient(m_serverUrl);
    AyonServerClient.set_bearer_token_auth(m_authKey);
    AyonServerClient.set_connection_timeout(m_connectionTimeOutMax);
    AyonServerClient.set_read_timeout(m_readTimeOutMax);

    httplib::Result response;
    int responeStatus;
    uint8_t retryes = 0;
    bool ffoLocking = false;
    uint16_t loopIertaion = 0;
    while (retryes <= m_maxCallRetrys || m_GenerativeCorePostMaxLoopIterations > loopIertaion) {
        loopIertaion++;
        m_Log->info("AyonApi::GenerativeCorePost while loop thread {} iteration {}",
                    std::hash<std::thread::id>{}(std::this_thread::get_id()), loopIertaion);

        if (ffoLocking) {
            m_ConcurentRequestAfterffoMutex.lock();
            m_Log->info("AyonApi::GenerativeCorePost ffoLocking enabled");
            if (m_maxConcurentRequestAfterffo >= 1) {
                m_maxConcurentRequestAfterffo--;

                m_Log->info("AyonApi::GenerativeCorePost thread pool open available: {}",
                            m_maxConcurentRequestAfterffo);

                m_ConcurentRequestAfterffoMutex.unlock();
            }
            else {
                m_Log->info("AyonApi::GenerativeCorePost Thread pool closed");

                m_ConcurentRequestAfterffoMutex.unlock();
                std::this_thread::sleep_for(std::chrono::milliseconds(800));
                continue;
            }
        }

        m_Log->info("AyonApi::GenerativeCorePost sending request");

        try {
            response = AyonServerClient.Post(endPoint, headers, Payload, "application/json");
            responeStatus = response->status;
            retryes++;
            if (ffoLocking) {
                m_ConcurentRequestAfterffoMutex.lock();
                m_maxConcurentRequestAfterffo++;
                m_ConcurentRequestAfterffoMutex.unlock();
            }
            if (responeStatus == sucsessStatus) {
                m_Log->info("AyonApi::GenerativeCorePost The request worked, unlocking and returning. ");

                return response->body;
            }
            else {
                if (responeStatus == m_ServerBusyCode) {
                    m_Log->warn("AyonApi::GenerativeCorePost The server responded with 503");

                    retryes = 0;
                    ffoLocking = true;
                    continue;
                }
                if (responeStatus == 401) {
                    m_Log->warn("not logged in 401 ");
                    return "";
                }
                if (responeStatus == 500) {
                    m_Log->warn("internal server error ");
                    return "";
                }
                m_Log->info("AyonApi::GenerativeCorePost wrong status code: {} expected: {} retrying", responeStatus,
                            sucsessStatus);
                std::this_thread::sleep_for(std::chrono::milliseconds(m_retryWaight));
                continue;
            }
        }   // TODO error reason not printed
        catch (const httplib::Error &e) {
            m_Log->warn("AyonApi::GenerativeCorePost Request Failed because: {}");
            break;
        }
    }

    m_Log->warn(
        "AyonApi::GenerativeCorePost Too many resolve retries without the correct response code  for: {}, on: {}",
        Payload, endPoint);
    return "";
};

std::string
AyonApi::convertUriVecToString(const std::vector<std::string> &uriVec) {
    PerfTimer("AyonApi::convertUriVecToString");
    m_Log->info(m_Log->key("AyonApi"), "AyonApi::convertUriVecToString({})",
                std::accumulate(uriVec.begin(), uriVec.end(), std::string()));

    std::string payload = R"({{"resolveRoots": true,"uris": [)";

    for (int i = 0; i <= int(uriVec.size()); i++) {
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
