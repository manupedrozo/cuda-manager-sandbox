project(daemon LANGUAGES CXX)

set (CMAKE_CXX_STANDARD 14)

set(SERVER_SRC server.cpp)
set(CLIENT_SRC client.cpp hello-client.cpp)
set(CLIENT_2_SRC client.cpp client-continous.cpp)
set(KILLER_SRC killer.cpp)

set(COMMON_HEADERS commands.h)

add_executable(server ${SERVER_SRC})
add_executable(client ${CLIENT_SRC})
add_executable(client-continous ${CLIENT_2_SRC})
add_executable(killer ${KILLER_SRC})
