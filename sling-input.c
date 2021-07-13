#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/un.h>

#include "defaults.h"
#include "descriptor.h"
#include "path.h"


int main(int argc, char **argv) {
    char *sockpath = get_socket_path();
    size_t socklen = strlen(sockpath);
    int sock_conn;

    struct sockaddr_un sa;
    memset(&sa, 0, sizeof(sa));
    sa.sun_family = AF_UNIX;
    if (socklen >= (sizeof(sa.sun_path))) {
        fprintf(stderr, "Socket path (%s) too long (%ld > %lu).\n",
                sockpath, socklen, sizeof(sa.sun_path) - 1);
        exit(1);
    }
    memcpy(sa.sun_path, sockpath, socklen); /* \0 already present */

    sock_conn = socket(AF_UNIX, SOCK_SEQPACKET, 0);
    if (sock_conn == -1) {
        perror("socket");
        exit(1);
    }

    if (connect(sock_conn, (struct sockaddr *)&sa, sizeof(sa)) == -1) {
        perror("connect");
        exit(1);
    }

    if (desc_write(sock_conn, "", 1, 0) == -1 ||
        desc_write(sock_conn, "", 1, 1) == -1) {
        perror("desc_write");
        exit(1);
    }

    return 0;
}

