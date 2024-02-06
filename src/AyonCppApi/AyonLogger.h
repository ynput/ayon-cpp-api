#include <memory>
#include "spdlog/sinks/basic_file_sink.h"
#include "spdlog/sinks/stdout_color_sinks.h"
#include "spdlog/async.h"

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

        template<typename... Args>
        void
        error(const std::string &format, const Args &... args) {
            log("error", format, args...);
        }

        template<typename... Args>
        void
        info(const std::string &format, const Args &... args) {
            log("info", format, args...);
        }

        template<typename... Args>
        void
        warn(const std::string &format, const Args &... args) {
            log("warn", format, args...);
        }

        template<typename... Args>
        void
        critical(const std::string &format, const Args &... args) {
            log("critical", format, args...);
        }

    private:
        AyonLogger(const std::string &filepath) {
            // Initialize console logger
            console_logger_ = spdlog::stdout_color_mt("console");
            console_logger_->set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%^%l%$] %v");

            // Initialize file logger
            file_logger_ = spdlog::basic_logger_mt<spdlog::async_factory>("fileLogger", filepath.c_str());
            file_logger_->set_pattern(
                "{\"timestamp\":\"%Y-%m-%d %H:%M:%S.%e\",\"level\":\"%l\",\"Thread "
                "Id\":\"%t\",\"Process Id\":\"%P\",\"message\":\"%v\"}");
        }

        /**
         * @brief this is the core logging function that is called fomr all other ones
         *
         * @tparam Args
         * @param level the log level for spdlog to use
         * @param format the massage text itself
         * @param args  things that will be included into the massage inplace off {}
         */
        template<typename... Args>
        void
        log(const std::string &level, const std::string &format, const Args &... args) {
            file_logger_->log(spdlog::level::from_str(level), format.c_str(), args...);
            console_logger_->log(spdlog::level::from_str(level), format.c_str(), args...);
        }
        std::shared_ptr<spdlog::logger> console_logger_;
        std::shared_ptr<spdlog::logger> file_logger_;
};
