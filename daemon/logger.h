#ifndef LOGGER_H
#define LOGGER_H

#include <memory>

#include "spdlog/spdlog.h"

namespace cuda_mango {
    class Logger {

        public:
            static std::shared_ptr<Logger> get_instance();

            template<typename FormatString, typename... Args>
            inline void trace(const FormatString &fmt, const Args &... args)
            {
                logger->trace(fmt, args...);
            }

            template<typename FormatString, typename... Args>
            inline void debug(const FormatString &fmt, const Args &... args)
            {
                logger->debug(fmt, args...);
            }

            template<typename FormatString, typename... Args>
            inline void info(const FormatString &fmt, const Args &... args)
            {
                logger->info(fmt, args...);
            }

            template<typename FormatString, typename... Args>
            inline void warn(const FormatString &fmt, const Args &... args)
            {
                logger->warn(fmt, args...);
            }

            template<typename FormatString, typename... Args>
            inline void error(const FormatString &fmt, const Args &... args)
            {
                logger->error(fmt, args...);
            }

            template<typename FormatString, typename... Args>
            inline void critical(const FormatString &fmt, const Args &... args)
            {
                logger->critical(fmt, args...);
            }

            template<typename T>
            inline void trace(const T &msg)
            {
                logger->trace(msg);
            }

            template<typename T>
            inline void debug(const T &msg)
            {
                logger->debug(msg);
            }

            template<typename T>
            inline void info(const T &msg)
            {
                logger->info(msg);
            }

            template<typename T>
            inline void warn(const T &msg)
            {
                logger->warn(msg);
            }

            template<typename T>
            inline void error(const T &msg)
            {
                logger->error(msg);
            }

            template<typename T>
            inline void critical(const T &msg)
            {
                logger->critical(msg);
            }

        private:
            static std::shared_ptr<Logger> instance;
            std::unique_ptr<spdlog::logger> logger;

            Logger();
    };
}

#endif