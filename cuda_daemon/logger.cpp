#include "logger.h"
#include "spdlog/sinks/stdout_color_sinks.h"

namespace cuda_daemon {
spdlog::level::level_enum Logger::level = spdlog::level::info;

Logger::Logger() {
  auto console_sink = std::make_unique<spdlog::sinks::stdout_color_sink_mt>();
  logger = std::make_unique<spdlog::logger>("logger", std::move(console_sink));
  logger->set_level(Logger::level);
}

Logger &Logger::get_instance() {
  static Logger instance;
  return instance;
}

void Logger::set_level(Level level) {
  switch (level) {
  case Level::TRACE:
    Logger::level = spdlog::level::trace;
    break;
  case Level::DEBUG:
    Logger::level = spdlog::level::debug;
    break;
  case Level::INFO:
    Logger::level = spdlog::level::info;
    break;
  case Level::WARN:
    Logger::level = spdlog::level::warn;
    break;
  case Level::ERROR:
    Logger::level = spdlog::level::err;
    break;
  case Level::CRITICAL:
    Logger::level = spdlog::level::critical;
    break;
  case Level::OFF:
    Logger::level = spdlog::level::off;
    break;
  }

  Logger::get_instance().logger->set_level(Logger::level);
}
} // namespace cuda_daemon
