#include <assert.h>
#include <errno.h>
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

#include <lx_string.h>

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

static inline void _make_my_dir(const char *path) {
    (void) mkdir(path, 0700);
}

/* Return the XDG cache dir.  The path is created (best effort) if it doesn't
 * exist.
 *
 * If $XDG_CACHE_HOME does not exist, then a .cache subdirectory under $HOME
 * will be chosen.
 *
 * This value is cached and must not be freed.
 */
char *get_xdg_cache_dir(void) {
    static char *xdg_cache;
    char *tmp_cache;

    if (xdg_cache)
        return xdg_cache;

    tmp_cache = getenv("XDG_CACHE_HOME");
    if (!tmp_cache || !*tmp_cache) {
        tmp_cache = path_attach(get_home(), "/", ".cache");
    }

    _make_my_dir(tmp_cache);
    xdg_cache = tmp_cache;
    return xdg_cache;

}

/* Return the XDG runtime dir.  The path is created (best effort) if it doesn't
 * exist.
 *
 * If $XDG_RUNTIME_DIR does not exist, the XDG cache dir is used instead, for
 * compatibility with glib.
 *
 * XXX: move to minilib
 *
 * This value is cached and must not be freed.
 */
char *get_xdg_runtime_dir(void) {
    static char *xdg_runtime;
    char *tmp_runtime;

    if (xdg_runtime)
        return xdg_runtime;

    tmp_runtime = getenv("XDG_RUNTIME_DIR");
    if (!tmp_runtime || !*tmp_runtime) {
        /* compatibility with glib */
        tmp_runtime = get_xdg_cache_dir();
        fprintf(stderr, "XDG_RUNTIME_DIR not set; using %s instead.\n", tmp_runtime);
    } else {
        _make_my_dir(tmp_runtime);
    }

    xdg_runtime = tmp_runtime;
    return xdg_runtime;
}

/* Return the socket slinger runtime dir.  The path is created (best effort) if it
 * doesn't exist.
 *
 * This value is cached and must not be freed.
 */
char *get_runtime_dir(void) {
    static char *runtime_dir;
    char *tmp_runtime;

    if (runtime_dir)
        return runtime_dir;

    tmp_runtime = path_attach(get_xdg_runtime_dir(), "/", DEFAULT_RUNTIME_SUBDIR);
    _make_my_dir(tmp_runtime);
    runtime_dir = tmp_runtime;
    return runtime_dir;
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
int build_socket_path_pid(lx_s *dest, pid_t pid) {
    char *socketbase;
    auto_lx_s(socketname) = {0};

    if (!(socketbase = getenv("SLING_SOCKET"))) {
        socketbase = DEFAULT_SLING_SOCKET;
    }

    if (lx_strset(&socketname, socketbase) ||
        lx_cadd(&socketname, '.') ||
        lx_straddlong(&socketname, pid, 10))
        return 1;

    return build_socket_path_name(dest, lx_cstr(&socketname));
}


int build_socket_path_name(lx_s *dest, const char *socketname) {
    char *runtime_dir = get_runtime_dir();

    if (index(socketname, '/')) {
        fprintf(stderr, "socket name can't contain path elements.\n");
        errno = EINVAL;
        return 1;
    }

    if (lx_strset(dest, runtime_dir) ||
        lx_cadd(dest, '/') ||
        lx_stradd(dest, socketname))
        return 1;

    if (dest->len > (SUN_PATH_SIZE - 1)) {
        fprintf(stderr, "Socket path (%s) too long (must be no more than %lu bytes).\n",
                lx_cstr(dest), SUN_PATH_SIZE - 1);
        errno = ENAMETOOLONG;
        return 1;
    }

    return 0;
}
