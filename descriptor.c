#include <stdio.h>
#include <string.h>
#include <sys/socket.h>

#include "descriptor.h"

/* XXX: requires MSGHDR_MSG_CONTROL.  Consider porting to systems
 * that lack this but still allow descriptor passing.
 */

ssize_t desc_write(int sd, void *msg_content, size_t msg_len, int send_desc) {
    struct msghdr msg = { 0 };
    struct cmsghdr *cmsg_ptr;

    /* only 1 iovec message */
    struct iovec io = {
        .iov_base = msg_content,
        .iov_len = msg_len,
    };

    /* union ensures proper alignment of buf */
    union {
        struct cmsghdr _cmsg_aligner;
        char buf[CMSG_SPACE(sizeof(send_desc))]; /* sizeof(cmsghdr) + size */
    } control_un;

    msg.msg_iov = &io;
    msg.msg_iovlen = 1;
    msg.msg_control = control_un.buf;
    msg.msg_controllen = sizeof(control_un.buf);

    cmsg_ptr = CMSG_FIRSTHDR(&msg); /* (struct cmsghdr *)msg.msg_control */
    cmsg_ptr->cmsg_level = SOL_SOCKET;
    cmsg_ptr->cmsg_type = SCM_RIGHTS;
    cmsg_ptr->cmsg_len = CMSG_LEN(sizeof(send_desc)); /* aligned size */

    *( (int *)CMSG_DATA(cmsg_ptr) ) = send_desc; /* data comes after cmsghdr */

    return sendmsg(sd, &msg, 0);
}


ssize_t desc_read(int sd, void *msg_buf, size_t msg_sz, int *recv_desc) {
    struct msghdr msg = {0};
    struct cmsghdr *cmsg_ptr;
    int recv_len;

    /* union ensures proper alignment of buf */
    union {
        struct cmsghdr _cmsg_aligner;
        char buf[CMSG_SPACE(sizeof(*recv_desc))]; /* sizeof(cmsghdr) + size */
    } control_un;

    struct iovec io = {
        .iov_base = msg_buf,
        .iov_len = msg_sz,
    };
    msg.msg_iov = &io;
    msg.msg_iovlen = 1;

    msg.msg_control = control_un.buf;
    msg.msg_controllen = sizeof(control_un.buf);

    recv_len = recvmsg(sd, &msg, 0);
    if (recv_len <= 0)
        return recv_len;

    cmsg_ptr = CMSG_FIRSTHDR(&msg);

    *recv_desc = *((int*) CMSG_DATA(cmsg_ptr));

    return recv_len;
}

/* relay a descriptor to dual descriptors a la stdio.
 *
 * Normally, d_in and d_out would be 0 and 1 (i.e. actually stdio).
 *
 * Returns 0 on success, or -1 if a read/write error occurred.
 */
int desc_relay(int desc, int d_in, int d_out) {
    // XXX: TODO  (should be impossible to get here until it's implemented)
    fprintf(stderr, "Descriptor relaying to stdio (without a program to exec) is unimplemented.  Sorry.\n\n");
    return -1;
}
