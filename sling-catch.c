#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>

#include <get_opts.h>

#include "defaults.h"
#include "descriptor.h"
#include "path.h"
#include "socket.h"

void usage(void) {
    fprintf(stderr,
"sling-catch [opts] -s sockname -- cmd args...\n"
"\n"
"  -s, --sockname      Socket name to connect to (required)\n"
"  -c, --cleanup       Clean up (remove) old sockets\n"
"  -d, --descriptor D  Descriptor number to use (default 5)\n"
"  -i, --stdio         Redirect or relay from/to stdin/stdout\n"
"  -h, --help          Display usage help\n");
    exit(1);
}


add_opt(opt_sockname,   "s",  "sockname", OPT_STRING, (opt_string_t)0, 0);
add_opt(opt_descriptor, "d",  "descriptor", OPT_INT, (opt_int_t)5, 0);
add_opt(opt_stdio,      "i",  "stdio", OPT_TOGGLE, (opt_toggle_t)0, 0); // TODO
add_opt(opt_clean,      "c",  "cleanup", OPT_TOGGLE, (opt_toggle_t)0, 0);
add_opt(opt_help,       "h",  "help", OPT_TOGGLE, (opt_toggle_t)0, 0);


put_opts(options, &opt_sockname, &opt_descriptor, &opt_stdio, &opt_clean,
        &opt_help);


static inline void set_fd(int dfrom, int dto) {
    if (dfrom == dto)
        return;

    if (dup2(dfrom, dto) == -1) {
        fprintf(stderr, "dup2(%d, %d): %s\n", dfrom, dto, strerror(errno));
    }
}

static inline int opts_validate(void) {
    if (!opt_sockname.v_opt.opt_string)
        return 1;

    if (opt_stdio.v_opt.opt_toggle &&
            (opt_descriptor.v_opt.opt_int == 0 ||
             opt_descriptor.v_opt.opt_int == 1)) {
        fprintf(stderr, "Can't redirect to stdio if descriptor is 0 or 1.\n\n");
        return 1;
    }

    return 0;
}

int main(int argc, char **argv) {
    lx_s sockpath = {0};
    int sock_conn;
    char **args;
    int argn;
    int d_io;

    argn = get_opts_errormatic(&args, argv+1, argc-1, options);
    if (argn < 1 || opt_help.v_opt.opt_toggle || opts_validate())
        usage();

    if (build_socket_path_name(&sockpath, opt_sockname.v_opt.opt_string)) {
        perror("build_socket_path_pid");
        return 1;
    }

    sock_conn = socket_connect(sockpath.s, sockpath.len);
    if (sock_conn < 0) {
        perror("socket_connect");
        if (opt_clean.v_opt.opt_toggle)
            unlink(lx_cstr(&sockpath));
        return 1;
    }

    {
        char discard;
        if (desc_read(sock_conn, &discard, 1, &d_io) == -1) {
            perror("desc_read");
            exit(1);
        }
    }

    if (opt_stdio.v_opt.opt_toggle && argn) {
        /* --stdio AND exec */
        set_fd(d_io, 0);
        set_fd(d_io, 1);
        if (d_io > 1)
            close(d_io);
    } else {
        /* --descriptor */
        if (d_io != opt_descriptor.v_opt.opt_int) {
            set_fd(d_io, opt_descriptor.v_opt.opt_int);
            close(d_io);
            d_io = opt_descriptor.v_opt.opt_int;
        }

        /* --stdio WITHOUT exec (!argn) */
        if (opt_stdio.v_opt.opt_toggle)
            return desc_relay(d_io, 0, 1);

    }

    // execlp("socat", "socat", "FD:5", "STDIO", NULL);
    return execvp(args[0], args);
}

