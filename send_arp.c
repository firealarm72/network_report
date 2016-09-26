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

char *name="enp0s3";
int getMyIpAddress(void){
	int fd;
	struct ifreq ifr;
	
	fd=socket(AF_INET, SOCK_DGRAM, 0);
	ifr.ifr_addr.sa_family=AF_INET;
	strncpy(ifr.ifr_name,name, IFNAMSIZ-1);
	ioctl(fd, SIOCGIFADDR, &ifr);
	close(fd);
	printf("%s\n", inet_ntoa(((struct sockaddr_in *)&ifr.ifr_addr)->sin_addr));

	return 0;
}

int getMyMacAddress(void){
	int fd;
	struct ifreq ifr;
	unsigned char *mac;
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

int getMyGateway(void){
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
	
	return 0;	
}

int getMyGatewayIp(void){
	char *gateway=NULL;
	char line[100];
	FILE *f=popen("netstat -rn", "r");
	char *head, *tail;
	char *result;
	char *def="0.0.0.0";
	while(fgets(line, 100, f) !=NULL){
		if(!strncmp(def, line, strlen(def))){
			head=strstr(line,"0.0.0.0         ");
		//	printf("%s\n",head);
			head+=strlen("0.0.0.0         ");
			tail=strstr(head,"0.0.0.0");
			printf("%s\n",head);		
			result=malloc(1+tail-head);
			memcpy(result,head,tail-head);
		}

	}
	pclose(f);
	printf("Gateway address is :%s\n", result);
	return 0;
	
}

int main(int argc, char **argv){
	getMyIpAddress();
	getMyMacAddress();
	getMyGateway();
	getMyGatewayIp();
	return 0;
}


