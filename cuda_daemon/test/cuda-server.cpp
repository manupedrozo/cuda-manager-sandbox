#include "cuda_server.h"
#include "config_reader.h"

#define SOCKET_PATH "/tmp/server-test"
#define CONFIG_FILE "daemon.conf"


using namespace cuda_daemon;

int main(int argc, char const *argv[]) { 
  daemon_config_t config;
  auto config_res = ConfigReader::read_config(CONFIG_FILE, config);

  auto &logger = Logger::get_instance();

  if (config_res != ConfigReader::ExitCode::OK) {
    if (config_res == ConfigReader::ExitCode::CANNOT_OPEN_FILE) {
      logger.critical("Cannot open config file");
    } else {
      logger.critical("Error parsing config file");
    }
    return -1;
  }

  Logger::set_level(config.log_level);

  logger.info("Server initialized, starting loop...");

  CudaServer cuda_server(SOCKET_PATH);
}
