#pragma once

#include <memory>
#include <string>
#include <vector>
#include "AyonLogger.h"
#include "appDataFoulder.h"
#include "httplib.h"
#include "nlohmann/json_fwd.hpp"

class AyonApi {
    public:
        AyonApi();
        ~AyonApi();

        std::string getKey();
        std::string getUrl();

        nlohmann::json GET(const std::string &endPoint);
        nlohmann::json POST(const std::string &endPoint, httplib::Headers headers, nlohmann::json &jsonPayload);

        std::string resolvePath(const std::string &uriPath);
        std::vector<std::string> batchResolvePath(const std::vector<std::string> &uriPaths);

        bool loadEnvVars();

    private:
        // ----- Env Varibles
        const char* authKey;
        const char* serverUrl;

        std::string ayonAppData = getAppDataDir() + "/AYON";

        // ---- Server Vars
        std::string siteId;
        std::string userName;

        // --- Runtime Dep Vars
        std::unique_ptr<httplib::Client> AyonServer;

        std::shared_ptr<AyonLogger> Log;
};
