
#include "AyonCppApi.h"
#include <sys/types.h>
#include "httplib.h"
#include "nlohmann/json.hpp"
#include "nlohmann/json_fwd.hpp"
#include "devMacros.h"
#include "AyonCppApiCrossPlatformMacros.h"
#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <fstream>
#include <functional>
#include <future>
#include <iostream>
#include <memory>
#include <mutex>
#include <numeric>
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

AyonApi::AyonApi(): num_threads(std::thread::hardware_concurrency() / 2) {
    PerfTimer("AyonApi::AyonApi");

    // ----------- Init Logger
    std::filesystem::path log_File_path = std::filesystem::current_path() / "logFile.json";
    Log = std::make_shared<AyonLogger>(AyonLogger::getInstance(log_File_path.string()));

    // ------------- Init Environment
    if (!loadEnvVars()) {
        throw std::invalid_argument(
            " Environment Could not be initialized are you sure your Running this library in an AYON Environment");
    }

    AyonServer = std::make_unique<httplib::Client>(serverUrl);
    AyonServer->set_bearer_token_auth(authKey);

    getSiteRoots();
};
AyonApi::~AyonApi(){};

bool
AyonApi::loadEnvVars() {
    PerfTimer("AyonApi::loadEnvVars");
    Log->info(Log->key("AyonApi"), "AyonApi::loadEnvVars()");

    authKey = std::getenv("AYON_API_KEY");
    serverUrl = std::getenv("AYON_SERVER_URL");

    if (authKey == nullptr) {
        Log->warn("AYON_API_KEY Env Variable could not be Found");
        return false;
    }
    if (serverUrl == nullptr) {
        Log->warn("AYON_SERVER_URL Env Variable could not be Found");
        return false;
    }

    Log->info("Loaded AYON_API_KEY and AYON_SERVER_URL");

    const char* siteIdEnv = std::getenv("AYON_SITE_ID");

    if (siteIdEnv == nullptr) {
        Log->info("SiteID is not Available as an ENV Variable");
        std::ifstream siteIdFile(ayonAppData + "/site_id");
        if (siteIdFile.is_open()) {
            std::getline(siteIdFile, siteId);
            siteIdFile.close();
        }
        else {
            Log->info("wasn't able to get site_id File");
        }
    }
    else {
        siteId = siteIdEnv;
    }

    Log->info("Found SideId");

    Log->info(Log->key("AyonApiDebugEnvVars"),
              "Loaded Environment Variables: AYON_API_KEY={}, AYON_SERVER_URL={}, AYON_SITE_ID={}", authKey, serverUrl,
              siteId);

    return true;
};

std::unordered_map<std::string, std::string>*
AyonApi::getSiteRoots() {
    Log->info(Log->key("AyonApi"), "AyonApi::getSiteRoots()");
    if (siteRoots.size() < 1) {
        httplib::Headers headers = {{"X-ayon-site-id", siteId}};
        nlohmann::json response = GET(std::make_shared<std::string>("/api/projects/Usd_Base/siteRoots"),
                                      std::make_shared<httplib::Headers>(headers), 200);
        siteRoots = response;
    }
    return &siteRoots;
};

