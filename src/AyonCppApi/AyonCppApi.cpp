
#include "AyonCppApi.h"
#include <netinet/in.h>
#include <sys/types.h>
#include "httplib.h"
#include "nlohmann/json.hpp"
#include "nlohmann/json_fwd.hpp"
#include "Instrumentor.h"
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
#include <iterator>
#include <memory>
#include <ostream>
#include <stdexcept>
#include <string>
#include <filesystem>
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
AyonApi::SPOST(const std::string &endPoint, httplib::Headers &headers, nlohmann::json &jsonPayload) {
    PerfTimer("AyonApi::SPOST");
    std::string payload = jsonPayload.dump();

    nlohmann::json jsonRespne = nlohmann::json::parse(serialCorePost(endPoint, headers, payload));

    return jsonRespne;
};

nlohmann::json
AyonApi::CPOST(const std::string &endPoint, httplib::Headers &headers, nlohmann::json jsonPayload) {
    PerfTimer("AyonApi::CPOST");
    // std::cout << "Cpost payload get: " << jsonPayload << std::endl;
    std::string payload = jsonPayload.dump();
    // std::cout << "dumped payload: " << payload << std::endl;
    nlohmann::json jsonRespne = nlohmann::json::parse(GenerativeCorePost(endPoint, headers, payload));
    // std::cout << "resp: " << jsonRespne << std::endl;
    return jsonRespne;
};

std::string
AyonApi::resolvePath(const std::string &uriPath) {
    PerfTimer("AyonApi::resolvePath");
    nlohmann::json jsonPayload = {{"resolveRoots", true}, {"uris", nlohmann::json::array({uriPath})}};
    httplib::Headers headers = {{"X-ayon-site-id", siteId}};

    std::string resolvedPath = SPOST("/api/resolve", headers, jsonPayload)[0]["entities"][0]["filePath"];
    return resolvedPath;
};

std::unordered_map<std::string, std::string>
AyonApi::batchResolvePath(const std::vector<std::string> &uriPaths) {
    PerfTimer("AyonApi::batchResolvePath");

    // std::vector<std::string> resolvedPaths;
    httplib::Headers headers = {{"X-ayon-site-id", siteId}};
    std::string ayonEndpoint = "/api/resolve";

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
                groupAmount = std::floor(static_cast<double>(uriPathsVecSize) / groupSize);
                ;   // TODO explicit rounding .x group amount
                grpReason = "5+ -500 build group amount by size";
            }
            else {
                // the groups are to beig
                // we have to generate more groups than we have threads

                groupAmount = std::floor(static_cast<double>(uriPathsVecSize) / regroupSizeForAsyncRequests);
                grpReason = "the groups are to big we will build more than we have threads ";
            }
        }
        else {
            // the groups are to small so we build groups by size
            // group size is 10

            groupSize = regroupSizeForAsyncRequests;
            groupAmount = std::floor(static_cast<double>(uriPathsVecSize) / groupSize);
            grpReason = "Groups are to small we will build them by size";
        }
    }

    // std::cout << "Group Reason  : " << grpReason << std::endl;
    // std::cout << "Ayailable Threads  : " << num_threads << std::endl;
    // std::cout << "Uri Vec size  : " << uriPathsVecSize << std::endl;
    // std::cout << "group amount : " << groupAmount << std::endl;
    // std::cout << "group Size : " << groupSize << std::endl;

    std::vector<std::future<nlohmann::json>> futures;
    int groupStartPos = 0;
    int groupEndPos;
    for (int thread = 0; thread < groupAmount; thread++) {
        // std::cout << std::endl;
        groupEndPos = groupSize * (thread + 1);

        if (uriPathsVecSize - groupEndPos < regroupSizeForAsyncRequests) {
            groupEndPos = uriPathsVecSize - 1;
        }

        // std::cout << "threadId: " << thread << " start end grp pos: " << groupStartPos << " " << groupEndPos
        //           << std::endl;
        // std::cout << "vec1val : " << uriPaths[groupStartPos] << " vec2Val : " << uriPaths[groupEndPos] << std::endl;
        nlohmann::json threadLocalUriPathsJsonArray;

        for (size_t i = groupStartPos; i <= groupEndPos; ++i) {
            threadLocalUriPathsJsonArray.push_back(uriPaths[i]);
        }
        nlohmann::json threadLocalJsonPayload = {{"resolveRoots", true}, {"uris", threadLocalUriPathsJsonArray}};

        // TODO CPOST needs an option to take in an expected response http code so that it can rerun if the code has not
        // been hit
        futures.push_back(std::async(std::launch::async, &AyonApi::CPOST, this, std::ref(ayonEndpoint),
                                     std::ref(headers), threadLocalJsonPayload));
        if (thread >= 10) {
            std::this_thread::sleep_for(std::chrono::milliseconds(groupSize + 10));
        }

        groupStartPos = groupEndPos + 1;
    }

    // for (int i = 0; i < num_threads; ++i) {
    //     // Launch a new thread and store its future in the vector
    //     futures.push_back(std::async(std::launch::async, &AyonApi::CPOST, this, std::ref(ayonEndpoint),
    //                                  std::ref(headers), std::ref(jsonPayload)));
    // }
    // std::cout << std::endl;
    std::vector<nlohmann::json> jsonResponse;
    for (auto &future: futures) {
        jsonResponse.push_back(future.get());
        // std::cout << "response" << future.get() << std::endl;
    }

    std::unordered_map<std::string, std::string> assetIdentGrp;

    for (const auto &response: jsonResponse) {
        for (const auto &assetRaw: response) {
            // std::cout << assetRaw["entities"][0]["filePath"] << std::endl;

            assetIdentGrp.emplace(getAssetIdent(assetRaw));
        }
    }
    // for (const auto &pair: assetIdentGrp) {
    //     std::cout << "Key: " << pair.first << ", Value: " << pair.second << std::endl;
    // }
    //--------- Get File paths from response and convert them into an vector
    // for (const auto &response: jsonResponse) {
    //     for (const auto &entry: response) {
    //         for (const auto &options: entry["entities"]) {
    //             resolvedPaths.push_back(
    //                 options["filePath"]);   // TODO this will return all the version if =* on versions
    //             // std::cout << options["filePath"] << std::endl;
    //         }
    //     }
    // }

    // std::cout << jsonResponse[0][0]["entities"][0]["filePath"] << std::endl;

    // resolvedPaths.emplace_back("this");
    // std::cout << "func end \n" << std::endl;
    return assetIdentGrp;
};

