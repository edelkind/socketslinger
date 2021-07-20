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
    lx_s sockpath = {0},
         socktmp = {0};
    int sock_listen, sock_conn;
    char **args;
    int argn;

    // gather_options(&argc, &argv);
    argn = get_opts_errormatic(&args, argv+1, argc-1, options);
    if (argn > 0 || opt_help.v_opt.opt_toggle)
        usage();


    if (build_socket_path_pid(&sockpath, getpid()))
        die_err(1, "build_socket_path_pid");

    /* Any of these signals should result in the socket being cleaned up.
     * This will also cause accept() to fail with error EINTR, but that's
     * fine, because it shouldn't happen in the course of normal operation
     * (i.e. if a signal is not delivered).  This program is simple enough
     * that we needn't to worry about whether the accept() should be retried.
     */
    sig_catch(SIGINT, sig_handle);
    sig_catch(SIGTERM, sig_handle);
    sig_catch(SIGHUP, sig_handle);

    /* To avoid a race condition with the watcher/catcher between bind() and
     * listen() calls, the socket is initially created with a .tmp suffix,
     * then renamed once the socket is being listened on.  The watcher will
     * ignore sockets with a .tmp suffix and will trigger on the rename.
     */
    if (lx_alloc(&socktmp, sockpath.len + 4))
        die_err(1, "lx_alloc");

    // alloc succeeded, so no failure case
    (void) lx_strcopy(&socktmp, &sockpath);
    (void) lx_striadd(&socktmp, ".tmp", 4);

    (void) unlink(lx_cstr(&socktmp));
    sock_listen = socket_listener(socktmp.s, socktmp.len);
    if (sock_listen < 0) {
        (void) unlink(lx_cstr(&socktmp));
        die_err(1, "socket_listener");
    }
    if (rename(lx_cstr(&socktmp), lx_cstr(&sockpath))) {
        (void) unlink(socktmp.s);
        die_err(1, "rename(%s, %s)", socktmp.s, sockpath.s);
    }

    lx_free(&socktmp);

    sock_conn = accept(sock_listen, NULL, NULL);
    unlink(lx_cstr(&sockpath));  // cleanup
    if (sock_conn == -1) {
        return 1;
    }

    if (desc_write(sock_conn, "", 1, opt_descriptor.v_opt.opt_int) == -1) {
        perror("desc_write");
        exit(1);
    }

    return 0;
}