std::string
AyonApi::rootReplace(const std::string &rootLessPath) {
    Log->info(Log->key("AyonApi"), "AyonApi::rootReplace({})", rootLessPath);
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
                std::string replacement = siteRoots.at(breakedString);
                rootedPath = std::regex_replace(rootLessPath, rootFindPattern, replacement);
                return rootedPath;
            }
            catch (std::out_of_range &e) {
                Log->warn("AyonApi::rootedPath error acured {}, list of Available root Replace str: ");
                for (auto &g: siteRoots) {
                    Log->warn("Key: {}, replacement: {}", g.first, g.second);
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
    Log->info(Log->key("AyonApi"), "AyonApi::GET({})", *endPoint);

    httplib::Result response;
    int responeStatus;
    uint8_t retryes = 0;
    while (retryes <= maxCallRetrys) {
        try {
            response = AyonServer->Get(*endPoint, *headers);
            responeStatus = response->status;
            retryes++;

            if (responeStatus == sucsessStatus) {
                return nlohmann::json::parse(response->body);
            }
            else {
                Log->info("AyonApi::serialCorePost wrong status code: {} expected: {}", responeStatus, sucsessStatus);
                if (responeStatus == 401) {
                    Log->warn("not logged in 401 ");
                    return nlohmann::json();
                }
                if (responeStatus == 500) {
                    Log->warn("internal server error ");
                    return nlohmann::json();
                }
                std::this_thread::sleep_for(std::chrono::milliseconds(
                    responeStatus == ServerBusyCode ? RequestDelayWhenServerBusy : retryWaight));
            }
        }   // TODO error reason not printed
        catch (const httplib::Error &e) {
            Log->warn("Request Failed because: {}");
            break;
        }
        Log->warn("The Connection Failed Rety now.");
    }
    return nlohmann::json();
};

nlohmann::json
AyonApi::SPOST(const std::shared_ptr<std::string> endPoint,
               const std::shared_ptr<httplib::Headers> headers,
               nlohmann::json jsonPayload,
               const std::shared_ptr<uint8_t> sucsessStatus) {
    PerfTimer("AyonApi::SPOST");
    Log->info(Log->key("AyonApi"), "AyonApi::SPOST endPoint: {}, jsonPayload: {}, sucsessStatus: {}", *endPoint,
              jsonPayload.dump(), *sucsessStatus);

    nlohmann::json jsonRespne;

    if (jsonPayload.empty()) {
        Log->info("JSON Payload is Empty. No Request Created");
        return jsonRespne;
    }

    if (endPoint == nullptr || headers == nullptr || sucsessStatus == nullptr) {
        Log->error("One or More of the Provided Pointers are null: endPoint, headers, sucsessStatus.");

        return jsonRespne;
    }

    AyonServerMutex.lock();

    std::string payload = jsonPayload.dump();
    std::string rawResponse = serialCorePost(*endPoint, *headers, payload, *sucsessStatus);

    if (!rawResponse.empty()) {
        jsonRespne = nlohmann::json::parse(rawResponse)[0];   // TODO figure out why this is isnt the same as CPOST and
                                                              // find a better way to make shure its not a array
    }
    else {
        Log->warn("SPOST cant phrase JSON // Response Empty");
    }
    AyonServerMutex.unlock();
    return jsonRespne;
};

nlohmann::json
AyonApi::CPOST(const std::shared_ptr<std::string> endPoint,
               const std::shared_ptr<httplib::Headers> headers,
               nlohmann::json jsonPayload,
               const std::shared_ptr<uint8_t> sucsessStatus) {
    PerfTimer("AyonApi::CPOST");
    Log->info(Log->key("AyonApi"), "AyonApi::CPOST endPoint: {}, jsonPayload: {}, sucsessStatus: {}", *endPoint,
              jsonPayload.dump(), *sucsessStatus);
    nlohmann::json jsonRespne;

    if (jsonPayload.empty()) {
        Log->info("JSON Payload is Empty. No Request Created");
        return jsonRespne;
    }

    if (endPoint == nullptr || headers == nullptr || sucsessStatus == nullptr) {
        Log->error("One or More of the Provided Pointers are null: endPoint, headers, sucsessStatus");

        return jsonRespne;
    }

    std::string payload = jsonPayload.dump();
    std::string rawResponse = GenerativeCorePost(*endPoint, *headers, payload, *sucsessStatus);
    if (!rawResponse.empty()) {
        jsonRespne = nlohmann::json::parse(rawResponse);
    }
    else {
        Log->warn("CPOST cant phrase JSON // Response Empty");
    }
    return jsonRespne;
};
// TODO change the pointer work in here because the pointers consume more data that coping would
std::pair<std::string, std::string>
AyonApi::resolvePath(const std::string &uriPath) {
    PerfTimer("AyonApi::resolvePath");
    Log->info(Log->key("AyonApi"), "AyonApi::resolvePath({})", uriPath);

    if (uriPath.empty()) {
        Log->info("Path was empty: {}", uriPath.c_str());
        return {};
    }
    std::pair<std::string, std::string> resolvedAsset;
    nlohmann::json jsonPayload = {{"resolveRoots", false}, {"uris", nlohmann::json::array({uriPath})}};
    httplib::Headers headers = {{"X-ayon-site-id", siteId}};
    uint8_t sucsessStatus = 200;

    nlohmann::json response
        = SPOST(std::make_shared<std::string>(uriResolverEndpoint + uriResolverEndpointPathOnlyVar),
                std::make_shared<httplib::Headers>(headers), jsonPayload, std::make_shared<uint8_t>(sucsessStatus));

    resolvedAsset = getAssetIdent(response);
    return resolvedAsset;
};

std::unordered_map<std::string, std::string>
AyonApi::batchResolvePath(std::vector<std::string> &uriPaths) {
    PerfTimer("AyonApi::batchResolvePath");
    Log->info(Log->key("AyonApi"), "AyonApi::batchResolvePath({})",
              std::accumulate(
                  uriPaths.begin(), uriPaths.end(), std::string(),
                  [](const std::string &a, const std::string &b) { return a + (a.length() > 0 ? " " : "") + b; }));

    if (uriPaths.size() < 1) {
        Log->warn("AyonApi::batchResolvePath Got Empty vector Stopped Resolution");
        return {};
    }

    if (batchResolveOptimizeVector) {
        {
            PerfTimer("AyonApi::batchResolvePath::sanatizeVector");
            std::set<std::string> s;
            unsigned size = uriPaths.size();

            for (unsigned i = 0; i < size; ++i) s.insert(uriPaths[i]);
            uriPaths.assign(s.begin(), s.end());
            Log->info("Make sure that the Vector has no Duplicates. vecSize before: {} after: {}", size,
                      uriPaths.size());
        }
    }
    std::unordered_map<std::string, std::string> assetIdentGrp;
    std::vector<std::future<nlohmann::json>> futures;

    std::shared_ptr<httplib::Headers> headers
        = std::make_shared<httplib::Headers>(httplib::Headers{{"X-ayon-site-id", siteId}});

    std::shared_ptr<std::string> batchResolveEndpoint;
    if (pathOnlyReselution) {
        batchResolveEndpoint
            = std::make_shared<std::string>(std::string_view(uriResolverEndpoint + uriResolverEndpointPathOnlyVar));
    }
    else {
        batchResolveEndpoint = std::make_shared<std::string>(uriResolverEndpoint);
    }

    std::shared_ptr<uint8_t> expectedResponseStatus = std::make_shared<uint8_t>(200);

    std::string grpReason;
    int uriPathsVecSize = uriPaths.size();
    int groupSize;
    int groupAmount;

    // set defaults for the grouping Increase the vector is to small
    groupSize = uriPathsVecSize;
    groupAmount = 1;
    grpReason = "The vector is too small.";

    // check what scaling the groups should have
    if (uriPathsVecSize > minVecSizeForGroupSplitAsyncRequests) {
        // vector size is large enough to build groups
        // double result = static_cast<double>(uriPathsVecSize) / num_threads;
        groupSize = std::ceil(static_cast<double>(uriPathsVecSize) / num_threads);
        if (groupSize > minGrpSizeForAsyncRequests) {
            // the group size is large enough to build groups from them
            if (groupSize < maxGroupSizeForAsyncRequests) {
                // now its bigger than 5 and smaller than 500
                // now we can just generate a group per thread and set the group amount
                groupSize = std::ceil(static_cast<double>(uriPathsVecSize) / num_threads);
                groupAmount = std::floor(static_cast<double>(uriPathsVecSize) / groupSize);

                // TODO explicit rounding .x group amount
                grpReason = "5> <500 build group amount by size";
            }
            else {
                // the groups are to big
                // we have to generate more groups than we have threads
                groupSize = regroupSizeForAsyncRequests;
                groupAmount = std::floor(static_cast<double>(uriPathsVecSize) / regroupSizeForAsyncRequests);
                grpReason = "The groups are too big. We will build More than we have CPU cores.";
            }
        }
        else {
            // the groups are to small so we build groups by size

            groupSize = std::min((int)regroupSizeForAsyncRequests, uriPathsVecSize);
            groupAmount = std::floor(static_cast<double>(uriPathsVecSize) / groupSize);
            grpReason = "If groups are too small, we will build them by size.";
        }
    }
    Log->info("AyonApi::batchResolvePath Build groups with grpSize: {} grpAmount: {} grouingReason: {} vectorSize: {}",
              groupSize, groupAmount, grpReason, uriPathsVecSize);

    int groupStartPos = 0;
    int groupEndPos;
    for (int thread = 0; thread < groupAmount; thread++) {
        groupEndPos = groupSize * (thread + 1);
        std::string perTimerLoopName = "AyonApi::batchResolvePath Thread Loop: " + std::to_string(thread);
        PerfTimer(perTimerLoopName.c_str());

        // check if we are to close to the end and extend the group to catch all the data and end the loop

        if (uriPathsVecSize - groupEndPos < groupSize + (groupSize / 2)) {
            Log->warn("the group with the threadId: {} It is too close to the end. This Group will be Extended. ",
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
    Log->info(Log->key("AyonApi"), "AyonApi::getAssetIdent({})", uriResolverRespone.dump());

    std::pair<std::string, std::string> AssetIdent;
    if (uriResolverRespone.empty()) {
        return AssetIdent;
    }
    try {
        AssetIdent.first = uriResolverRespone.at("uri");
        if (uriResolverRespone.at("entities").size() > 1) {
            Log->warn("Uri resolution Returned more than one path (%s)", uriResolverRespone.at("entities").dump());
        }
        AssetIdent.second = rootReplace(
            uriResolverRespone.at("entities").at(uriResolverRespone.at("entities").size() - 1).at("filePath"));
    }
    catch (const nlohmann::json::exception &e) {
        Log->warn("asset Identification cant be Generated {}", uriResolverRespone.dump());
    }
    return AssetIdent;
};

std::string
AyonApi::getKey() {
    PerfTimer("AyonApi::getKey");
    Log->info(Log->key("AyonApi"), "AyonApi::getKey");
    return authKey;
};

std::string
AyonApi::getUrl() {
    PerfTimer("AyonApi::getUrl");
    Log->info(Log->key("AyonApi"), "AyonApi::getUrl");
    return serverUrl;
}

//--------------------- Internal Funcs
std::string
AyonApi::serialCorePost(const std::string &endPoint,
                        httplib::Headers headers,
                        std::string &Payload,
                        const int &sucsessStatus) {
    PerfTimer("AyonApi::serialCorePost");
    Log->info(Log->key("AyonApi"), "AyonApi::serialCorePost() endPoint: {}, Payload: {}, sucsessStatus: {}", endPoint,
              Payload, sucsessStatus);

    httplib::Result response;
    int responeStatus;
    uint8_t retryes = 0;
    while (retryes <= maxCallRetrys) {
        try {
            response = AyonServer->Post(endPoint, headers, Payload, "application/json");
            responeStatus = response->status;
            retryes++;

            if (responeStatus == sucsessStatus) {
                return response->body;
            }
            else {
                Log->info("AyonApi::serialCorePost Wrong Status code: {} expected: {}", responeStatus, sucsessStatus);
                if (responeStatus == 401) {
                    Log->warn("not logged in 401 ");
                    return "";
                }
                if (responeStatus == 500) {
                    Log->warn("Internal Server Error ");
                    return "";
                }
                std::this_thread::sleep_for(std::chrono::milliseconds(
                    responeStatus == ServerBusyCode ? RequestDelayWhenServerBusy : retryWaight));
            }
        }   // TODO error reason not printed
        catch (const httplib::Error &e) {
            Log->warn("Request Failed Because: {}");
            break;
        }
        Log->warn("The Connection Failed Rety now.");
    }
    return "";
};

std::string
AyonApi::GenerativeCorePost(const std::string &endPoint,
                            httplib::Headers headers,
                            std::string &Payload,
                            const int &sucsessStatus) {
    PerfTimer("AyonApi::GenerativeCorePost");
    Log->info(Log->key("AyonApi"), "AyonApi::GenerativeCorePost() endPoint: {}, Payload: {}, sucsessStatus: {}",
              endPoint, Payload, sucsessStatus);

    httplib::Client AyonServerClient(serverUrl);
    AyonServerClient.set_bearer_token_auth(authKey);
    AyonServerClient.set_connection_timeout(connectionTimeOutMax);
    AyonServerClient.set_read_timeout(readTimeOutMax);

    httplib::Result response;
    int responeStatus;
    uint8_t retryes = 0;
    bool ffoLocking = false;
    uint16_t loopIertaion = 0;
    while (retryes <= maxCallRetrys || GenerativeCorePostMaxLoopIterations > loopIertaion) {
        loopIertaion++;
        Log->info("AyonApi::GenerativeCorePost while loop Thread {} Iteration {}",
                  std::hash<std::thread::id>{}(std::this_thread::get_id()), loopIertaion);

        if (ffoLocking) {
            ConcurentRequestAfterffoMutex.lock();
            Log->info("AyonApi::GenerativeCorePost ffoLocking Enabled");
            if (maxConcurentRequestAfterffo >= 1) {
                maxConcurentRequestAfterffo--;

                Log->info("AyonApi::GenerativeCorePost Thread pool open Available: {}", maxConcurentRequestAfterffo);

                ConcurentRequestAfterffoMutex.unlock();
            }
            else {
                Log->info("AyonApi::GenerativeCorePost Threat pool Closed");

                ConcurentRequestAfterffoMutex.unlock();
                std::this_thread::sleep_for(std::chrono::milliseconds(800));
                continue;
            }
        }

        Log->info("AyonApi::GenerativeCorePost Sending Request");

        try {
            response = AyonServerClient.Post(endPoint, headers, Payload, "application/json");
            responeStatus = response->status;
            retryes++;
            if (ffoLocking) {
                ConcurentRequestAfterffoMutex.lock();
                maxConcurentRequestAfterffo++;
                ConcurentRequestAfterffoMutex.unlock();
            }
            if (responeStatus == sucsessStatus) {
                Log->info("AyonApi::GenerativeCorePost The Request Worked, Unlocking and Returning. ");

                return response->body;
            }
            else {
                if (responeStatus == ServerBusyCode) {
                    Log->warn("AyonApi::GenerativeCorePost The server Responded with 503");

                    retryes = 0;
                    ffoLocking = true;
                    continue;
                }
                if (responeStatus == 401) {
                    Log->warn("not Logged in 401 ");
                    return "";
                }
                if (responeStatus == 500) {
                    Log->warn("Internal Server Error ");
                    return "";
                }
                Log->info("AyonApi::GenerativeCorePost wrong status code: {} expected: {} retrying", responeStatus,
                          sucsessStatus);
                std::this_thread::sleep_for(std::chrono::milliseconds(retryWaight));
                continue;
            }
        }   // TODO error reason not printed
        catch (const httplib::Error &e) {
            Log->warn("AyonApi::GenerativeCorePost Request Failed Because: {}");
            break;
        }
    }

    Log->warn("AyonApi::GenerativeCorePost Too many Resolve Retries Without the correct Response code  for: {}, on: {}",
              Payload, endPoint);
    return "";
};

std::string
AyonApi::convertUriVecToString(const std::vector<std::string> &uriVec) {
    PerfTimer("AyonApi::convertUriVecToString");
    Log->info(Log->key("AyonApi"), "AyonApi::convertUriVecToString({})",
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
    Log->info(Log->key("AyonApi"), "AyonApi::logPointer()");
    return Log;
};
