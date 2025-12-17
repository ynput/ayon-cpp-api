#include "gtest/gtest.h"
#include <AyonCppApi.h>
#include <iostream>
#include <string>
#include "Instrumentor.h"
#include "nlohmann/json.hpp"
#include "AyonCppApiTestsMain.h"

nlohmann::json JsonFile;

AyonApi getApiInstance() {
    std::string AYON_API_KEY;
    std::string AYON_SERVER_URL;
    std::string AYON_SITE_ID;
    std::string AYON_PROJECT_NAME;

    #ifdef _WIN32
    std::string envFilePath("test\\.env_http");
    #else
    std::string envFilePath("test/.env_http");
    #endif
    if (!AyonCppApiTest::load_EnvVariables(envFilePath, AYON_API_KEY, AYON_SERVER_URL, AYON_SITE_ID, AYON_PROJECT_NAME)) {
        std::cerr << "Failed to load environment variables!" << std::endl;
        throw std::runtime_error("Failed to load environment variables!");
    }

    return AyonApi("./test_logs", AYON_API_KEY, AYON_SERVER_URL, AYON_PROJECT_NAME, AYON_SITE_ID);
}

TEST(AyonCppApi, AyonCppApiCreation) {
    AyonApi Test = getApiInstance();
}

TEST(AyonCppApi, AyonCppApiSerialResolveRootReplace) {
    Instrumentor::Get().BeginSession("Profile", "bin/profSerial.json");
    AyonApi Api = getApiInstance();
    nlohmann::json JsonFileStage = JsonFile["Resolve"];
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
    nlohmann::json JsonFileStage = JsonFile["Resolve"];
    bool RunOnlyOneResolveIteration = false;
    bool printResult = false;

    if (!AyonCppApiTest::test_BatchResolve(JsonFile, printResult, Api)) {
        FAIL();
    }

    Instrumentor::Get().EndSession();
}

AyonApi getApiInstanceSSL() {
    std::string AYON_API_KEY;
    std::string AYON_SERVER_URL;
    std::string AYON_SITE_ID;
    std::string AYON_PROJECT_NAME;

    #ifdef _WIN32
    std::string envFilePath("test\\.env_https");
    #else
    std::string envFilePath("test/.env_https");
    #endif
    if (!AyonCppApiTest::load_EnvVariables(envFilePath, AYON_API_KEY, AYON_SERVER_URL, AYON_SITE_ID, AYON_PROJECT_NAME)) {
        std::cerr << "Failed to load environment variables!" << std::endl;
        throw std::runtime_error("Failed to load environment variables!");
    }

    return AyonApi("./test_logs", AYON_API_KEY, AYON_SERVER_URL, AYON_PROJECT_NAME, AYON_SITE_ID);
}

TEST(AyonCppApi, AyonCppApiCreationSSL) {
    AyonApi Test = getApiInstanceSSL();
}

int main(int argc, char** argv) {
    std::cout << "Running tests..." << std::endl;
    std::ifstream file("test/testData.json");
    if (!file.is_open()) {
        std::cerr << "Failed to open file!" << std::endl;
    }
    JsonFile = nlohmann::json::parse(file);
    file.close();

    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
