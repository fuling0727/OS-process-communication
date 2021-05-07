#include "com_app.h"
#include <sys/socket.h>
#include <linux/netlink.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#define NETLINK_USER 31

#define MAX_PAYLOAD 1024 /* maximum payload size*/
struct sockaddr_nl src_addr, dest_addr;
struct nlmsghdr *nlh = NULL;
struct iovec iov;
int sock_fd;
char str[300];
struct msghdr msg;

int main(int argc, char *argv[])
{
    sprintf(str,"Registration. id=%d, type=%s",atoi(argv[1]),argv[2]);
    //puts(str);


    sock_fd=socket(PF_NETLINK, SOCK_RAW, NETLINK_USER);
    if(sock_fd<0)
        return -1;

    memset(&src_addr, 0, sizeof(src_addr));
    src_addr.nl_family = AF_NETLINK;
    src_addr.nl_pid = getpid(); /* self pid */

    bind(sock_fd, (struct sockaddr*)&src_addr, sizeof(src_addr));

    memset(&dest_addr, 0, sizeof(dest_addr));
    memset(&dest_addr, 0, sizeof(dest_addr));
    dest_addr.nl_family = AF_NETLINK;
    dest_addr.nl_pid = 0; /* For Linux Kernel */
    dest_addr.nl_groups = 0; /* unicast */

    nlh = (struct nlmsghdr *)malloc(NLMSG_SPACE(MAX_PAYLOAD));
    memset(nlh, 0, NLMSG_SPACE(MAX_PAYLOAD));
    nlh->nlmsg_len = NLMSG_SPACE(MAX_PAYLOAD);
    nlh->nlmsg_pid = getpid();
    nlh->nlmsg_flags = 0;


    strcpy(NLMSG_DATA(nlh), str);

    iov.iov_base = (void *)nlh;
    iov.iov_len = nlh->nlmsg_len;
    msg.msg_name = (void *)&dest_addr;
    msg.msg_namelen = sizeof(dest_addr);
    msg.msg_iov = &iov;
    msg.msg_iovlen = 1;
    //printf("Sending message to kernel\n");
    sendmsg(sock_fd,&msg,0);
    //printf("Waiting for message from kernel\n");
    memset(nlh, 0, NLMSG_SPACE(MAX_PAYLOAD));//reset string
    //printf("before:%s\n", (char *)NLMSG_DATA(nlh));
    /* Read message from kernel */
    recvmsg(sock_fd, &msg, 0);
    printf("%s\n", (char *)NLMSG_DATA(nlh));
    if(strcmp((char *)NLMSG_DATA(nlh),"Fail!") == 0)
    {
        return -1;
    }

    /*loop here*/

    while(fgets(str,sizeof(str),stdin) )
    {
        //memset(nlh, 0, NLMSG_SPACE(MAX_PAYLOAD));//reset string
        nlh = (struct nlmsghdr *)malloc(NLMSG_SPACE(MAX_PAYLOAD));
        //memset(nlh, 0, NLMSG_SPACE(MAX_PAYLOAD));
        nlh->nlmsg_len = NLMSG_SPACE(MAX_PAYLOAD);
        nlh->nlmsg_pid = getpid();
        nlh->nlmsg_flags = 0;
        if(str[strlen(str)-1]=='\n')
            str[strlen(str)-1]='\0';
        //memset(nlh, 0, NLMSG_SPACE(MAX_PAYLOAD));//reset string
        // printf("Before Received: %s\n", (char *)NLMSG_DATA(nlh));
        strcpy(NLMSG_DATA(nlh), str);

        iov.iov_base = (void *)nlh;
        iov.iov_len = nlh->nlmsg_len;
        msg.msg_name = (void *)&dest_addr;
        msg.msg_namelen = sizeof(dest_addr);
        msg.msg_iov = &iov;
        msg.msg_iovlen = 1;

        //printf("Sending message to kernel\n");
        sendmsg(sock_fd,&msg,0);
        //printf("Waiting for message from kernel\n");

        memset(nlh, 0, NLMSG_SPACE(MAX_PAYLOAD));//reset string
        /* Read message from kernel */
        recvmsg(sock_fd, &msg, 0);
        printf("%s\n", (char *)NLMSG_DATA(nlh));
        //printf("len:%d\n", strlen((char *)NLMSG_DATA(nlh)));


    }
    close(sock_fd);
}


