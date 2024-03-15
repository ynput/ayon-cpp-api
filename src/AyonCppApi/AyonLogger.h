#include <cstdint>
#include <cstdlib>
#include <filesystem>
#include <memory>
#include <optional>
#include <set>
#include <string>
#include "spdlog/common.h"
#include "spdlog/sinks/basic_file_sink.h"
#include "spdlog/sinks/stdout_color_sinks.h"
#include "spdlog/async.h"
#include <spdlog/spdlog.h>
#include "AyonCppApiCrossPlatformMacros.h"
/**
 * @class AyonLogger
 * @brief Simple Logger Class that wraps around spdlog in order to expose easy logging functions \n
 *  AyonLogger::getInstance(log_File_path.json) init code \n
 *  automaticly logs to file and console
 */
class AyonLogger {
    public:
        static AyonLogger &
        getInstance(const std::string &filepath) {
            static AyonLogger instance(filepath);
            return instance;
        }

        std::set<std::string>::iterator
        key(const std::string &key) {
            return EnabledLoggingKeys.find(key);
        }

        bool
        regesterLoggingKey(const std::string &KeyName) {
            std::pair<std::set<std::string>::iterator, bool> insertion = EnabledLoggingKeys.insert(KeyName);
            if (insertion.second) {
                return true;
            }
            return false;
        }

        bool
        unregisterLoggingKey(const std::string &KeyName) {
            std::set<std::string>::iterator it = EnabledLoggingKeys.find(KeyName);
            if (it != EnabledLoggingKeys.end()) {
                EnabledLoggingKeys.erase(it);
                return true;
            }
            return false;
        }

        template<typename... Args>
        void
        error(const std::set<std::string>::iterator &logginIterator, const std::string &format, const Args &... args) {
            if (logginIterator != EnabledLoggingKeys.end()) {
                log("error", format, args...);
            }
        }

        template<typename... Args>
        void
        error(const std::string &format, const Args &... args) {
            log("error", format, args...);
        }

        template<typename... Args>
        void
        info(const std::set<std::string>::iterator &logginIterator, const std::string &format, const Args &... args) {
            if (logginIterator != EnabledLoggingKeys.end()) {
                log("info", format, args...);
            }
        }

        template<typename... Args>
        void
        info(const std::string &format, const Args &... args) {
            log("info", format, args...);
        }

        template<typename... Args>
        void
        warn(const std::set<std::string>::iterator &logginIterator, const std::string &format, const Args &... args) {
            if (logginIterator != EnabledLoggingKeys.end()) {
                log("warn", format, args...);
            }
        }

        template<typename... Args>
        void
        warn(const std::string &format, const Args &... args) {
            log("warn", format, args...);
        }

        template<typename... Args>
        void
        critical(const std::set<std::string>::iterator &logginIterator,
                 const std::string &format,
                 const Args &... args) {
            if (logginIterator != EnabledLoggingKeys.end()) {
                log("critical", format, args...);
            }
        }

        template<typename... Args>
        void
        critical(const std::string &format, const Args &... args) {
            log("critical", format, args...);
        }
        void
        LogLevlInfo(const bool &alsoSetFileLogger = false) {
            if (alsoSetFileLogger) {
                file_logger_->set_level(spdlog::level::info);
            }
            console_logger_->set_level(spdlog::level::info);
        }
        void
        LogLevlError(const bool &alsoSetFileLogger = false) {
            if (alsoSetFileLogger) {
                file_logger_->set_level(spdlog::level::err);
            }
            console_logger_->set_level(spdlog::level::err);
        }
        void
        LogLevlWarn(const bool &alsoSetFileLogger = false) {
            if (alsoSetFileLogger) {
                file_logger_->set_level(spdlog::level::warn);
            }
            console_logger_->set_level(spdlog::level::warn);
        }
        void
        LogLevlCritical(const bool &alsoSetFileLogger = false) {
            if (alsoSetFileLogger) {
                file_logger_->set_level(spdlog::level::critical);
            }
            console_logger_->set_level(spdlog::level::critical);
        }
        void
        LogLevlOff(const bool &alsoSetFileLogger = false) {
            if (alsoSetFileLogger) {
                file_logger_->set_level(spdlog::level::off);
            }
            console_logger_->set_level(spdlog::level::off);
        }

