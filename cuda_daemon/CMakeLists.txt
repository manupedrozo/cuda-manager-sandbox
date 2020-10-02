cmake_minimum_required (VERSION 2.8.11)

set(NO_INSTALL true CACHE BOOL "Whether daemon links to the installed cuda manager library or not")

set(MANGO_ROOT /opt/mango)

project(daemon LANGUAGES CXX)

set (CMAKE_CXX_STANDARD 14)

set(SERVER_SRC server.cpp test/server-example.cpp logger.cpp config_reader.cpp)
set(CLIENT_1_SRC socket_client.cpp test/client-hello.cpp)
set(CLIENT_2_SRC socket_client.cpp test/client-continous.cpp)
set(CLIENT_3_SRC socket_client.cpp test/client-variable-length.cpp)
set(CLIENT_4_SRC socket_client.cpp test/client-multimessage.cpp)
set(CLIENT_5_SRC cuda_client.cpp server.cpp socket_client.cpp cuda_server.cpp config_reader.cpp logger.cpp test/kernel-launch.cpp)
set(CUDA_SERVER cuda_client.cpp server.cpp socket_client.cpp cuda_server.cpp config_reader.cpp logger.cpp test/cuda-server.cpp)
set(KILLER_SRC test/killer.cpp)

set(COMMON_HEADERS commands.h)

find_package(spdlog REQUIRED)

add_executable(server ${SERVER_SRC})
add_executable(client ${CLIENT_1_SRC})
add_executable(client-continous ${CLIENT_2_SRC})
add_executable(client-variable-length ${CLIENT_3_SRC})
add_executable(client-multimessage ${CLIENT_4_SRC})
add_executable(kernel-launch ${CLIENT_5_SRC})
add_executable(cuda-server ${CUDA_SERVER})
add_executable(killer ${KILLER_SRC})

target_link_libraries(server PRIVATE spdlog::spdlog)


target_include_directories(server PRIVATE ${PROJECT_SOURCE_DIR} ${PROJECT_SOURCE_DIR}/external)
target_include_directories(client PRIVATE ${PROJECT_SOURCE_DIR} ${PROJECT_SOURCE_DIR}/external)
target_include_directories(client-continous PRIVATE ${PROJECT_SOURCE_DIR} ${PROJECT_SOURCE_DIR}/external)
target_include_directories(client-variable-length PRIVATE ${PROJECT_SOURCE_DIR} ${PROJECT_SOURCE_DIR}/external)
target_include_directories(client-multimessage PRIVATE ${PROJECT_SOURCE_DIR} ${PROJECT_SOURCE_DIR}/external)
target_include_directories(killer PRIVATE ${PROJECT_SOURCE_DIR} ${PROJECT_SOURCE_DIR}/external)

if(NO_INSTALL)
  message("Daemon: linking to project local cuda_manager library")
  target_link_libraries(cuda-server PRIVATE cuda_manager spdlog::spdlog)
  target_link_libraries(kernel-launch PRIVATE cuda_manager spdlog::spdlog)
  target_include_directories(kernel-launch PRIVATE ../cuda_manager ${PROJECT_SOURCE_DIR} ${PROJECT_SOURCE_DIR}/external)
  target_include_directories(cuda-server PRIVATE ../cuda_manager ${PROJECT_SOURCE_DIR} ${PROJECT_SOURCE_DIR}/external)
else()
  message("Daemon: linking to installed cuda_manager library")
  find_library(CUDA_MANAGER cuda_manager HINTS ${MANGO_ROOT}/lib)
  target_link_libraries(cuda-server PRIVATE ${CUDA_MANAGER} spdlog::spdlog)
  target_link_libraries(kernel-launch PRIVATE ${CUDA_MANAGER} spdlog::spdlog)
  target_include_directories(kernel-launch PRIVATE ${MANAGER_ROOT}/include/libcuda_manager/cuda_manager ${PROJECT_SOURCE_DIR} ${PROJECT_SOURCE_DIR}/external)
  target_include_directories(cuda-server PRIVATE ${MANGO_ROOT}/include/libcuda_manager/cuda_manager ${PROJECT_SOURCE_DIR} ${PROJECT_SOURCE_DIR}/external)
endif()

configure_file(lorem.txt lorem.txt COPYONLY)
configure_file(daemon.conf.in daemon.conf COPYONLY)
