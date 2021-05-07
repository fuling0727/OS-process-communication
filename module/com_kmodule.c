#include "com_kmodule.h"
#include <linux/module.h>
#include <net/sock.h>
#include <linux/netlink.h>
#include <linux/slab.h>
#include <linux/skbuff.h>


#define NETLINK_USER 31

char *t = "queued";
char *t1 = "unqueued";
//char *msg = "hello worldddddd";
//char *msg1 = "Success";
char send_back_msg[256];
int id;
int reg[1001];//0->empty 1->unqueued 2->queued
char message[256];
struct sock *nl_sk = NULL;
struct mailbox *box;

int return_status;//0->fail 1->success
int FindRegistration(char *id_c,char *type_c);
int FindSend(int id, char message[256]);
int FindRecv(int id);
static void hello_nl_recv_msg(struct sk_buff *skb)
{

    struct nlmsghdr *nlh;
    int pid;
    struct sk_buff *skb_out;
    int msg_size;
    int res;
    char *msg1 = "Success";
    char *msg2 = "Fail";
    //msg = kmalloc(sizeof(1024),GFP_KERNEL);
    printk(KERN_INFO "Entering: %s\n", __FUNCTION__);

    nlh=(struct nlmsghdr*)skb->data;
    printk(KERN_INFO "Netlink received msg payload:%s\n",(char*)nlmsg_data(nlh));
    pid = nlh->nlmsg_pid; /*pid of sending process */
    //process strsep
    char *delim = ",.= ";
    char *sepstr = (char*)nlmsg_data(nlh);
    char *substr;
    char *first;

    int count = 0;

    first = strsep(&sepstr, delim);
    printk(KERN_INFO "first:%s\n",first);

    if(strcmp(first,"Registration") == 0)
    {
        char *id_c,*type_c;

        do
        {
            substr = strsep(&sepstr, delim);
            //printk(KERN_INFO "count:%d = %s\n",count,substr);
            if(count == 2)
            {
                id_c = substr;
            }
            if(count == 5)
            {
                type_c = substr;
            }
            count++;
        }
        while(substr);

        return_status = FindRegistration(id_c,type_c);
        if(return_status == 1) box[id].msg_data_count = 0;

    }
    else if(strcmp(first,"Send") == 0)
    {
        return_status = 0;
        char *id_c;
        //printk(KERN_INFO "Here\n");
        count = 0;
        id_c = strsep(&sepstr, delim);
        printk(KERN_INFO "id:%s\n",id_c);
        kstrtoint(id_c,10,&id);
        if(strlen(sepstr) > 255 || reg[id] == 0)
        {
            return_status = 0;
        }
        else
        {
            substr = strsep(&sepstr, delim);
            memset(message,'\0',256);
            //printk(KERN_INFO "sub:%s\n",substr);
            strncpy(message,substr,strlen(substr));
            //printk(KERN_INFO "message:%s\n",message);
            //printk(KERN_INFO "msg count=%d\n",box[id].msg_data_count);
            return_status = FindSend(id,message);
            printk(KERN_INFO "return :%d\n",return_status);

        }

    }
    else if(strcmp(first,"Recv") == 0)
    {
        return_status = 0;
        printk(KERN_INFO "message:%s\n",message);
        char *id_c;
        id_c = strsep(&sepstr, delim);
        kstrtoint(id_c,10,&id);
        //printk(KERN_INFO "id:%d\n",id);
        memset(send_back_msg,'\0',256);
        memset(message,'\0',256);
        return_status = FindRecv(id);

    }



    //msg_size=strlen(msg);
    msg_size = 1024;
    skb_out = nlmsg_new(msg_size,0);

    if(!skb_out)
    {

        printk(KERN_ERR "Failed to allocate new skb\n");
        return;

    }
    nlh=nlmsg_put(skb_out,0,0,NLMSG_DONE,msg_size,0);
    NETLINK_CB(skb_out).dst_group = 0; /* not in mcast group */
    //strncpy(nlmsg_data(nlh),msg,msg_size);
    memset(nlh,'\0',NLMSG_SPACE(1024));
    /*return back to com_app*/
    if(return_status == 1)//success
    {
        //printk(KERN_ERR "Fail.id invalid");

        strncpy(nlmsg_data(nlh),msg1,strlen(msg1));
        printk(KERN_INFO "send back :%s\n",(char*)nlmsg_data(nlh));
    }
    else if(return_status == 2)//message
    {
        strncpy(nlmsg_data(nlh),send_back_msg,strlen(send_back_msg));
        printk(KERN_INFO "send back msg:%s\n",(char*)nlmsg_data(nlh));
        printk(KERN_INFO "mg len:%d\n",strlen((char*)nlmsg_data(nlh)));
    }
    else //fail
    {

        strncpy(nlmsg_data(nlh),msg2,strlen(msg2));
    }
    res=nlmsg_unicast(nl_sk,skb_out,pid);

    if(res<0)
        printk(KERN_INFO "Error while sending bak to user\n");
}
int FindRegistration(char *id_c,char *type_c)
{
    //printk(KERN_INFO "id1=%s\n",id_c);

    if(kstrtoint(id_c,10,&id) >= 0)
    {
        if(reg[id] == 0)
        {

            if(strcmp(type_c,t) == 0)//queued
            {
                reg[id] = 2;
                box[id].type = 1;
                //printk(KERN_INFO "mailbox type: %d",box[id].type);
                return 1;

            }
            else if(strcmp(type_c,t1) == 0)//unqueued
            {
                //printk(KERN_INFO "come in!\n");
                reg[id] = 1;
                box[id].type = 0;
                //printk(KERN_INFO "mailbox type: %d\n",box[id].type);
                return 1;
            }
            //box[id].msg_data_count = 0;

        }
        else
        {

            return 0;
        }
    }
    return 0;
}
int FindSend(int id, char message[256])
{
    if(box[id].type == 0) //unqueued
    {
        if(box[id].msg_data_count == 0) //first input
        {
            //printk(KERN_INFO "head_first=%s\n",box[id].msg_data_tail->buf);
            box[id].msg_data_head = (struct msg_data*)kmalloc(sizeof(struct msg_data),GFP_KERNEL);
            memset(box[id].msg_data_head->buf,'\0',256);
            strncpy(box[id].msg_data_head->buf,message,strlen(message));
            printk(KERN_INFO "head_first2=%s\n",box[id].msg_data_head->buf);
            box[id].msg_data_count = 1;

        }
        else
        {
            //printk(KERN_INFO "message:%s\n",message);
            memset(box[id].msg_data_head->buf,'\0',256);
            printk(KERN_INFO "head=%s\n",box[id].msg_data_head->buf);
            strncpy(box[id].msg_data_head->buf,message,strlen(message));
            printk(KERN_INFO "head=%s\n",box[id].msg_data_head->buf);
            box[id].msg_data_count = 1;
        }
        return 1;
    }
    else //queued
    {
        if(box[id].msg_data_count == 0)
        {
            box[id].msg_data_head = (struct msg_data*)kmalloc(sizeof(struct msg_data),GFP_KERNEL);
            box[id].msg_data_tail = box[id].msg_data_head;
            box[id].msg_data_tail->next = NULL;
            memset(box[id].msg_data_head->buf,'\0',256);
            strncpy(box[id].msg_data_head->buf,message,strlen(message));
            printk(KERN_INFO "newNode:%s\n",box[id].msg_data_head->buf);
            box[id].msg_data_count++;
            return 1;
        }
        else if(box[id].msg_data_count == 3)
        {
            return 0;
        }
        else
        {
            struct msg_data *newNode;
            newNode = (struct msg_data*)kmalloc(sizeof(struct msg_data),GFP_KERNEL);
            box[id].msg_data_tail->next = newNode;
            memset(newNode->buf,'\0',256);
            strncpy(newNode->buf,message,strlen(message));
            box[id].msg_data_tail = newNode;
            printk(KERN_INFO "head:%s\n",box[id].msg_data_head->buf);
            printk(KERN_INFO "tail:%s\n",box[id].msg_data_tail->buf);
            box[id].msg_data_count++;
            return 1;
        }
    }
}
int FindRecv(int id)
{
    if(box[id].msg_data_count == 0 || reg[id] == 0)
    {
        return 0;
    }
    else
    {
        if(box[id].type == 0)//unqueued
        {
            strncpy(send_back_msg,box[id].msg_data_head->buf,strlen(box[id].msg_data_head->buf));
            //printk(KERN_INFO "send back msg:%s\n",send_back_msg);
            box[id].msg_data_count--;
            kfree(box[id].msg_data_head);
            return 2;
        }
        else //queued
        {
            if(box[id].msg_data_count == 1)
            {
                strncpy(send_back_msg,box[id].msg_data_head->buf,strlen(box[id].msg_data_head->buf));
                printk(KERN_INFO "send back msg:%s\n",send_back_msg);
                box[id].msg_data_count--;
                kfree(box[id].msg_data_head);
                return 2;
            }
            else
            {
                struct msg_data *tmp;
                tmp = (struct msg_data*)kmalloc(sizeof(struct msg_data),GFP_KERNEL);
                tmp = box[id].msg_data_head;
                strncpy(send_back_msg,tmp->buf,strlen(tmp->buf));
                box[id].msg_data_head = box[id].msg_data_head->next;
                box[id].msg_data_count--;
                printk(KERN_INFO "send back msg:%s\n",send_back_msg);
                printk(KERN_INFO "head:%s\n", box[id].msg_data_head);
                kfree(tmp);
                return 2;
            }

        }
    }
}
static int __init hello_init(void)
{

    printk("Entering: %s\n",__FUNCTION__);
    //This is for 3.6 kernels and above.
    struct netlink_kernel_cfg cfg =
    {
        .input = hello_nl_recv_msg,
    };
    box = (struct mailbox*)kmalloc(1000*sizeof(struct mailbox),GFP_KERNEL);

    //msg = kmalloc(sizeof(1024),GFP_KERNEL);
    nl_sk = netlink_kernel_create(&init_net, NETLINK_USER, &cfg);
    //nl_sk = netlink_kernel_create(&init_net, NETLINK_USER, 0, hello_nl_recv_msg,NULL,THIS_MODULE);
    if(!nl_sk)
    {

        printk(KERN_ALERT "Error creating socket.\n");
        return -10;

    }

    return 0;
}

static void __exit hello_exit(void)
{

    printk(KERN_INFO "exiting hello module\n");
    netlink_kernel_release(nl_sk);
}

module_init(hello_init);
module_exit(hello_exit);

MODULE_LICENSE("GPL");

