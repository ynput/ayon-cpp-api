
#include <functional>
#include <iostream>
#include <map>
#include <memory>
#include <string>
#include <algorithm>
#include <filesystem>
#include <future>
#include <iostream>
#include <string>
#include <vector>
#include "AyonCppApi.h"
#include "Instrumentor.h"
#include <backward.hpp>

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

// TODO implemnt the Testing class for better controle

// class Tester {
//     public:
//         Tester(TestFileLogger* globaleFileLogger) {
//             testFileLogger_ptr = std::make_shared<TestFileLogger>(globaleFileLogger);
//         }
//         void
//         runTest(const std::string &funcName) {
//             auto it = functionRegester.find(funcName);
//             if (it != functionRegester.end()) {
//                 it->second();
//             }
//             else {
//                 std::cout << "Function not found" << std::endl;
//             }
//         }
//         void
//         runAllTests() {
//             for (auto i = functionRegester.begin(); i != functionRegester.end(); i++) {
//                 *testFileLogger_ptr << "running function: " << i->first;
//             }
//         }
//
//     private:
//         std::vector<bool> funcRunSucsessStore;
//         std::map<std::string, std::function<void()>> functionRegester;
//         std::shared_ptr<TestFileLogger> testFileLogger_ptr;
// };
