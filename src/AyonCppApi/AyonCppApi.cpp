
#include "AyonCppApi.h"
#include <netinet/in.h>
#include <sys/types.h>
#include "httplib.h"
#include "nlohmann/json.hpp"
#include "nlohmann/json_fwd.hpp"
#include "devMacros.h"
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
#include <regex>
#include <stdexcept>
#include <string>
#include <filesystem>
#include <string_view>
#include <thread>
#include <unordered_map>
#include <utility>
#include <vector>

AyonApi::AyonApi(): num_threads(std::thread::hardware_concurrency() / 2) {
    PerfTimer("AyonApi::AyonApi");

    // ----------- Init Logger
    std::filesystem::path log_File_path = std::filesystem::current_path() / "logFile.json";
    Log = std::make_shared<AyonLogger>(AyonLogger::getInstance(log_File_path));

    // ------------- Init Environment

    if (!loadEnvVars()) {
        throw std::invalid_argument(
            " Environment Could not be initialised are you shure your running this libary in an AYON environment");
    }

    AyonServer = std::make_unique<httplib::Client>(serverUrl);
    AyonServer->set_bearer_token_auth(authKey);

    // requestPool.availableRequestSlots.reserve(requestPool.defaultPoolSize);
    // std::fill(requestPool.availableRequestSlots.begin(), requestPool.availableRequestSlots.end(), true);
    //
    // requestPool.requestSlotMutex.reserve(requestPool.defaultPoolSize);
    // std::fill(requestPool.requestSlotMutex.begin(), requestPool.requestSlotMutex.end(), std::mutex());
    //
    // asyncThreadCreationSmallWaitTime = maxThreadsBeforeSmallWait * 50;

    // maxThreadsBeforeBigWait = num_threads;
    // asyncThreadCreationBigWaitTime = maxThreadsBeforeBigWait * 10;
};
AyonApi::~AyonApi(){};

bool
AyonApi::loadEnvVars() {
    PerfTimer("AyonApi::loadEnvVars");
    authKey = std::getenv("AYON_API_KEY");
    serverUrl = std::getenv("AYON_SERVER_URL");

    if (authKey == nullptr || serverUrl == nullptr) {
        Log->warn("One ore More Environment Varibles Could not be Loaded  AYON_API_KEY: , AYON_SERVER_URL:");
        return false;
    }

    Log->info("AYON_API_KEY and AYON_SERVER_URL have been loaded");

    const char* siteIdEnv = getenv("AYON_SITE_ID");

    if (siteIdEnv == nullptr) {
        Log->info("SiteID is not available as an ENV varible");
        std::ifstream siteIdFile(ayonAppData + "/site_id");
        if (siteIdFile.is_open()) {
            std::getline(siteIdFile, siteId);
            siteIdFile.close();
        }
        else {
            Log->info("wasnt able to get site_id file cant read siteId file");
        }
    }
    else {
        siteId = siteIdEnv;
    }
    Log->info("All Env varibles have been loaded");
    return true;
};

nlohmann::json
AyonApi::GET(const std::string &endPoint) {
    PerfTimer("AyonApi::GET");
    nlohmann::json jsonRespne = nlohmann::json::parse(AyonServer->Get(endPoint)->body);

    return jsonRespne;
};

