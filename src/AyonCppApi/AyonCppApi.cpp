
#include "AyonCppApi.h"
#include "httplib.h"
#include "nlohmann/json.hpp"
#include "nlohmann/json_fwd.hpp"

#include <algorithm>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <memory>
#include <ostream>
#include <stdexcept>
#include <string>
#include <filesystem>
#include <vector>

AyonApi::AyonApi() {
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
    authKey = std::getenv("AYON_API_KEY");
    serverUrl = std::getenv("AYON_SERVER_URL");

    if (authKey == nullptr || serverUrl == nullptr) {
        Log->warn(
            "One ore More Environment Varibles Could not be Loaded  AYON_API_KEY: {}, AYON_SERVER_URL: {}, "
            "AYON_SITE_ID: {}",
            authKey, serverUrl);
        return false;
    }

    Log->info(
        "All Environment Varibles have been loaded AYON_API_KEY: {}, AYON_SERVER_URL: {}, "
        "AYON_SITE_ID: {}",
        authKey, serverUrl);

    const char* siteIdEnv = getenv("AYON_SITE_ID");

    if (siteIdEnv == nullptr) {
        Log->info("SiteID is not available as an ENV varible");
        std::ifstream siteIdFile(ayonAppData + "/site_id");
        if (siteIdFile.is_open()) {
            std::getline(siteIdFile, siteId);
            siteIdFile.close();
        }
        else {
            Log->info("wasnt able to get site_id file can reat in siteId");
        }
    }
    else {
        siteId = siteIdEnv;
    }

    return true;
};

nlohmann::json
AyonApi::GET(const std::string &endPoint) {
    nlohmann::json jsonRespne = nlohmann::json::parse(AyonServer->Get(endPoint)->body);

    return jsonRespne;
};

nlohmann::json
AyonApi::POST(const std::string &endPoint, httplib::Headers headers, nlohmann::json &jsonPayload) {
    std::string payload = jsonPayload.dump();

    std::string test = R"({
  "resolveRoots": true,
  "uris": [
    "ayon://Usd_Base/Assets/lib_Caracter/Hero_01?product=usdTest&version=v001&representation=usd",
        "ayon://Usd_Base/Assets/lib_Caracter/Hero_01?product=usdTest&version=v001&representation=usd"
  ]
})";

    // TODO this function fails after about 34000 paths ( batch request )
    nlohmann::json jsonRespne
        = nlohmann::json::parse(AyonServer->Post(endPoint, headers, test, "application/json")->body);

    return jsonRespne;
};

std::string
AyonApi::resolvePath(const std::string &uriPath) {
    nlohmann::json jsonPayload = {{"resolveRoots", true}, {"uris", nlohmann::json::array({uriPath})}};
    httplib::Headers headers = {{"X-ayon-site-id", siteId}};

    std::string resolvedPath = POST("/api/resolve", headers, jsonPayload)[0]["entities"][0]["filePath"];
    return resolvedPath;
};

std::vector<std::string>
AyonApi::batchResolvePath(const std::vector<std::string> &uriPaths) {
    // TODO build a system that creates the array via string manipulation instead off json
    //   std::string result;
    //    result.reserve(vec.size() * 3);
    //
    //    std::string payload = R"({{"resolveRoots": true,"uris": [)";
    //    payload += "]}";

    nlohmann::json uriPathsJsonArray;

    std::move(uriPaths.begin(), uriPaths.end(), std::back_inserter(uriPathsJsonArray));
    // TODO the json payload will be converted to a string so it schould be posible to just do this here
    nlohmann::json jsonPayload = {{"resolveRoots", true}, {"uris", uriPathsJsonArray}};
    httplib::Headers headers = {{"X-ayon-site-id", siteId}};

    nlohmann::json response = POST("/api/resolve", headers, jsonPayload);
    std::vector<std::string> resolvedPath;

    for (const auto &entry: response) {
        for (const auto &entity: entry["entities"]) {
            resolvedPath.push_back(entity["filePath"]);
        }
    }
    return resolvedPath;
};

std::string
AyonApi::getKey() {
    return authKey;
};

std::string
AyonApi::getUrl() {
    return serverUrl;
}
