#include <errno.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>

// #include <glib.h>
#include <get_opts.h>
#include <lx_string.h>
#include <minimisc.h>

#include "defaults.h"
#include "descriptor.h"
#include "path.h"
#include "socket.h"

// static gint Descriptor;
// 
// static GOptionEntry opts_main[] =
// {
//     { "descriptor", 'd', 0, G_OPTION_ARG_INT, &Descriptor, "Descriptor to pass (default 0)", "N" },
//     { NULL }
// };
// 
// /* Collect command-line options.
//  */
// static void
// gather_options(int *argcp, char ***argvp)
// {
//     GOptionContext *ctx;
//     GError *error = 0;
// 
//     ctx = g_option_context_new(" - descriptor slinger");
//     g_option_context_add_main_entries(ctx, opts_main, NULL);
//     if (!g_option_context_parse(ctx, argcp, argvp, &error))
//     {
//         g_printerr ("Usage Error: %s\nTry -h or --help for help.\n", error->message);
//         exit(1);
//     }
// 
//     g_option_context_free(ctx);
// }

void usage(void) {
    fprintf(stderr,
"sling-input [-d N]\n"
"\n"
"  -d, --descriptor  Descriptor to pass (default 0)\n"
"  -h, --help        Display usage help\n");
    exit(1);
}


add_opt(opt_descriptor, "d",  "descriptor", OPT_INT, (opt_int_t)0, 0);
add_opt(opt_help,       "h",  "help", OPT_TOGGLE, (opt_toggle_t)0, 0);

put_opts(options, &opt_descriptor, &opt_help);

static int interrupt;
void sig_handle(int sig) {
    interrupt = sig;
}

int main(int argc, char **argv) {
    lx_s sockpath = {0};
    int sock_listen, sock_conn;
    char **args;
    int argn;

    // gather_options(&argc, &argv);
    argn = get_opts_errormatic(&args, argv+1, argc-1, options);
    if (argn > 0 || opt_help.v_opt.opt_toggle)
        usage();


    if (build_socket_path_pid(&sockpath, getpid())) {
        perror("build_socket_path_pid");
        return 1;
    }

    /* Any of these signals should result in the socket being cleaned up.
     * This will also cause accept() to fail with error EINTR, but that's
     * fine, because it shouldn't happen in the course of normal operation
     * (i.e. if a signal is not delivered).  This program is simple enough
     * that we needn't to worry about whether the accept() should be retried.
     */
    sig_catch(SIGINT, sig_handle);
    sig_catch(SIGTERM, sig_handle);
    sig_catch(SIGHUP, sig_handle);

    (void) unlink(lx_cstr(&sockpath));
    sock_listen = socket_listener(sockpath.s, sockpath.len);
    if (sock_listen < 0) {
        perror("socket_listener");
        unlink(lx_cstr(&sockpath));
        return 1;
    }


    sock_conn = accept(sock_listen, NULL, NULL);
    unlink(lx_cstr(&sockpath));  // cleanup
    if (sock_conn == -1) {
        return 1;
    }


//    struct sockaddr_un sa;
//    memset(&sa, 0, sizeof(sa));
//    sa.sun_family = AF_UNIX;
//    if (socklen >= (sizeof(sa.sun_path))) {
//        fprintf(stderr, "Socket path (%s) too long (%ld > %lu).\n",
//                sockpath, socklen, sizeof(sa.sun_path) - 1);
//        exit(1);
//    }
//    memcpy(sa.sun_path, sockpath, socklen); /* \0 already present */
//
//    sock_conn = socket(AF_UNIX, SOCK_SEQPACKET, 0);
//    if (sock_conn == -1) {
//        perror("socket");
//        exit(1);
//    }
//
//
//
//    for (unsigned tries = 0;; tries++) {
//        if (!connect(sock_conn, (struct sockaddr *)&sa, sizeof(sa)))
//            break;
//        if (errno == ECONNREFUSED) {
//            if (!(tries % 10))
//                fprintf(stderr, "Waiting for catcher to listen on socket...\n");
//            sleep(1);
//            continue;
//        }
//        perror("connect");
//        exit(1);
//    }

//    if (desc_write(sock_conn, "", 1, 0) == -1 ||
//        desc_write(sock_conn, "", 1, 1) == -1) {
    if (desc_write(sock_conn, "", 1, opt_descriptor.v_opt.opt_int) == -1) {
        perror("desc_write");
        exit(1);
    }

    return 0;
}

