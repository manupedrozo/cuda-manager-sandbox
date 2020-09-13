cmake_minimum_required (VERSION 2.8.11)

project(daemon LANGUAGES CXX)

set (CMAKE_CXX_STANDARD 14)

set(SERVER_SRC server.cpp)
set(CLIENT_SRC client.cpp client-hello.cpp)
set(CLIENT_2_SRC client.cpp client-continous.cpp)
set(CLIENT_3_SRC client.cpp client-variable-length.cpp)
set(CLIENT_4_SRC client.cpp client-multimessage.cpp)
set(KILLER_SRC killer.cpp)

set(COMMON_HEADERS commands.h)

add_executable(server ${SERVER_SRC})
add_executable(client ${CLIENT_SRC})
add_executable(client-continous ${CLIENT_2_SRC})
add_executable(client-variable-length ${CLIENT_3_SRC})
add_executable(client-multimessage ${CLIENT_4_SRC})
add_executable(killer ${KILLER_SRC})

configure_file(lorem.txt lorem.txt COPYONLY)
