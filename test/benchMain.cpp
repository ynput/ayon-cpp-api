#include "gtest/gtest.h"
#include <AyonCppApi.h>
#include <iostream>
#include <string>
#include <utility>
#include "Instrumentor.h"
#include "nlohmann/json.hpp"
#include "nlohmann/json_fwd.hpp"

nlohmann::json JsonFile;

TEST(AyonCppApi, AyonCppApiCreaion) {
    AyonApi Test = AyonApi();
}

TEST(AyonCppApi, AyonCppApiSerialResolveNoRootReplace) {
    Instrumentor::Get().BeginSession("Profile",
                                     "/home/lyonh/ynput/DevEnv/projects/AyonCppApi/ayon-cpp-api/bin/prof.json");
    AyonApi Api = AyonApi();
    nlohmann::json JsonFileStage = JsonFile["Resolve"];

    std::cout << "AyonCppApiSerialResolveNoRootReplace test: ";
    for (auto it = JsonFileStage.begin(); it != JsonFileStage.end(); it++) {
        std::pair<std::string, std::string> test = Api.resolvePath(it.key());
        if (test.second != JsonFileStage[it.key()]["NoRootResolve"]) {
            FAIL();
        }
        std::cout << test.first << " / " << test.second;
    }

    Instrumentor::Get().EndSession();
    std::cout << std::endl;
}

int
main(int argc, char** argv) {
    std::ifstream file("test/testData.json");
    if (!file.is_open()) {
        std::cerr << "Failed to open file!" << std::endl;
    }
    JsonFile = nlohmann::json::parse(file);
    file.close();

    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
