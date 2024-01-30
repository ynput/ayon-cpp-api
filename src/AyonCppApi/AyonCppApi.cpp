
#include "AyonCppApi.h"
#include "httplib.h"
#include "nlohmann/json.hpp"

#include <cstdlib>
#include <string>

using json = nlohmann::json;

AyonApi::AyonApi() {
    // authKey = std::getenv("AYON_API_KEY");
    // serverUrl = std::getenv("AYON_SERVER_URL");
    // siteId = std::getenv("AYON_SITE_ID");
    httplib::Client cli("http://localhost:5000");

    // Set the bearer token
    cli.set_bearer_token_auth("f04a1fb3d4c7b47c7f419c9bbeaca3aa336860e6ead2ead7c638071ccb89df01");

    // JSON payload
    std::string json_payload = R"({
        "resolveRoots": false,
        "uris": [ 
            "ayon://Usd_Base/Assets/lib_Caracter/Hero_01?product=usdTest&version=v001&representation=*"
        ]
    })";

    // Make POST request
    auto res = cli.Post("/api/resolve", json_payload, "application/json");

    json j = json::parse(res->body);
    std::string filePath = j[0]["entities"][0]["filePath"];

    std::cout << "File Path : " << filePath << std::endl;
};
AyonApi::~AyonApi(){};

std::string
AyonApi::getKey() {
    return authKey;
};

std::string
AyonApi::getUrl() {
    return serverUrl;
}

std::string
AyonApi::getSiteId() {
    return siteId;
}
