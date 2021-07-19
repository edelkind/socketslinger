#ifndef _SLING_SOCKET_H
#define _SLING_SOCKET_H

int socket_listener(const char *path, unsigned len);
int socket_connect(const char *path, unsigned len);

#endif // _SLING_SOCKET_H