nlohmann::json
AyonApi::SPOST(const std::shared_ptr<std::string> endPoint,
               const std::shared_ptr<httplib::Headers> headers,
               nlohmann::json jsonPayload,
               const std::shared_ptr<uint8_t> sucsessStatus) {
    PerfTimer("AyonApi::SPOST");
    nlohmann::json jsonRespne;

    if (jsonPayload.empty()) {
        Log->info("jsonPayload is empty no request created");
        return jsonRespne;
    }

    if (endPoint == nullptr || headers == nullptr || sucsessStatus == nullptr) {
        Log->error("One ore more of the provided pointers are null: endPoint, headers, sucsessStatus");

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
        Log->warn("SPOST cant phrease json // response empty");
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
    nlohmann::json jsonRespne;

    if (jsonPayload.empty()) {
        Log->info("jsonPayload is empty no request created");
        return jsonRespne;
    }

    if (endPoint == nullptr || headers == nullptr || sucsessStatus == nullptr) {
        Log->error("One ore more of the provided pointers are null: endPoint, headers, sucsessStatus");

        return jsonRespne;
    }

    std::string payload = jsonPayload.dump();
    std::string rawResponse = GenerativeCorePost(*endPoint, *headers, payload, *sucsessStatus);
    if (!rawResponse.empty()) {
        jsonRespne = nlohmann::json::parse(rawResponse);
    }
    else {
        Log->warn("CPOST cant phrease json // response empty");
    }
    return jsonRespne;
};
// TODO change the pointer work in here because the pointers consume more data that coping would
std::pair<std::string, std::string>
AyonApi::resolvePath(const std::string &uriPath) {
    PerfTimer("AyonApi::resolvePath");
    if (uriPath.empty()) {
        Log->info("path was empty: {}", uriPath.c_str());
        return {};
    }
    std::pair<std::string, std::string> resolvedAsset;
    nlohmann::json jsonPayload = {{"resolveRoots", true}, {"uris", nlohmann::json::array({uriPath})}};
    httplib::Headers headers = {{"X-ayon-site-id", siteId}};
    uint8_t sucsessStatus = 200;

    nlohmann::json response
        = SPOST(std::make_shared<std::string>(uriResolverEndpoint + uriResolverEndpointPathOnlyVar),
                std::make_shared<httplib::Headers>(headers), jsonPayload, std::make_shared<uint8_t>(sucsessStatus));

    resolvedAsset = getAssetIdent(response);

    // std::pair<std::string, std::string> resolvedPath = {"this", "this"};
    return resolvedAsset;
};

std::unordered_map<std::string, std::string>
AyonApi::batchResolvePath(std::vector<std::string> &uriPaths) {
    PerfTimer("AyonApi::batchResolvePath");

    // TODO build a thread function or a threadPool class that can take a data struct as a request and then it handles
    // the async work

    if (uriPaths.size() < 1) {
        Log->warn("AyonApi::batchResolvePath got empty vector stoped reselution");
        return {};
    }

    if (batchResolveOptimizeVector) {
        {
            PerfTimer("AyonApi::batchResolvePath::sanatizeVector");
            std::set<std::string> s;
            unsigned size = uriPaths.size();

            for (unsigned i = 0; i < size; ++i) s.insert(uriPaths[i]);
            uriPaths.assign(s.begin(), s.end());
            Log->info("Making shure that the vector has no duplicates vecSize before: {} after: {}", size,
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

    // set defaults for the grouping incase the vector is to small
    groupSize = uriPathsVecSize;
    groupAmount = 1;
    grpReason = "Vector is to small";

    // check what scaling the groups schould have
    if (uriPathsVecSize > minVecSizeForGroupSplitAsyncRequests) {
        // vector size is large eonught to build groups
        // double result = static_cast<double>(uriPathsVecSize) / num_threads;
        groupSize = std::ceil(static_cast<double>(uriPathsVecSize) / num_threads);
        if (groupSize > minGrpSizeForAsyncRequests) {
            // the group size is lagre enought to build groups from them
            if (groupSize < maxGroupSizeForAsyncRequests) {
                // now its bigger than 5 and smaller than 500
                // now we can just generate a group per thread and set the group amount
                groupSize = std::ceil(static_cast<double>(uriPathsVecSize) / num_threads);
                groupAmount = std::floor(static_cast<double>(uriPathsVecSize) / groupSize);

                // TODO explicit rounding .x group amount
                grpReason = "5+ -500 build group amount by size";
            }
            else {
                // the groups are to beig
                // we have to generate more groups than we have threads
                groupSize = regroupSizeForAsyncRequests;
                groupAmount = std::floor(static_cast<double>(uriPathsVecSize) / regroupSizeForAsyncRequests);
                grpReason = "the groups are to big we will build more than we have threads ";
            }
        }
        else {
            // the groups are to small so we build groups by size

            groupSize = std::min((int)regroupSizeForAsyncRequests, uriPathsVecSize);
            groupAmount = std::floor(static_cast<double>(uriPathsVecSize) / groupSize);
            grpReason = "Groups are to small we will build them by size";
        }
    }
    Log->info("AyonApi::batchResolvePath build groups with grpSize: {} grpAmount: {} grouingReason: {} vectorSize: {}",
              groupSize, groupAmount, grpReason, uriPathsVecSize);

    // TODO this works but its bad
    // evaluate what needs to be done to load balance the requests to the AYON server so that it dosnt crash
    // maxThreadsBeforeBigWait = maxThreadsBeforeSmallWait * 2.5;
    // asyncThreadCreationBigWaitTime = groupAmount * 2;

    int groupStartPos = 0;
    int groupEndPos;
    for (int thread = 0; thread < groupAmount; thread++) {
        groupEndPos = groupSize * (thread + 1);
        std::string perTimerLoopName = "AyonApi::batchResolvePath Thread Loop: " + std::to_string(thread);
        PerfTimer(perTimerLoopName.c_str());

        // check if we are to close to the end and extend the group to catch all the data and end the loop

        if (uriPathsVecSize - groupEndPos < groupSize + (groupSize / 2)) {
            Log->warn("the group with the threadId: {} is to close to the end. this group will be extended  ", thread);
            groupEndPos = uriPathsVecSize - 1;
            thread = groupAmount;
        }
        // TODO move the threadLocalUriPathsJsonArray generation onto the thread itself
        nlohmann::json threadLocalUriPathsJsonArray;

        for (int i = groupStartPos; i <= groupEndPos; i++) {
            threadLocalUriPathsJsonArray.push_back(uriPaths[i]);
        }
        nlohmann::json threadLocalJsonPayload = {{"resolveRoots", true}, {"uris", threadLocalUriPathsJsonArray}};

        futures.push_back(std::async(std::launch::async, &AyonApi::CPOST, this, std::ref(batchResolveEndpoint),
                                     std::ref(headers), std::move(threadLocalJsonPayload),
                                     std::ref(expectedResponseStatus)));
        // TODO reorder this because small sleep is more comman and can shadow the other if in order to only check if no
        // small sleep is done
        // if (enableThreadWaithing && thread != groupAmount) {
        //     // if ((thread + 1) % maxThreadsBeforeBigWait == 0 && enableBigBlockThreadWaithing) {
        //     //     std::this_thread::sleep_for(std::chrono::milliseconds(asyncThreadCreationBigWaitTime));
        //     // }
        //     // else {
        //     if ((thread + 1) % maxThreadsBeforeSmallWait == 0) {
        //         std::this_thread::sleep_for(std::chrono::milliseconds(groupSize + asyncThreadCreationSmallWaitTime));
        //     }
        //     // }
        // }
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
    std::pair<std::string, std::string> AssetIdent;
    try {
        uint16_t priorVersion;
        std::smatch match;
        for (const auto &versions: uriResolverRespone["entities"]) {
            std::string versionPath = versions["filePath"];

            if (std::regex_search(versionPath, match, regexVersionPattern)) {
                uint16_t version = std::stoi(match[1]);
                if (version > priorVersion) {
                    priorVersion = version;
                    AssetIdent.second = versions["filePath"];
                }
            }
            else {
                Log->info("there is at least one path in the entities thats missing a version number {}",
                          uriResolverRespone.dump());
            }
        }

        AssetIdent.first = uriResolverRespone["uri"];
    }
    catch (nlohmann::json_abi_v3_11_3::detail::type_error &e) {
        Log->warn("asset identification cant be generated {}", uriResolverRespone.dump());
    }
    return AssetIdent;
};

std::string
AyonApi::getKey() {
    PerfTimer("AyonApi::getKey");
    return authKey;
};

std::string
AyonApi::getUrl() {
    PerfTimer("AyonApi::getUrl");
    return serverUrl;
}

//--------------------- Internal Funcs
std::string
AyonApi::serialCorePost(const std::string &endPoint,
                        httplib::Headers headers,
                        std::string &Payload,
                        const int &sucsessStatus) {
    PerfTimer("AyonApi::serialCorePost");

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
                Log->info("AyonApi::GenerativeCorePost wrong status code: {} expected: {}", responeStatus,
                          sucsessStatus);
                std::this_thread::sleep_for(std::chrono::milliseconds(
                    responeStatus == ServerBusyCode ? RequestDelayWhenServerBusy : retryWaight));
            }
        }   // TODO error reason not printed
        catch (const httplib::Error &e) {
            Log->warn("Request Failed because: {}");
            break;
        }
        Log->warn("Connection failed Rety now");
    }
    return "";
};

std::string
AyonApi::GenerativeCorePost(const std::string &endPoint,
                            httplib::Headers headers,
                            std::string &Payload,
                            const int &sucsessStatus) {
    PerfTimer("AyonApi::GenerativeCorePost");
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
        Log->info("AyonApi::GenerativeCorePost while loop iteration {}", loopIertaion);
        if (allowRequest.try_lock()) {
            allowRequest.unlock();

            if (ffoLocking) {
                // 503 locking is enabled so we want to check if we can run a thread or if the thread pool is full
                // lock the mutex for the maxConcurentRequestAfterffo int for multithreading
                ConcurentRequestAfterffoMutex.lock();
                Log->info("AyonApi::GenerativeCorePost ffoLocking enabled");
                if (maxConcurentRequestAfterffo >= 1) {
                    // the thread pool is not full so we runn and we deduct 1 from the thread pool to block for this
                    // thread
                    maxConcurentRequestAfterffo--;

                    Log->info("AyonApi::GenerativeCorePost thread pool open available: {}",
                              maxConcurentRequestAfterffo);

                    ConcurentRequestAfterffoMutex.unlock();
                }
                else {
                    Log->info("AyonApi::GenerativeCorePost thread pool close");
                    // the thread pool has no space for this thread so we wait a bit and then we try again.

                    ConcurentRequestAfterffoMutex.unlock();
                    std::this_thread::sleep_for(std::chrono::milliseconds(800));
                    continue;
                }
                // unlock the mutex for the maxConcurentRequestAfterffo int for multithreading
            }

            Log->info("AyonApi::GenerativeCorePost Lock open");
            // ffo locking is off on this thread so this is the first time that we know about the ffo event
        }
        else {
            // we where unable to lock the allowRequest motex so on some thread an ffo event accuret
            // this means we want to set the ffoLocking to ture so that we limit the amount off requests
            ffoLocking = true;
            Log->info("AyonApi::GenerativeCorePost unable to get lock set ffoLocking to true and return");
            std::this_thread::sleep_for(std::chrono::milliseconds(800));
            continue;
        }

        // allowRequest.lock();
        // // server is available
        // allowRequest.unlock();
        Log->info("AyonApi::GenerativeCorePost server is available sending request");

        try {
            response = AyonServerClient.Post(endPoint, headers, Payload, "application/json");
            responeStatus = response->status;
            retryes++;

            if (responeStatus == sucsessStatus) {
                Log->info("AyonApi::GenerativeCorePost request worked unlocking and returning ");
                allowRequest.unlock();
                if (ffoLocking) {
                    // if ffoLocking was enabled for this thread then we have to add to the thread pool after we are
                    // finished so that the next thread can join

                    ConcurentRequestAfterffoMutex.lock();
                    maxConcurentRequestAfterffo++;
                    ConcurentRequestAfterffoMutex.unlock();
                }
                return response->body;
            }
            else {
                if (responeStatus == ServerBusyCode) {
                    Log->warn("AyonApi::GenerativeCorePost Server responded with 503");
                    // we hit a 503
                    // we add to the ReserverBusyResponses (++)
                    // at the top we check if server
                    retryes = 0;
                    if (allowRequest.try_lock()) {
                        // no one locked the requests

                        Log->info("AyonApi::GenerativeCorePost no one locked; locking");
                        allowRequest.lock();
                        Log->info("AyonApi::GenerativeCorePost locked");
                        continue;
                    }
                    else {
                        // some one allready locked the request we start at the beginning off the whiel again

                        Log->info("AyonApi::GenerativeCorePost  allready locked");
                        continue;
                    }
                }
                Log->info("AyonApi::GenerativeCorePost wrong status code: {} expected: {}", responeStatus,
                          sucsessStatus);
                std::this_thread::sleep_for(std::chrono::milliseconds(retryWaight));
            }
        }   // TODO error reason not printed
        catch (const httplib::Error &e) {
            Log->warn("AyonApi::GenerativeCorePost Request Failed because: {}");
            break;
        }
        Log->warn("AyonApi::GenerativeCorePost Connection failed Rety now");
    }

    Log->warn("AyonApi::GenerativeCorePost to manny resolve retryes without correct response code  for: {}, on: {}",
              Payload, endPoint);
    return "";
};

std::string
AyonApi::convertUriVecToString(const std::vector<std::string> &uriVec) {
    PerfTimer("AyonApi::convertUriVecToString");
    std::string payload = R"({{"resolveRoots": true,"uris": [)";

    for (int i = 0; i <= int(uriVec.size()); i++) {
        payload += uriVec[i];
    }

    payload += "]}";

    return payload;
};

// // TODO check if this needs to be implemented
// //  TODO implement
// std::string
// AyonApi::getFileVersion(const nlohmann::json &jsonData, u_int16_t versionNum = 65535) {
//     PerfTimer("AyonApi::getFileVersion");
//     return "no";
// };
//
// // TODO implement
// std::vector<nlohmann::json>
// AyonApi::splitBatchResolveResponse(const nlohmann::json &jsonDataGrp) {
//     PerfTimer("AyonApi::splitBatchResolveResponse");
//     std::vector<nlohmann::json> seperatedData;
//
//     return seperatedData;
// };
