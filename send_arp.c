#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <netinet/in.h>
#include <net/if.h>
#include <arpa/inet.h>
#include <netpacket/packet.h>
#include <net/ethernet.h>
#include <netdb.h>

struct arp_packet{
	//etherent header
	unsigned char* dst_mac;
	unsigned char* src_mac;
	unsigned short ether_type;
	//arp header
	unsigned short hw_type;
	unsigned short proto_type;
	unsigned char hw_len;
	unsigned char proto_len;
	unsigned short opcode;
	unsigned char sender_mac;
	unsigned long sender_ip;
	unsigned char target_mac;
	unsigned long target_ip;
	char padding[18];
};

char *name="enp0s3";
int getMyIpAddress(unsigned char* ipaddress){
	int fd;
	struct ifreq ifr;
	
	fd=socket(AF_INET, SOCK_DGRAM, 0);
	ifr.ifr_addr.sa_family=AF_INET;
	strncpy(ifr.ifr_name,name, IFNAMSIZ-1);
	ioctl(fd, SIOCGIFADDR, &ifr);
	close(fd);
	ipaddress=inet_ntoa(((struct sockaddr_in *)&ifr.ifr_addr)->sin_addr);
	printf("My IP address is : %s\n", ipaddress);
	return 0;
}

int getMyMacAddress(unsigned char* mac){
	int fd;
	struct ifreq ifr;
	
	int i;
	fd=socket(AF_INET, SOCK_DGRAM,0);
	ifr.ifr_addr.sa_family=AF_INET;
	strncpy(ifr.ifr_name, name, IFNAMSIZ-1);
	ioctl(fd, SIOCGIFHWADDR, &ifr);
	
	mac=(unsigned char*)ifr.ifr_hwaddr.sa_data;
	printf("Mac address : %.2x:%.2x:%.2x:%.2x:%02x:%02x\n", mac[0],mac[1],mac[2],mac[3],mac[4],mac[5]);
	close(fd);
	return 0;
}

int getMyGateway(unsigned char* name){
	FILE *f;
	char line[100], *p, *c;
	f=fopen("/proc/net/route","r");//routing table
	while(fgets(line, 100, f)){
		p=strtok(line, "\t");//tab기준으로 자르기->Iface
		c=strtok(NULL,"\t");//tab기준으으로 자르기->Destination
		
		if(p!=NULL && c!=NULL){
			if(strcmp(c, "00000000")==0){//default gateway
				printf("Default Gateway is :%s\n", p);
				break;
			}
		}
	}
	
	fclose(f);
	name=p;
	return 0;	
}

int getMyGatewayIp(unsigned char* ip){
	char *gateway=NULL;
	char line[100];
	FILE *f=popen("netstat -rn", "r");
	char *head, *tail;
	char *def="0.0.0.0";
	while(fgets(line, 100, f) !=NULL){
		if(!strncmp(def, line, strlen(def))){
			head=strstr(line,"0.0.0.0         ");
			head+=strlen("0.0.0.0         ");
			tail=strstr(head,"0.0.0.0");
			printf("%s\n",head);		
			ip=malloc(1+tail-head);
			memcpy(ip,head,tail-head);
		}

	}
	pclose(f);
	printf("Gateway address is : %s\n", ip);
	return 0;
	
}

int arpRequest(char *srcmac,char *myip, char *victimip){
	int arp_fd,retVal;
	struct arp_packet pkt;

	
	arp_fd=socket(PF_PACKET, SOCK_RAW, htons(ETH_P_ARP));
	if(arp_fd==-1){
		close(arp_fd);
		exit(1);
	}
	
	//ethernet header
	memset(pkt.dst_mac, 0xff, (6*sizeof(unsigned char)));
	memcpy(pkt.src_mac, srcmac, (6*sizeof(unsigned char)));
	pkt.ether_type=htons(ETHER_TYPE_FOR_ARP);
	
	//ARP header
	pkt.hw_type=htons(HW_TYPE_FOR_ETHER);
	pkt.proto_type=htons(PROTO_TYPE_FOR_IP);
	pkt.hw_len=HW_LEN_FOR_ETHER;
	pkt.proto_len=HW_LEN_FOR_IP;
	pkt.opcode=htons(OP_CODE_FOR_ARP_REQ);
	memcpy(pkt.sender_mac,pkt.src_mac, (6*sizeof(unsigned char)));
	pkt.sender_ip=htons(myip);
	memeset(pkt.target_mac,0,(6*sizeof(unsigned char)));
	pkt.target_ip=inet_addr(victimip);
	memset(pkt.padding, 0, (18*sizeof(unsigned char)));

	retVal=sendto(arp_fd, &pkt, sizeof(pkt), 0, (struct socketaddr *)&sa, sizeof(sa));
	if(retVal<0){
		clsoe(arp_fd);
		exit(1);
	}
	printf("packet :%s",pkt);
	return 0;
	 
	
}
int main(int argc, char *argv[]){//1 : victim ip
	unsigned char *myip, *mymac, *gatename, *gateip;
	getMyIpAddress(myip);//내 ip 찾기
	getMyMacAddress(mymac);//내 mac 찾기
	getMyGateway(gatename);//default gateway 이름 찾기
	getMyGatewayIp(gateip);//default gateway ip 찾기
	
	arpRequest(mymac, myip,argv[1]);
 	
	return 0;
}


