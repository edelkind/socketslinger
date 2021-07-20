#include <dirent.h>
#include <errno.h>
#include <poll.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/inotify.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <string.h>

#include <minimisc.h>

#include "path.h"


static char
is_interesting(const char *name, char do_stat) {
    struct stat statbuf;
    int namelen = strlen(name);

    /* skip anything ending in ".tmp" */
    if (namelen >= 4 && !strcmp(name + (namelen-4), ".tmp"))
        return 0;

    if (do_stat) {
        if (lstat(name, &statbuf))
            die_err(1, "lstat(%s)", name);

        /* we only care about sockets */
        if (!S_ISSOCK(statbuf.st_mode))
            return 0;
    }

    return 1;
}


static inline void
print_name(const char *name) {
    printf("%s\n", name);
    fflush(stdout);
}


static void
process_events(const char *buf, ssize_t len) {
    const struct inotify_event *event;

    for (const char *ptr = buf; ptr < buf + len;
            ptr += sizeof(struct inotify_event) + event->len) {

        event = (const struct inotify_event *) ptr;

        if (!(event->mask & (IN_CREATE|IN_MOVED_TO)) || !event->len)
            continue;

        if (is_interesting(event->name, 1))
            print_name(event->name);
    }
}


static void
handle_events(int fd, int wd, int argc, char* argv[])
{
    /* Align buffer to inotify_event */
    char buf[4096]
        __attribute__ ((aligned(__alignof__(struct inotify_event))));
    ssize_t len;

    for (;;) {
        len = read(fd, buf, sizeof(buf));
        if (len == -1 && errno != EAGAIN) {
            die_err(EXIT_FAILURE, "read");
        }

        if (len <= 0)
            break;

        process_events(buf, len);

    }
}


static void
cycle_direntries(const char *dirname) {
    DIR *d;
    struct dirent *dentry;
    char is_printworthy;
    int errno_save;

    if (!(d = opendir(dirname)))
        die_err(1, "opendir");

    errno = 0;
    while ((dentry = readdir(d))) {
        is_printworthy = 0;
        switch (dentry->d_type) {
            case DT_UNKNOWN:
                is_printworthy = is_interesting(dentry->d_name, 1);
                break;
            case DT_SOCK:
                /* can skip the stat */
                is_printworthy = is_interesting(dentry->d_name, 0);
                break;
            default:
                continue;
        }

        if (is_printworthy)
            print_name(dentry->d_name);
    }

    errno_save = errno;
    (void) closedir(d); // may alter errno
    if (errno_save) {
        errno = errno_save;
        die_err(1, "readdir");
    }

}


/* TODO: Add options:
 *   -n N   read N items and exit
 *   -1     shorthand for -n 1
 *   -m     monitor-only mode (don't cycle through dir)
 */
int
main(int argc, char* argv[])
{
    int notify_fd, poll_num;
    int watcher;
    struct pollfd fdset;
    const char *runtime_dir = get_runtime_dir();
    
    notify_fd = inotify_init1(IN_NONBLOCK);
    if (notify_fd == -1)
        die_err(1, "inotify_init1");

    if (chdir(runtime_dir))
        die_err(1, "chdir(%s)", runtime_dir);

    watcher = inotify_add_watch(notify_fd, ".",
            IN_CREATE|IN_MOVED_TO);
    if (watcher < 0) {
        die_err(1, "inotify_add_watch(%s)", runtime_dir);
    }

    cycle_direntries(".");

    fdset.fd = notify_fd;
    fdset.events = POLLIN;

    for (;;) {
        poll_num = poll(&fdset, 1, -1);
        if (poll_num == -1) {
            if (errno == EINTR)
                continue;
            die_err(1, "poll(inotify_fd)");
        }

        if (poll_num > 0 && fdset.revents & POLLIN) {
            handle_events(notify_fd, watcher, argc, argv);
        }
    }

    close(notify_fd);
    return 0;
}
