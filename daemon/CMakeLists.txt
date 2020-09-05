set(SERVER_SRC server.cpp)
set(CLIENT_SRC client.cpp)
set(KILLER_SRC killer.cpp)

set(COMMON_HEADERS commands.h)

add_executable(server ${SERVER_SRC})
add_executable(client ${CLIENT_SRC})
add_executable(killer ${KILLER_SRC})