std::pair<std::string, std::string>
AyonApi::getAssetIdent(const nlohmann::json &uriResolverRespone) {
    std::pair<std::string, std::string> AssetIdent;

    try {
        for (const auto &versions: uriResolverRespone["entities"]) {
            AssetIdent.second = versions["filePath"];
        }
        AssetIdent.first = uriResolverRespone["uri"];
    }
    catch (nlohmann::json_abi_v3_11_3::detail::type_error) {
        Log->warn("asset identification cant be generated ");
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
AyonApi::serialCorePost(const std::string &endPoint, httplib::Headers headers, std::string &Payload) {
    PerfTimer("AyonApi::serialCorePost");

    std::string respone = AyonServer->Post(endPoint, headers, Payload, "application/json")->body;

    return respone;
};

std::string
AyonApi::GenerativeCorePost(const std::string &endPoint,
                            httplib::Headers headers,
                            std::string &Payload,
                            int &sucsessStatus) {
    PerfTimer("AyonApi::GenerativeCorePost");

    httplib::Client AyonServerClient(serverUrl);
    AyonServerClient.set_bearer_token_auth(authKey);
    httplib::Result response;
    int responeStatus;
    uint8_t retryes;

    while (responeStatus != sucsessStatus || retryes >= maxCallRetrys) {
        response = AyonServerClient.Post(endPoint, headers, Payload, "application/json");
        responeStatus = response->status;
    }
    if (responeStatus == sucsessStatus) {
        return response->body;
    }
    Log->warn("to manny resolve retryes without correct response code ");
    return "";
};
// TODO multi thread this because why not
std::string
AyonApi::convertUriVecToString(const std::vector<std::string> &uriVec) {
    PerfTimer("AyonApi::convertUriVecToString");
    std::string payload = R"({{"resolveRoots": true,"uris": [)";

    for (int i; i <= uriVec.size(); i++) {
        payload += uriVec[i];
    }

    payload += "]}";

    return payload;
};
// TODO implement
std::string
AyonApi::getFileVersion(const nlohmann::json &jsonData, u_int16_t versionNum = 65535) {
    PerfTimer("AyonApi::getFileVersion");
    return "no";
};

// TODO implement
std::vector<nlohmann::json>
AyonApi::splitBatchResolveResponse(const nlohmann::json &jsonDataGrp) {
    PerfTimer("AyonApi::splitBatchResolveResponse");
    std::vector<nlohmann::json> seperatedData;

    return seperatedData;
};
