#ifndef _SLING_DESCRIPTOR_H
#define _SLING_DESCRIPTOR_H

#include <sys/types.h>

ssize_t desc_write(int sd, void *msg_content, size_t msg_len, int send_desc);
ssize_t desc_read(int sd, void *msg_buf, size_t msg_sz, int *recv_desc);
int desc_relay(int desc, int d_in, int d_out);

#endif // _SLING_DESCRIPTOR_H
