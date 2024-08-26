#include "gtest/gtest.h"
#include <AyonCppApi.h>
#include <iostream>
#include <string>
#include "Instrumentor.h"
#include "nlohmann/json.hpp"
#include "nlohmann/json_fwd.hpp"
#include "AyonCppApiTestsMain.h"

nlohmann::json JsonFile;

TEST(AyonCppApi, AyonCppApiCreaion) {
    AyonApi Test = AyonApi();
}

TEST(AyonCppApi, AyonCppApiSerialResolveRootReplace) {
    Instrumentor::Get().BeginSession("Profile", "bin/profSerial.json");
    AyonApi Api = AyonApi();
    nlohmann::json JsonFileStage = JsonFile["Resolve"];
    bool RunOnlyOneResolveIteration = false;
    bool printResult = true;

    if (!AyonCppApiTest::test_SimpleResolve(JsonFile, RunOnlyOneResolveIteration, printResult, Api)) {
        FAIL();
    }

    Instrumentor::Get().EndSession();
    std::cout << std::endl;
}

TEST(AyonCppApi, AyonCppApiBathResolveRootReplace) {
    Instrumentor::Get().BeginSession("Profile", "bin/profBatch.json");
    AyonApi Api = AyonApi();
    nlohmann::json JsonFileStage = JsonFile["Resolve"];
    bool RunOnlyOneResolveIteration = false;
    bool printResult = true;

    if (!AyonCppApiTest::test_BatchResolve(JsonFile, printResult, Api)) {
        FAIL();
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
