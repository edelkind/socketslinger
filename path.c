#include <assert.h>
#include <pwd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <strings.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/un.h>
#include <unistd.h>

#include "defaults.h"
#include "path.h"

/* Return the user's home directory.  Prefer $HOME, but fallback to the passwd
 * entry.
 */
char *get_home(void) {
    char *homedir = getenv("HOME");

    if (!homedir) {
        homedir = getpwuid(getuid())->pw_dir;
    }
    return homedir;
}

/* Concatenate three path elements (e.g. "part1", "separator", "part2").
 *
 * The path returned will be dynamically allocated and may be freed.
 *
 * Never fails.  Aborts the program on OOM.
 */
char *path_attach(const char *arg1, const char *arg2, const char *arg3) {
    char *target;
    char *targetp;
    int len1 = strlen(arg1),
        len2 = strlen(arg2),
        len3 = strlen(arg3);

    assert(( target = malloc(len1 + len2 + len3 + 1) ));

    targetp = target;
    memcpy(targetp, arg1, len1);
    targetp += len1;
    memcpy(targetp, arg2, len2);
    targetp += len2;
    memcpy(targetp, arg3, len3);
    targetp += len3;
    *targetp = 0;

    return target;
}

/* Return the socket path under the user's runtime directory.  If there is no
 * XDG runtime directory, then a runtime subdirectory under $HOME will be
 * chosen.
 *
 * This function will attempt to create the runtime directory if it does not
 * exist, but it will not fail if unsuccessful.
 *
 * Also checks to ensure that the socket is not too long for
 * sockaddr_un.sun_path (plus terminating 0).
 *
 * This path is dynamically allocated but must never be freed manually, since
 * socket_path is used as cache.
 *
 * Never fails.  Aborts the program on OOM or if the socket length is too long.
 */
char *get_socket_path(void) {
    static char *socket_path;
    char *runtime_path;
    char *socketname;

    if (socket_path)
        return socket_path;

    runtime_path = getenv("XDG_RUNTIME_DIR");
    if (runtime_path) {
        runtime_path = path_attach(runtime_path, "/", DEFAULT_RUNTIME_SUBDIR);
    } else { 
        fprintf(stderr, "XDG_RUNTIME_DIR not set; using ~/.%s instead.\n", DEFAULT_RUNTIME_SUBDIR);
        runtime_path = path_attach(get_home(), "/.", DEFAULT_RUNTIME_SUBDIR);
    }
    (void) mkdir(runtime_path, 0700);


    if (!(socketname = getenv("SLING_SOCKET"))) {
        socketname = DEFAULT_SLING_SOCKET;
    } else if (index(socketname, '/')) {
        fprintf(stderr, "SLING_SOCKET can't contain path elements.\n");
        exit(1);
    }


    socket_path = path_attach(runtime_path, "/", socketname);
    free(runtime_path);

#define SUN_PATH_SIZE sizeof(((struct sockaddr_un*)0)->sun_path)
    if (strlen(socket_path) > (SUN_PATH_SIZE - 1)) {
        fprintf(stderr, "Socket path (%s) too long (must be no more than %lu bytes).\n",
                socket_path, SUN_PATH_SIZE - 1);
        exit(1);
    }

    return socket_path;
}


