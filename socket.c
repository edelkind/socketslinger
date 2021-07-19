#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/un.h>

#include <lx_string.h>

#include "path.h"

/* connect() and bind() share this prototype */
typedef int(*sockfn_t)(int,const struct sockaddr*, socklen_t);

static inline int _socket_attach(
        const char *path, unsigned len,
        sockfn_t sockfn) {
    struct sockaddr_un sa;
    int sock;

    memset(&sa, 0, sizeof(sa));
    sa.sun_family = AF_UNIX;
    if (len >= SUN_PATH_SIZE)
        len = SUN_PATH_SIZE - 1;

    memcpy(sa.sun_path, path, len); /* \0 already present */

    if (    (sock = socket(AF_UNIX, SOCK_SEQPACKET, 0)) < 0 ||
            sockfn(sock, (struct sockaddr *)&sa, sizeof(sa)) )
        return -1;

    return sock;
}

/* Open unix domain socket with the supplied path (which has the supplied len).
 * If len is longer than (sun_path-1), it will be truncated.
 *
 * Return value is the same as for socket(2).
 */
int socket_listener(const char *path, unsigned len) {
    int sock_listen;

    if (    (sock_listen = _socket_attach(path, len, bind)) < 0 ||
            listen(sock_listen, 20) ) {
        return -1;
    }

    return sock_listen;

}

int socket_connect(const char *path, unsigned len) {
    return _socket_attach(path, len, connect);
}
