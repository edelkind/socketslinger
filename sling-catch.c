#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>

#include "defaults.h"
#include "descriptor.h"
#include "path.h"


int main(int argc, char **argv) {
    char *sockpath = get_socket_path();
    size_t socklen = strlen(sockpath);
    int sock_listen, sock_conn;

    struct sockaddr_un sa;
    memset(&sa, 0, sizeof(sa));
    sa.sun_family = AF_UNIX;
    if (socklen >= (sizeof(sa.sun_path))) {
        fprintf(stderr, "Socket path (%s) too long (%ld > %lu).\n",
                sockpath, socklen, sizeof(sa.sun_path) - 1);
        exit(1);
    }
    memcpy(sa.sun_path, sockpath, socklen); /* \0 already present */

    sock_listen = socket(AF_UNIX, SOCK_SEQPACKET, 0);
    if (sock_listen == -1) {
        perror("socket");
        exit(1);
    }

    unlink(sockpath);
    if (bind(sock_listen, (struct sockaddr *)&sa, sizeof(sa)) == -1) {
        perror("bind");
        exit(1);
    }

    if (listen(sock_listen, 20) == -1) {
        perror("listen");
        exit(1);
    }

    sock_conn = accept(sock_listen, NULL, NULL);

    char discard;
    int d_stdin;
    int d_stdout;
    if (desc_read(sock_conn, &discard, 1, &d_stdin) == -1 ||
        desc_read(sock_conn, &discard, 1, &d_stdout) == -1) {
        perror("desc_read");
        exit(1);
    }

    write(d_stdout, "stuff\n", 6);

    return 0;
}