    private:
        AyonLogger(const std::string &filepath) {
            // Initialize console logger
            console_logger_ = spdlog::stdout_color_mt("console");
            console_logger_->set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%^%l%$] %v");

            char* envVarFileLoggingPath = std::getenv("AYONLOGGERFILEPOS");

            if (envVarFileLoggingPath != nullptr) {
                fileLoggerFilePath = std::string(
                    std::filesystem::absolute(std::string(envVarFileLoggingPath) + "/logFile.json").string());
                fileLoggerFilePathOverwrite = true;
            }

            char* envVarFileLogging = std::getenv("AYONLOGGERFILELOGGING");

            if (envVarFileLogging != nullptr) {
                switch (envVarFileLogging[1]) {
                    case 'F':
                        enableFileLogging = false;
                        break;
                    default:
                        enableFileLogging = true;
                        file_logger_ = spdlog::basic_logger_mt<spdlog::async_factory>(
                            "fileLogger", fileLoggerFilePathOverwrite ? fileLoggerFilePath.c_str() : filepath.c_str());
                        file_logger_->set_pattern(
                            "{\"timestamp\":\"%Y-%m-%d %H:%M:%S.%e\",\"level\":\"%l\",\"Thread "
                            "Id\":\"%t\",\"Process Id\":\"%P\",\"message\":\"%v\"}");
                        break;
                }
            }

            char* envVarLogLvl = std::getenv("AYONLOGGERLOGLVL");

            if (envVarLogLvl != nullptr) {
                switch (envVarLogLvl[0]) {
                    case 'I':
                        if (enableFileLogging) {
                            file_logger_->set_level(spdlog::level::info);
                        }
                        console_logger_->set_level(spdlog::level::info);
                        break;
                    case 'E':
                        if (enableFileLogging) {
                            file_logger_->set_level(spdlog::level::err);
                        }
                        console_logger_->set_level(spdlog::level::err);
                        break;
                    case 'W':
                        if (enableFileLogging) {
                            file_logger_->set_level(spdlog::level::warn);
                        }
                        console_logger_->set_level(spdlog::level::warn);
                        break;
                    case 'C':
                        if (enableFileLogging) {
                            file_logger_->set_level(spdlog::level::critical);
                        }
                        console_logger_->set_level(spdlog::level::critical);
                        break;
                    case 'O':
                        if (enableFileLogging) {
                            file_logger_->set_level(spdlog::level::off);
                        }
                        console_logger_->set_level(spdlog::level::off);
                        break;
                    default:
                        break;
                }
            }

            const char* envVarLoggingKeys = std::getenv("AYON_LOGGIN_LOGGIN_KEYS");
            if (envVarLoggingKeys != nullptr) {
                std::string envVarString(envVarLoggingKeys);
                std::string token;
                std::istringstream tokenStream(envVarString);
                while (std::getline(tokenStream, token, '/')) {
                    EnabledLoggingKeys.insert(token);
                }
            }
            else {
            }
        }

        template<typename... Args>
        void
        log(const std::string &level, const std::string &massage, const Args &... args) {
            // std::string formattedMassage = fmt::vformat(massage, args...);
            std::string formatted_message = fmt::vformat(massage, fmt::make_format_args(args...));

            if (enableFileLogging) {
                file_logger_->log(spdlog::level::from_str(level), formatted_message);
            }
            console_logger_->log(spdlog::level::from_str(level), formatted_message);
        }
        std::shared_ptr<spdlog::logger> console_logger_;
        std::shared_ptr<spdlog::logger> file_logger_;
        bool enableFileLogging;
        bool fileLoggerFilePathOverwrite;
        std::string fileLoggerFilePath;
        std::set<std::string> EnabledLoggingKeys;
};
