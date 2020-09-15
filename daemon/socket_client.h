#ifndef SOCKET_CLIENT_H
#define SOCKET_CLIENT_H

#include <stdlib.h>

int initialize(const char *socket_path);

bool send_on_socket(int fd, void *buf, size_t size);

bool receive_on_socket(int fd, void *buf, size_t size);

void end(int fd);

#endif