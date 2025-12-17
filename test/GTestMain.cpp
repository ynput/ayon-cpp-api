#include "gtest/gtest.h"
#include <AyonCppApi.h>
#include <iostream>
#include <string>
#include "Instrumentor.h"
#include "nlohmann/json.hpp"
#include "AyonCppApiTestsMain.h"

nlohmann::json JsonFile;

AyonApi
getApiInstance() {
    std::string AYON_API_KEY("SuperSaveTestKey");
    std::string AYON_SERVER_URL("http://localhost:8003");
    std::string AYON_SITE_ID("TestId");
    std::string AYON_PROJECT_NAME("TestPrjName");
    std::string AYONLOGGERLOGLVL("CRITICAL");
    std::string AYONLOGGERFILELOGGING("OFF");

    return AyonApi("./test_logs", AYON_API_KEY, AYON_SERVER_URL, AYON_PROJECT_NAME, AYON_SITE_ID);
}

TEST(AyonCppApi, AyonCppApiCreation) {
    AyonApi Test = getApiInstance();
}

TEST(AyonCppApi, AyonCppApiSerialResolveRootReplace) {
    Instrumentor::Get().BeginSession("Profile", "bin/profSerial.json");
    AyonApi Api = getApiInstance();
    bool RunOnlyOneResolveIteration = false;
    bool printResult = false;

    if (!AyonCppApiTest::test_SimpleResolve(JsonFile, RunOnlyOneResolveIteration, printResult, Api)) {
        FAIL();
    }

    Instrumentor::Get().EndSession();
}

TEST(AyonCppApi, AyonCppApiBatchResolveRootReplace) {
    Instrumentor::Get().BeginSession("Profile", "bin/profBatch.json");
    AyonApi Api = getApiInstance();
    bool RunOnlyOneResolveIteration = false;
    bool printResult = false;

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
