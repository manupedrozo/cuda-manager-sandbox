#ifndef LOGGER_H
#define LOGGER_H

#include <memory>

#include "spdlog/spdlog.h"

namespace cuda_daemon {

class Logger {

public:
  enum class Level {
    TRACE,
    DEBUG,
    INFO,
    WARN,
    ERROR,
    CRITICAL,
    OFF
  };

  Logger(const Logger &) = delete;

  static void set_level(Level level);
  static Logger &get_instance();

  template <typename FormatString, typename... Args>
  inline void trace(const FormatString &fmt, const Args &... args) {
    logger->trace(fmt, args...);
  }

  template <typename FormatString, typename... Args>
  inline void debug(const FormatString &fmt, const Args &... args) {
    logger->debug(fmt, args...);
  }

  template <typename FormatString, typename... Args>
  inline void info(const FormatString &fmt, const Args &... args) {
    logger->info(fmt, args...);
  }

  template <typename FormatString, typename... Args>
  inline void warn(const FormatString &fmt, const Args &... args) {
    logger->warn(fmt, args...);
  }

  template <typename FormatString, typename... Args>
  inline void error(const FormatString &fmt, const Args &... args) {
    logger->error(fmt, args...);
  }

  template <typename FormatString, typename... Args>
  inline void critical(const FormatString &fmt, const Args &... args) {
    logger->critical(fmt, args...);
  }

  template <typename T>
  inline void trace(const T &msg) {
    logger->trace(msg);
  }

  template <typename T>
  inline void debug(const T &msg) {
    logger->debug(msg);
  }

  template <typename T>
  inline void info(const T &msg) {
    logger->info(msg);
  }

  template <typename T>
  inline void warn(const T &msg) {
    logger->warn(msg);
  }

  template <typename T>
  inline void error(const T &msg) {
    logger->error(msg);
  }

  template <typename T>
  inline void critical(const T &msg) {
    logger->critical(msg);
  }

private:
  static spdlog::level::level_enum level;
  std::unique_ptr<spdlog::logger> logger;

  Logger();
};

} // namespace cuda_daemon

#endif
