#include <sys/socket.h>
#include <sys/ioctl.h>
#include <sys/time.h>

#include <asm/types.h>
#include <netinet/in.h>
#include <net/if.h>
#include <arpa/inet.h>
#include <asm/types.h>
#include <math.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>

#include <linux/if_packet.h>
#include <linux/if_ether.h>
#include <linux/if_arp.h>

#define PROTO_ARP 0x0806
#define ETH2_HEADER_LEN 14
#define HW_TYPE 1
#define PROTOCOL_TYPE 0x800
#define ARP_REQUEST 0x01
#define ARP_REPLY 0x02

#define BUF_SIZE 60

struct arp_header{
        unsigned short hw_type;
        unsigned short proto_type;
        unsigned char hw_len;
        unsigned char  proto_len;
        unsigned short opcode;
        unsigned char sender_mac[6];
        unsigned char sender_ip[4];
        unsigned char target_mac[6];
        unsigned char target_ip[4];
};



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
			//printf("%s\n",head);		
			ip=malloc(1+tail-head);
			memcpy(ip,head,tail-head);
		}

	}
	pclose(f);
	printf("Gateway address is : %s\n", ip);
	return 0;
	
}

int getMyMacAddress(unsigned char* mac){
	int fd;
	unsigned char * ip;
	struct ifreq ifr;
	unsigned char* name="enp0s8";
	int i;
	fd=socket(AF_INET, SOCK_DGRAM,0);
	if(fd==-1){
		exit(1);
	}
	ifr.ifr_addr.sa_family=AF_INET;
	strncpy(ifr.ifr_name, name, IFNAMSIZ-1);
	if(ioctl(fd, SIOCGIFHWADDR, &ifr)==-1){
		exit(1);
	}
	
	mac=(unsigned char*)ifr.ifr_hwaddr.sa_data;
	printf("Mac address : %.2x:%.2x:%.2x:%.2x:%02x:%02x\n", mac[0],mac[1],mac[2],mac[3],mac[4],mac[5]);
	close(fd);
	return 0;
}

