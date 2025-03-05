#include "AyonCppApi.h"
#include <string>
#include <unordered_map>
#include <utility>
#include "nlohmann/json.hpp"
#include "nlohmann/json_fwd.hpp"
#include "AyonCppApiTestsMain.h"


bool
AyonCppApiTest::load_EnvVariables(std::string &envFilePath,
                                  std::string &AYON_API_KEY,
                                  std::string &AYON_SERVER_URL,
                                  std::string &AYON_SITE_ID,
                                  std::string &AYON_PROJECT_NAME,
                                  std::string &AYONLOGGERLOGLVL,
                                  std::string &AYONLOGGERFILELOGGING) {
    std::ifstream envFile(envFilePath);
    if (!envFile.is_open()) {
        std::cerr << "Failed to open .env file: " << envFilePath << std::endl;
        return false;
    }

    std::string line;
    while (std::getline(envFile, line)) {
        std::istringstream iss(line);
        std::string key, value;
        if (std::getline(iss, key, '=') && std::getline(iss, value)) {
            if (key == "AYON_API_KEY") {
                AYON_API_KEY = value;
            } else if (key == "AYON_SERVER_URL") {
                AYON_SERVER_URL = value;
            } else if (key == "AYON_SITE_ID") {
                AYON_SITE_ID = value;
            } else if (key == "AYON_PROJECT_NAME") {
                AYON_PROJECT_NAME = value;
            } else if (key == "AYONLOGGERLOGLVL") {
                AYONLOGGERLOGLVL = value;
            } else if (key == "AYONLOGGERFILELOGGING") {
                AYONLOGGERFILELOGGING = value;
            }
        }
    }

    envFile.close();
    return true;
}

bool
AyonCppApiTest::test_SimpleResolve(nlohmann::json &JsonFile, const bool &RunOnlyOnce, const bool &Print, AyonApi &Api) {
    nlohmann::json JsonFileStage = JsonFile["Resolve"];

    for (auto it = JsonFileStage.begin(); it != JsonFileStage.end(); it++) {
        std::pair<std::string, std::string> test = Api.resolvePath(it.key());

        if (test.second != JsonFileStage[it.key()]["RootResolved"]) {
            return false;
        }
        if (Print) {
            std::cout << "SerialTest Run Result: " << test.first << " / " << test.second << std::endl;
        }
        if (RunOnlyOnce) {
            break;
        }
    }
    return true;
}

bool
AyonCppApiTest::test_BatchResolve(nlohmann::json &JsonFile, const bool &Print, AyonApi &Api) {
    nlohmann::json JsonFileStage = JsonFile["Resolve"];
    std::vector<std::string> uriListSource;

    for (auto it = JsonFileStage.begin(); it != JsonFileStage.end(); it++) {
        uriListSource.push_back(it.key());
    }

    std::unordered_map<std::string, std::string> test = Api.batchResolvePath(uriListSource);

    for (std::pair<std::string, std::string> element: test) {
        if (Print) {
            std::cout << "BatchTest Run Result: " << element.first << " / " << element.second;
        }
        if (JsonFileStage.find(element.first) != JsonFileStage.end()) {
            if (element.second != JsonFileStage[element.first]["RootResolved"]) {
                return false;
            }
        }
        else {
            return false;
        }
    }
    if (Print) {
        std::cout << std::endl;
    }

    return true;
}
