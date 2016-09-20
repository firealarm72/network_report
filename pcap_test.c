#include <stdio.h>
#include <pcap.h>
#include <libnet.h>
#include <stdlib.h>
#include <string.h>
#include <netinet/ip.h>
#include <netinet/tcp.h>
#include <netinet/in.h>
#include <stdint.h>
int count=1;
static int loop_count=20;//패킷 개수
void callback(u_char *useless, const struct pcap_pkthdr *header, const u_char *packet){
	static int count =1;
	struct libnet_ethernet_hdr *ehdr;
	u_int16_t ether_type;
	struct libnet_ipv4_hdr *ihdr;
	struct libnet_tcp_hdr *thdr;
	int chcnt=0;
	int length=header ->len;
	//이더넷 헤더 가져오기
	ehdr = (struct libnet_ethernet_hdr *)packet;
	printf("\n\n#####################%d번째 패킷#################\n",count++);

	
	printf("==================Ethernet Header=================\n");
	printf("Destination MAC: %02x:%02x:%02x:%02x:%02x:%02x\n",ehdr->ether_dhost[0], ehdr->ether_dhost[1], ehdr->ether_dhost[2], ehdr->ether_dhost[3], ehdr->ether_dhost[4], ehdr->ether_dhost[5] );
	 printf("Source MAC: %02x:%02x:%02x:%02x:%02x:%02x\n",ehdr->ether_shost[0], ehdr->ether_shost[1], ehdr->ether_shost[2], ehdr->ether_shost[3], ehdr->ether_shost[4], ehdr->ether_shost[5] );

	

	//ip헤더 가져오기 위해서 오프셋 더하기
	packet +=sizeof(struct libnet_ethernet_hdr);
	ether_type=ehdr ->ether_type;

	//ipv4인 경우
	if(ether_type==0x08){
		ihdr=(struct libnet_ipv4_hdr*)packet;
		printf("==================IP Header===============\n");
		printf("Srouce ip : %s\n",inet_ntoa(ihdr->ip_src));
		printf("Destination ip :%s\n", inet_ntoa(ihdr->ip_dst));
		packet +=ihdr -> ip_hl*4;
		if(ihdr->ip_p==IPPROTO_TCP){//TCP라면
			thdr=(struct libnet_tcp_hdr *)packet;
			printf("Srouce Port Number :%d\n", ntohs(thdr->th_sport));
			printf("Destination Port number :%d\n", ntohs(thdr->th_dport));
		}
		printf("================Payload================\n");	
		while(length--){//데이터 출력
	
			printf("%02x", *(packet++));
			if((++chcnt % 16) ==0)
				printf("\n");
		}

	}
	
}
int main(int argc, char **argv){
	char *dev;
	
	bpf_u_int32 net;
	bpf_u_int32 mask;
	char errbuf[PCAP_ERRBUF_SIZE];
	
	struct pcap_pkthdr header;
	struct bpf_program fp;
	const u_char *packet;
	char filter_exp[] = "port 80";
	pcap_t *handle;

	dev=pcap_lookupdev(errbuf);
	if(dev==NULL){
		fprintf(stderr, "Couldn't find default defice :%s\n", errbuf);
		return(2);
	}
//사용중인 디바이스 이름, 디바이스 없으면 에러보여줌
	printf("DEVICE : %s\n", dev);
	
	if(pcap_lookupnet(dev, &net, &mask, errbuf)==-1){
		fprintf(stderr, "Couldn't get netmask for device %s :%s\n",dev,errbuf);
		net=0;
		mask=0;
	}
	//디바이스 속성 찾기-넷마스크, ip


	handle =pcap_open_live(dev, BUFSIZ, 1, 1000, errbuf);
	if(handle==NULL){
		fprintf(stderr, "Couldn't open device %s : %s\n", dev, errbuf);
		return(2);
	}
	
	//put device into promiscuous mode to sniff until an error occurs
/*
	if(pcap_compile(handle, &fp,filter_exp, 0, net) ==-1){
		fprintf(stderr, "Couldn't install filter %s: %s\n", filter_exp, pcap_geterr(handle));
		return(2);
}
*/
//compile
/*

	if(pcap_setfilter(handle, &fp) ==-1){
	fprintf(stderr, "Couldn't install filter %s: %s\n", filter_exp, pcap_geterr(handle));
	return(2);
}
*/
//apply filter

	

	pcap_loop(handle, loop_count , callback, NULL);
	
	pcap_close(handle);
	return(0);
}

