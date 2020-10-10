cmake_minimum_required (VERSION 2.8.11)

set(MANGO_ROOT /opt/mango)

project(daemon LANGUAGES CXX)

# Specify rpath to avoid having to add the path to LD_LIBRARY_PATH
# Necessary because cuda_manager is in a non-standard location
set(CMAKE_INSTALL_RPATH ${CMAKE_INSTALL_RPATH}:${MANGO_ROOT}/lib/cuda_manager)

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

target_include_directories(kernel-launch PRIVATE ${PROJECT_SOURCE_DIR} ${PROJECT_SOURCE_DIR}/external)
target_include_directories(cuda-server PRIVATE ${PROJECT_SOURCE_DIR} ${PROJECT_SOURCE_DIR}/external)

target_link_libraries(cuda-server PRIVATE cuda_manager spdlog::spdlog)
target_link_libraries(kernel-launch PRIVATE cuda_manager spdlog::spdlog)

configure_file(lorem.txt lorem.txt COPYONLY)
configure_file(daemon.conf.in daemon.conf COPYONLY)
configure_file(saxpy.cu saxpy.cu COPYONLY)

install(TARGETS cuda-server 
  RUNTIME DESTINATION ${MANGO_ROOT}/bin)
