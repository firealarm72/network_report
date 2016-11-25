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
#include <asm/types.h>
#include <linux/if_packet.h>
#include <linux/if_ether.h>
#include <linux/if_arp.h>


#define PROTO_ARP 0X0806
#define HW_TYPE 1
#define PROTOCOL_TYPE 0x800//IPV4
#define ETH2_HEADER_LEN 14
#define ARP_REQUEST 0x01
struct arp_header{
	//etherent header
	//unsigned char dst_mac;
	//unsigned char src_mac;
	//unsigned short ether_type;
	unsigned short hw_type;
	unsigned short proto_type;
	unsigned char hw_len;
	unsigned char proto_len;
	unsigned short opcode;
	unsigned char sender_mac[6];
	unsigned long sender_ip[4];
	unsigned char target_mac[6];
	unsigned long target_ip[4];
};

char *name="enp0s8";
int getMyIpAddress(unsigned char* ipaddress){
	int fd,ifindex;
	struct ifreq ifr;
	
	fd=socket(AF_INET, SOCK_DGRAM, 0);
	if(fd==-1){
		exit(1);
	}
	ifr.ifr_addr.sa_family=AF_INET;
	strncpy(ifr.ifr_name,name, IFNAMSIZ-1);
	if(ioctl(fd, SIOCGIFADDR, &ifr)==-1){
		exit(1);
	}
	close(fd);
	ipaddress=inet_ntoa(((struct sockaddr_in *)&ifr.ifr_addr)->sin_addr);
	ifindex=ifr.ifr_ifindex;
	printf("My IP address is : %s\n", ipaddress);
	return ifindex;
}

int getMyMacAddress(unsigned char* mac){
	int fd;
	struct ifreq ifr;
	
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

int arpRequest(char *srcmac,char *myip, char *victimip, int ifindex){
	int sd;
	int i, ret, length;
	unsigned char buffer[60];
	struct ethhdr *send_req=(struct ethhdr *)buffer;//ehternet header for sending packet
        struct ethhdr *rcv_rpl=(struct ethhdr *)buffer;//ethernet header for receiving packet
        struct arp_header *arp_req=(struct arp_header *)(buffer+ETH2_HEADER_LEN);//arp request arp header
        struct arp_header *arp_rpl=(struct arp_header *)(buffer+ETH2_HEADER_LEN);//arp reply arp header
        struct sockaddr_ll socket_address;
	
	sd = socket(AF_PACKET, SOCK_RAW, htons(ETH_P_ALL));
        if (sd == -1) {
                perror("socket():");
                exit(1);
        }
	
	
	
	for(i=0;i<6;i++){
                send_req->h_dest[i]=(unsigned char)0xff;//ethernet destination ff:ff:ff:ff:ff:ff
                arp_req->target_mac[i]=(unsigned char)0x00;//arp destination mac 00:00:00:00:00:00 arp reqeust통해서 알고싶음
                send_req->h_source[i]=(unsigned char)srcmac[i];//내mac
                arp_req->sender_mac[i]=(unsigned char)srcmac[i];//내 mac
                socket_address.sll_addr[i]=(unsigned char)srcmac[i];
        }
       
	socket_address.sll_family=AF_PACKET;
        socket_address.sll_protocol=htons(ETH_P_ARP);
        socket_address.sll_ifindex=ifindex;
        socket_address.sll_hatype=htons(ARPHRD_ETHER);
        socket_address.sll_pkttype=(PACKET_BROADCAST);
        socket_address.sll_halen=6;
        socket_address.sll_addr[6]=0x00;
        socket_address.sll_addr[7]=0x00;
	
	send_req->h_proto=htons(ETH_P_ARP);

	arp_req->hw_type=htons(HW_TYPE);
        arp_req->proto_type=htons(ETH_P_IP);
        arp_req->hw_len=6;
        arp_req->proto_len=4;
        arp_req->opcode=htons(ARP_REQUEST);
	


        for(i=0;i<5;i++){
                arp_req->sender_ip[i]=(unsigned char)myip[i];//sender ip
                arp_req->target_ip[i]=(unsigned char)victimip[i];//victim ip
        }
	sd= socket(PF_PACKET, SOCK_RAW, htons(ETH_P_ALL));
	if(sd==-1){
		printf("sending socket failed");
	    exit(1);
	}
	//memset(buffer,0x00,60);
	//buffer[32]=0x00;
	//send arp request 
        ret=sendto(sd,buffer,42,0,(struct  sockaddr*)&socket_address, sizeof(socket_address));
        if(ret==-1){
		printf("failed");
                exit(1);
        }
        else{
         	printf("Sent ARP request to %s\n", victimip);
                for(i=0;i<200;i++){
                        printf("%02X ",buffer[i]);
                        if(i%16 ==0 && i!=0){
				printf("\n");
			}
                }
        }

	//memset(buffer, 0x00, 60);
	//receive packet
	/*while(1){
	length=recvfrom(sd,buffer,60,0,NULL,NULL);
	if(length==-1){
		exit(1);
	}
	if(htons(rcv_rply->h_proto)==PROTO_ARP){
	*/	

	while(1){
                length = recvfrom(sd, buffer, 60, 0, NULL, NULL);
                if (length == -1)
                {
                        perror("recvfrom():");
                        exit(1);
                }
                if(htons(rcv_rpl->h_proto) == PROTO_ARP)
                {
                        //if( arp_resp->opcode == ARP_REPLY )
                        printf(" RECEIVED ARP RESP len=%d \n",length);
                        printf(" Sender IP :");
                        for(i=0;i<4;i++)
                                printf("%u.",(unsigned int)arp_rpl->sender_ip[i]);

                        printf("\n Sender MAC :");
                        for(i=0;i<6;i++)
                                printf(" %02X:",arp_rpl->sender_mac[i]);

                        printf("\nReceiver  IP :");
                        for(i=0;i<4;i++)
                                printf(" %u.",arp_rpl->target_ip[i]);

                        printf("\n Self MAC :");
                        for(i=0;i<6;i++)
                                printf(" %02X:",arp_rpl->target_mac[i]);

                        printf("\n  :");

                        break;
                }
        }

	return 0;
	 
	
}
int main(int argc, char *argv[]){//1 : victim ip
	unsigned char *myip, *mymac, *gatename, *gateip;
	int ifindex;

	ifindex=getMyIpAddress(myip);//내 ip 찾기
	getMyMacAddress(mymac);//내 mac 찾기
	getMyGateway(gatename);//default gateway 이름 찾기
	getMyGatewayIp(gateip);//default gateway ip 찾기
	
	arpRequest(mymac, myip,"192.168.10.3",ifindex);
 	
	return 0;
}


