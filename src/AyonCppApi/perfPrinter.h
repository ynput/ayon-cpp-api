#include <chrono>
#include <iostream>
#include <string>
#include <vector>

struct perfStats {
        std::chrono::time_point<std::chrono::high_resolution_clock> startTime;
        std::chrono::time_point<std::chrono::high_resolution_clock> endTime;
        std::string StatName;
        std::string HeaderText;
        std::string FoderText;
        std::string Note;
        std::vector<std::string> extraInfo;
        bool Stop = false;
};

class perfPrinter {
    public:
        perfStats currentRunStats;

        perfPrinter(const std::string &Name) {
            currentRunStats.StatName = Name;
            currentRunStats.startTime = std::chrono::high_resolution_clock::now();
        }
        ~perfPrinter() {
            this->printStats();
        };

        void
        printStats() {
            std::cout << currentRunStats.StatName << "\n"
                      << currentRunStats.HeaderText << "\n"
                      << currentRunStats.Note << "\n";

            // Print Time Info
            long long castedStartTime
                = std::chrono::time_point_cast<std::chrono::microseconds>(currentRunStats.startTime)
                      .time_since_epoch()
                      .count();

            long long castEndTime
                = std::chrono::time_point_cast<std::chrono::milliseconds>(std::chrono::high_resolution_clock::now())
                      .time_since_epoch()
                      .count();
            std::cout << "Execution Time: " << castedStartTime - castEndTime << "\n";

            std::cout << "ThreadId: " << std::hash<std::thread::id>{}(std::this_thread::get_id()) << "\n";

            // Print Extra Info
            for (const std::string &infoEntry: currentRunStats.extraInfo) {
                std::cout << infoEntry << "\n";
            }
            std::cout << currentRunStats.FoderText << std::endl;
        };
};
