#ifndef AYONCPPAPIINSTRUMENTOR_H
#define AYONCPPAPIINSTRUMENTOR_H

#include <algorithm>
#include <chrono>
#include <fstream>
#include <future>
#include <mutex>
#include <string>
#include <thread>

struct ProfileResult {
    std::string name;
    long long start, end;
    uint32_t threadID;
};

struct InstrumentationSession {
    std::string name;
};

class Instrumentor {
    private:
        InstrumentationSession* m_currentSession;
        std::ofstream m_outputStream;
        int m_profileCount;
        std::mutex m_mutex;   // Mutex for synchronization

    public:
        Instrumentor(): m_currentSession(nullptr), m_profileCount(0) {
        }

        void BeginSession(const std::string &name, const std::string &filepath = "results.json") {
            std::lock_guard<std::mutex> lock(m_mutex);   // Lock mutex for critical section
            m_outputStream.open(filepath);
            WriteHeader();
            m_currentSession = new InstrumentationSession{name};
        }

        void endSession() {
            WriteFooter();
            m_outputStream.close();
            delete m_currentSession;
            m_currentSession = nullptr;
            m_profileCount = 0;
        }

        void WriteProfileAsync(const ProfileResult &result) {
            std::future<void> asyncResult = std::async(std::launch::async, [this, result]() { WriteProfile(result); });
            asyncResult.get();
        }

        void WriteProfile(const ProfileResult &result) {
            std::lock_guard<std::mutex> lock(m_mutex);   // Lock mutex for critical section
            if (m_profileCount++ > 0)
                m_outputStream << ",";

            std::string name = result.name;
            std::replace(name.begin(), name.end(), '"', '\'');

            m_outputStream << "{";
            m_outputStream << "\"cat\":\"function\",";
            m_outputStream << "\"dur\":" << (result.end - result.start) << ',';
            m_outputStream << "\"name\":\"" << name << "\",";
            m_outputStream << "\"ph\":\"X\",";
            m_outputStream << "\"pid\":0,";
            m_outputStream << "\"tid\":" << result.threadID << ",";
            m_outputStream << "\"ts\":" << result.start;
            m_outputStream << "}";

            m_outputStream.flush();
        }

        void WriteHeader() {
            m_outputStream << "{\"otherData\": {},\"traceEvents\":[";
            m_outputStream.flush();
        }

        void WriteFooter() {
            m_outputStream << "]}";
            m_outputStream.flush();
        }

        static Instrumentor & Get() {
            static Instrumentor instance;
            return instance;
        }
};

class InstrumentationTimer {
    public:
        InstrumentationTimer(const char* name): m_name(name), m_stopped(false) {
            m_startTimepoint = std::chrono::high_resolution_clock::now();
        }

        ~InstrumentationTimer() {
            if (!m_stopped)
                Stop();
        }

        void Stop() {
            auto endTimepoint = std::chrono::high_resolution_clock::now();

            long long start 
                = std::chrono::time_point_cast<std::chrono::microseconds>(m_startTimepoint).time_since_epoch().count();
            long long end 
                = std::chrono::time_point_cast<std::chrono::microseconds>(endTimepoint).time_since_epoch().count();

            uint32_t threadID = std::hash<std::thread::id>{}(std::this_thread::get_id());

            Instrumentor::Get().WriteProfileAsync({m_name, start, end, threadID});

            m_stopped = true;
        }

    private:
        const char* m_name;
        std::chrono::time_point<std::chrono::high_resolution_clock> m_startTimepoint;
        bool m_stopped;
};

#endif   // !AYONCPPAPIINSTRUMENTOR_H
