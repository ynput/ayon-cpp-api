#include <filesystem>
#include <iostream>
#include <string>
#include <vector>
#include "AyonCppApi.h"
#include "Instrumentor.h"
#include "testLib.h"

int
generateRandomInt(int seed, int min_val, int max_val) {
    std::mt19937 gen(seed);
    std::uniform_int_distribution<int> distribution(min_val, max_val);
    return distribution(gen);
}

void
printInput(const std::vector<std::string> &strings) {
    std::cout << "testing on :\n ";
    for (const auto &str: strings) {
        std::cout << str << std::endl;
    }
}
std::string
getCurrentDateTimeAsString() {
    auto now = std::chrono::system_clock::now();
    std::time_t now_time_t = std::chrono::system_clock::to_time_t(now);
    std::tm* now_tm = std::localtime(&now_time_t);
    std::ostringstream oss;
    oss << std::put_time(now_tm, "%Y-%m-%d_%H:%M:%S");

    return oss.str();
}

class TestFileLogger {
    public:
        TestFileLogger(std::string &loggingFile) {
            outFile = std::ofstream(loggingFile, std::ios::app);
        }
        ~TestFileLogger() {
            outFile.close();
        }
        template<typename T>
        TestFileLogger &
        operator<<(const T &value) {
            outFile << value << std::endl;
            outFile << std::flush;
            return *this;
        }

    private:
        std::vector<std::string> vec;
        std::ofstream outFile;
};

int
main(int argc, char* argv[]) {
    //------------------ Init
    if (argc != 7) {
        std::cerr << "Usage: " << argc << argv[0] << " <string1> <string2> <int1> <int2> <int3> <int4>" << std::endl;
        return 1;
    }

    std::string string1 = argv[1];
    std::string string2 = argv[2];
    int min_paths = std::atoi(argv[3]);
    int runIterations = std::atoi(argv[4]);
    int minRandomeNameInt = std::atoi(argv[5]);
    int maxRandomeNameInt = std::atoi(argv[6]);

    //------- setup logging and progiling
    std::string profile_ext = ".json";
    std::string testStartDatation = getCurrentDateTimeAsString();
    std::string profileJsonName = "profile_" + testStartDatation + profile_ext;

    std::string loggingFileName = "Log_" + testStartDatation + ".txt";

    //-------- Start logging and profiling
    Instrumentor::Get().BeginSession("Profile", std::filesystem::current_path() / profileJsonName.c_str());
    TestFileLogger TestLogger = TestFileLogger(loggingFileName);

    //-------------- Testing
    AyonApi Ayon;
    with_func("\nBatch Resolve Test \n") {
        for (int x = 1; x <= runIterations; x++) {
            int pathsToResolve = min_paths * x;
            std::string timer_name = "Paths : " + std::to_string(pathsToResolve);
            InstrumentationTimer timer(timer_name.data());
            TestLogger << timer_name.data() << "\n";

            std::vector<std::string>* test = new std::vector<std::string>;

            for (int i = 0; i < pathsToResolve; i++) {
                test->emplace_back(string1 + std::to_string(generateRandomInt(i, 0, 300)) + string2);
            }
            std::unordered_map<std::string, std::string> batchResolve = Ayon.batchResolvePath(*test);

            for (const std::pair<std::string, std::string> &path: batchResolve) {
                TestLogger << "Asset Identifier: " << path.first << "asset path: " << path.second << "\n";
            }
            delete test;
        }
    }

    // int min_paths = 30;
    // int iterations = 20;
    // std::cout << "Start Test" << std::endl;
    // Instrumentor::Get().BeginSession("Profile", "/home/workh/Ynput/dev/ayon-cpp-api/profile.json");

    // ---------------- Testing individuale resolve
    // with_func("\n single Resolve Test \n") {
    //     {
    //         InstrumentationTimer timer("Single Test");
    //
    //         for (int x = 1; x <= runIterations; x++) {
    //             int pathsToResolve = min_paths * x;
    //             std::string timer_name = "Paths : " + std::to_string(pathsToResolve);
    //             InstrumentationTimer timer(timer_name.data());
    //
    //             std::vector<std::string>* test = new std::vector<std::string>;
    //
    //             for (int i = 0; i < pathsToResolve; i++) {
    //                 test->emplace_back(string1 + std::to_string(generateRandomInt(i, 0, 300)) + string2);
    //             }
    //
    //             delete test;
    //         }
    //     }
    // }
    Instrumentor::Get().EndSession();
}
