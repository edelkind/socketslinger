#ifndef _SLING_DESCRIPTOR_H
#define _SLING_DESCRIPTOR_H

ssize_t desc_write(int sd, void *msg_content, size_t msg_len, int send_desc);
ssize_t desc_read(int sd, void *msg_buf, size_t msg_sz, int *recv_desc);

#endif // _SLING_DESCRIPTOR_H
