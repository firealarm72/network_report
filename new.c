#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <net/if.h>
#include <netinet/if_ether.h>
#include <sys/socket.h>

static char *device ="enp0s8";

struct arp_packet{
	//ethernet header
	u_char dst_hw_addr[6];
	u_char src_hw_addr[6];
	u_short ether_type;
	//arp header
	u_shor hw_type;
	u_short proto_type;
	u_char hw_addr_size;
	u_char proto_addr_size;
	u_short opcode;
	u_char send_hw_addr[6];
	u_char send_ip_addr[4];
	u_char recv_hw_addr[6];
	u_char recv_ip_addr[4];
	u_char padding[18];
};

int getIpAddress(char* ipaddress);
int getMacAddress(char *macaddress);

int getIpAddress(char* ipaddress){
	int fd;
	struct ifreq ifr;
	struct sockaddr_in sin;
	fd=socket(AF_INET, SOCK_DGRAM, 0);
	if(fd==-1){
		exit(1);
	}
	ifr.ifr_addr.sa_family=AF_INET;
	strncpy(ifr.ifr_name, device , IFNAMSIZ-1);
	if(ioctl(fd, SIOCGIFADDR, &ifr)==-1){
		exit(1);
	}
	close(fd);
	ipaddress=inet_ntoa(((struct sockaddr_in *)&ifr.ifr_addr)->sin_addr);
	ifindex=ifr.ifr_ifindex;
	printf("My IP address is : %s\n", ipaddress);
	return 0;
}

	