void getMyIpAddress(unsigned char* ip){
	int fd;
	struct ifreq ifr;
	unsigned char* name="enp0s8";
	fd=socket(AF_INET, SOCK_DGRAM, 0);
	if(fd==-1){
		perror("socket():");
		exit(1);
	}
	ifr.ifr_addr.sa_family=AF_INET;
	strncpy(ifr.ifr_name, name, IFNAMSIZ-1);
	//printf("%d",(int)(strlen(gateway)));
	if(ioctl(fd, SIOCGIFADDR, &ifr)==-1){
		perror("SIOCGIFINDEX");
		exit(1);
	}
	
		
	ip=inet_ntoa(((struct sockaddr_in *)&ifr.ifr_addr)->sin_addr);
	
	printf("My IP address is : %s\n", ip);
	
	
close(fd);
	return;
}
int main(int argc, char* argv[])
{
        int sd;
        unsigned char buffer[BUF_SIZE];
	unsigned char* gatewayip;
	unsigned char *myip;
	unsigned char *mymac;
        unsigned char source_ip[4] = {192,168,10,4};//4,10,168,192
        unsigned char target_ip[4] = {192,168,0,1};//1,10,168,192
	unsigned char *name="enp0s8";
	unsigned char *interface;
	
        struct ifreq ifr;
        struct ethhdr *send_req = (struct ethhdr *)buffer;
        struct ethhdr *rcv_resp= (struct ethhdr *)buffer;
        struct arp_header *arp_req = (struct arp_header *)(buffer+ETH2_HEADER_LEN);
        struct arp_header *arp_resp = (struct arp_header *)(buffer+ETH2_HEADER_LEN);
        struct sockaddr_ll socket_address;
        int index,ret,length=0,ifindex;
	/*
	getMyGateway(interface);
	getMyGatewayIp(gatewayip);
	getMyIpAddress(myip);
	getMyMacAddress(mymac);
	printf("djdfkjdf");
*/
	memset(buffer,0x00,60);
        
        sd = socket(AF_PACKET, SOCK_RAW, htons(ETH_P_ALL));
        if (sd == -1) {
                perror("socket():");
                exit(1);
        }
printf("dfdf");
        strncpy(ifr.ifr_name,name, IFNAMSIZ-1);
   		
    if (ioctl(sd, SIOCGIFINDEX, &ifr) == -1) {
        perror("SIOCGIFINDEX");
        exit(1);
    }
    ifindex = ifr.ifr_ifindex;

        /*retrieve corresponding MAC*/
        if (ioctl(sd, SIOCGIFHWADDR, &ifr) == -1) {
                perror("SIOCGIFINDEX");
                exit(1);
        }
close (sd);

        for (index = 0; index < 6; index++)
        {

                send_req->h_dest[index] = (unsigned char)0xff;
                arp_req->target_mac[index] = (unsigned char)0x00;
                /* Filling the source  mac address in the header*/
                send_req->h_source[index] = (unsigned char)mymac[index];
                arp_req->sender_mac[index] = (unsigned char)mymac[index];
                socket_address.sll_addr[index] = (unsigned char)mymac[index];
        }
       
        /*prepare sockaddr_ll*/
        socket_address.sll_family = AF_PACKET;
        socket_address.sll_protocol = htons(ETH_P_ARP);
        socket_address.sll_ifindex = ifindex;
        socket_address.sll_hatype = htons(ARPHRD_ETHER);
        socket_address.sll_pkttype = (PACKET_BROADCAST);
        socket_address.sll_halen =6;
        socket_address.sll_addr[6] = 0x00;
        socket_address.sll_addr[7] = 0x00;

        /* Setting protocol of the packet */
        send_req->h_proto = htons(ETH_P_ARP);

        /* Creating ARP request */
        arp_req->hw_type = htons(HW_TYPE);
        arp_req->proto_type = htons(ETH_P_IP);
        arp_req->hw_len =6;
        arp_req->proto_len =4;
        arp_req->opcode = htons(ARP_REQUEST);
        for(index=0;index<5;index++)
        {
                arp_req->sender_ip[index]=(unsigned char)myip[index];
                arp_req->target_ip[index]=(unsigned char)argv[1][index];
        }
  // Submit request for a raw socket descriptor.
  if ((sd = socket (PF_PACKET, SOCK_RAW, htons (ETH_P_ALL))) < 0) {
    perror ("socket() failed ");
    exit (EXIT_FAILURE);
  }

buffer[32]=0x00;
        ret = sendto(sd, buffer, 42, 0, (struct  sockaddr*)&socket_address, sizeof(socket_address));
        if (ret == -1)
        {
                perror("sendto():");
                exit(1);
        }
        else
        {
                printf(" Sent the ARP REQ \n\t");
                for(index=0;index<42;index++)
                {
                        printf("%02X ",buffer[index]);
                        if(index % 16 ==0 && index !=0)
                        {printf("\n\t");}
                }
        }
printf("\n\t");
        memset(buffer,0x00,60);
        while(1)
        {
                length = recvfrom(sd, buffer, BUF_SIZE, 0, NULL, NULL);
                if (length == -1)
                {
                        perror("recvfrom():");
                        exit(1);
                }
                if(htons(rcv_resp->h_proto) == PROTO_ARP)//arp pacekt일 때
                {
                        if( arp_resp->opcode == ARP_REPLY ){
                        printf(" Received ARP reply : len=%d \n",length);
                        printf(" Sender IP :");
                        for(index=0;index<4;index++)
                                printf("%u.",(unsigned int)arp_resp->sender_ip[index]);

                        printf("\n Sender MAC :");
                        for(index=0;index<6;index++)
                                printf(" %02X:",arp_resp->sender_mac[index]);

                        printf("\nReceiver  IP :");
                        for(index=0;index<4;index++)
                                printf(" %u.",arp_resp->target_ip[index]);

                        printf("\n Self MAC :");
                        for(index=0;index<6;index++)
                                printf(" %02X:",arp_resp->target_mac[index]);

                        printf("\n  :");

                        break;
}
                }
        }

        return 0;
}
