#ifndef _SLING_PATH_H
#define _SLING_PATH_H

#include <unistd.h>

#include <lx_string.h>

/* total size of sun_path, including \0 */
#define SUN_PATH_SIZE sizeof(((struct sockaddr_un*)0)->sun_path)

char *path_attach(const char *arg1, const char *arg2, const char *arg3);
const char *get_home(void);
int build_socket_path_pid(lx_s *dest, pid_t pid);
int build_socket_path_name(lx_s *dest, const char *socketname);
const char *get_runtime_dir(void);

#endif // _SLING_PATH_H
