#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>

#include "defaults.h"
#include "descriptor.h"
#include "path.h"

static inline void set_fd(int dfrom, int dto) {
    if (dfrom == dto)
        return;

    if (dup2(dfrom, dto) == -1) {
        fprintf(stderr, "dup2(%d, %d): %s\n", dfrom, dto, strerror(errno));
    }
}

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
    int d_stdio;
    // int d_stdin;
    // int d_stdout;
    // if (desc_read(sock_conn, &discard, 1, &d_stdin) == -1 ||
    //     desc_read(sock_conn, &discard, 1, &d_stdout) == -1) {
    if (desc_read(sock_conn, &discard, 1, &d_stdio) == -1) {
        perror("desc_read");
        exit(1);
    }
    write (d_stdio, "stdout!\n", 8);

    /* ensure that stdin and stdout are fds 5 and 6, respectively */
    /*
    set_fd(d_stdin, 5);
    set_fd(d_stdout, 6);
    */


    // int fds[] = { d_stdin, d_stdout };
    // for (int fd=0; fd <= 1; fd++) {
    //     int flags = fcntl(fds[fd], F_GETFD);
    //     if (flags == -1) {
    //         perror("fcntl(fd, F_GETFD)");
    //         continue;
    //     }
    //     flags &= ~FD_CLOEXEC;
    //     if (fcntl(fds[fd], F_SETFD, flags) == -1) {
    //         perror("fcntl(fd, F_SETFD, ...)");
    //         continue;
    //     }
    // }
    execlp("socat", "socat", "FD:5", "STDIO", NULL);

    return 0;
}

