#include "inih/INIReader.h"
#include "config_reader.h"

namespace cuda_daemon {
    ConfigReader::ExitCode ConfigReader::read_config(std::string path, daemon_config_t &config) {
        auto reader = INIReader(path);

        auto level_str = reader.Get("log", "level", "INFO");

        Logger::Level level = Logger::Level::INFO;
        if (level_str == "TRACE")           level = Logger::Level::TRACE;
        else if (level_str == "DEBUG")      level = Logger::Level::DEBUG;
        else if (level_str == "INFO")       level = Logger::Level::INFO;
        else if (level_str == "WARN")       level = Logger::Level::WARN;
        else if (level_str == "ERROR")      level = Logger::Level::ERROR;
        else if (level_str == "CRITICAL")   level = Logger::Level::CRITICAL;
        else if (level_str == "OFF")        level = Logger::Level::OFF;
        else level = Logger::Level::INFO;

        config.log_level = level;

        return ExitCode::OK;
    }
}
