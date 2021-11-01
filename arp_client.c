#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <netinet/in.h>
#include <linux/if_ether.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <net/if.h>
#include <string.h>

struct arp_message{
	uint16_t hrd; //hardware type
	uint16_t pro; //protocol type. IPv4 = 2048(0x0800)
	uint8_t hln; //hareware address length. Ethernet = 6
	uint8_t pln; //protocol address length. IPv4 = 4
	uint16_t op; //opcode. ARP Req = 1
	uint8_t sha[6]; //sender hardware addr.
	uint8_t spa[4]; //sender protocol addr.
	uint8_t tha[6]; //target hardware addr.
	uint8_t tpa[4]; //target protocol addr.
};

void get_hwaddr(uint8_t *dest ,char *iface){
	struct ifreq ifr;
	uint8_t mac_addr[6];

	int sock=socket(AF_INET, SOCK_DGRAM, IPPROTO_IP);

	ifr.ifr_addr.sa_family=AF_INET;
	strncpy(ifr.ifr_name, iface, sizeof(ifr.ifr_name)-1);
	ioctl(sock, SIOCGIFHWADDR, &ifr);
	close(sock);
	for(int i=0;i<6;i++){
		mac_addr[i]=(uint8_t)ifr.ifr_hwaddr.sa_data[i];
	}
	memcpy(dest, mac_addr, 6);
}

int send_arp(int sock, char *iface){
	unsigned char sender_mac_addr[6], target_mac_addr[6]={0xff, 0xff, 0xff, 0xff, 0xff, 0xff};
	struct arp_message arp_message_buf;
	struct sockaddr_in server_addr;
	uint8_t sender_ip_addr[4]={192, 168, 11, 98}, target_ip_addr[4]={192, 168, 11, 1};

	arp_message_buf.hrd=1;
	arp_message_buf.pro=0x0800;
	arp_message_buf.hln=6;
	arp_message_buf.pln=4;
	arp_message_buf.op=1;
	get_hwaddr(&sender_mac_addr, "enp2s0");
	memcpy(arp_message_buf.sha, sender_mac_addr, 6);
	memcpy(arp_message_buf.spa, sender_ip_addr, 4);
	memcpy(arp_message_buf.tha, target_mac_addr, 6);
	memcpy(arp_message_buf.tpa, target_ip_addr, 4);

	server_addr.sin_family=AF_INET;
	server_addr.sin_addr.s_addr=INADDR_BROADCAST;

	if(sendto(sock, &arp_message_buf, sizeof(arp_message_buf), 0, &server_addr, sizeof(struct sockaddr_in))<0){
		perror("send error\n");
	}
	close(sock);

	return 0;
}

int main(){
	int sock;
	char iface[10];

	strcpy(iface, "wlp1s0");

	/*
	ETH_P_ALL(0x0806) = all protocols are received.

	All incoming packets of that protocol type will be passed to the packet socket 
	before they are passed to the protocols implemented in the kernel.
	*/
	if((sock=socket(AF_PACKET, SOCK_RAW, htons(ETH_P_ALL)))<0){
		perror("socket failed\n");
	}
	send_arp(sock, iface);
	return 0;
}