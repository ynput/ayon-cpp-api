#include "AyonCppApi.h"
#include <string>
#include <unordered_map>
#include <utility>
#include "nlohmann/json.hpp"
#include "nlohmann/json_fwd.hpp"
#include "AyonCppApiTestsMain.h"

bool
AyonCppApiTest::test_SimpleResolve(nlohmann::json &JsonFile, const bool &RunOnlyOnce, const bool &Print, AyonApi &Api) {
    nlohmann::json jsonFileStage = JsonFile["Resolve"];

    for (const auto& item : jsonFileStage.items()) {
        std::pair<std::string, std::string> test = Api.resolvePath(item.key());

        if (test.second != item.value()["RootResolved"]) {
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
    nlohmann::json jsonFileStage = JsonFile["Resolve"];
    std::vector<std::string> uriListSource;

    for (const auto& item : jsonFileStage.items()) {
        uriListSource.push_back(item.key());
    }

    std::unordered_map<std::string, std::string> test = Api.batchResolvePath(uriListSource);

    for (const auto& element : test) {
        if (Print) {
            std::cout << "BatchTest Run Result: " << element.first << " / " << element.second;
        }
        if (jsonFileStage.find(element.first) != jsonFileStage.end()) {
            if (element.second != jsonFileStage[element.first]["RootResolved"]) {
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
