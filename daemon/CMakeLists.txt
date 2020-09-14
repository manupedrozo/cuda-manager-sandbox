cmake_minimum_required (VERSION 2.8.11)

project(daemon LANGUAGES CXX)

set (CMAKE_CXX_STANDARD 14)

set(SERVER_SRC server.cpp)
set(CLIENT_1_SRC socket_client.cpp test/client-hello.cpp)
set(CLIENT_2_SRC socket_client.cpp test/client-continous.cpp)
set(CLIENT_3_SRC socket_client.cpp test/client-variable-length.cpp)
set(CLIENT_4_SRC socket_client.cpp test/client-multimessage.cpp)
set(KILLER_SRC test/killer.cpp)

set(COMMON_HEADERS commands.h)

add_executable(server ${SERVER_SRC})
add_executable(client ${CLIENT_1_SRC})
add_executable(client-continous ${CLIENT_2_SRC})
add_executable(client-variable-length ${CLIENT_3_SRC})
add_executable(client-multimessage ${CLIENT_4_SRC})
add_executable(killer ${KILLER_SRC})

target_include_directories(server PRIVATE ${PROJECT_SOURCE_DIR})
target_include_directories(client PRIVATE ${PROJECT_SOURCE_DIR})
target_include_directories(client-continous PRIVATE ${PROJECT_SOURCE_DIR})
target_include_directories(client-variable-length PRIVATE ${PROJECT_SOURCE_DIR})
target_include_directories(client-multimessage PRIVATE ${PROJECT_SOURCE_DIR})
target_include_directories(killer PRIVATE ${PROJECT_SOURCE_DIR})

configure_file(lorem.txt lorem.txt COPYONLY)
