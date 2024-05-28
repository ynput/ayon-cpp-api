#include "AyonCppApi.h"
#include <string>
#include <unordered_map>
#include <utility>
#include "nlohmann/json.hpp"
#include "nlohmann/json_fwd.hpp"
#include "AyonCppApiTestsMain.h"

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
