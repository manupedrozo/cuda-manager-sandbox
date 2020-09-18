#include "logger.h"
#include "spdlog/sinks/stdout_color_sinks.h"

namespace cuda_mango {
    std::shared_ptr<Logger> Logger::instance = nullptr;

    Logger::Logger() {
        auto console_sink = std::make_unique<spdlog::sinks::stdout_color_sink_mt>();
        logger = std::make_unique<spdlog::logger>("logger", std::move(console_sink));
        logger->set_level(spdlog::level::trace);
    }

    std::shared_ptr<Logger> Logger::get_instance() {
        if (!instance) instance = std::shared_ptr<Logger>(new Logger);
        return instance;
    }
}