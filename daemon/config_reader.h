#include <string>
#include "logger.h"

namespace cuda_mango {

    typedef struct {
        Logger::Level log_level;
    } daemon_config_t;

    class ConfigReader {

        public:
            enum class ExitCode {
                OK,                 // Success
                CANNOT_OPEN_FILE,   // Cannot open config file
                ERROR               // Generic error
            };

            static ExitCode read_config(std::string path, daemon_config_t &config);

        private:
            ConfigReader();
    };
}