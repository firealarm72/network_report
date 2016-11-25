#include <winsock2.h>
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "windivert.h"

#define MAXBUF 0xFFFF


const char * pattern = "Michael";
const char * replacement = "Gilbert";

//char* replaceAll(char *string, const char* pattern, const char* repalacement, int* check);
//static void PacketInit(PPACKET packet);

int __cdecl main(int argc, char **argv) {
	HANDLE handle;	//windivert handle
	WINDIVERT_ADDRESS addr;	//packet address
	char packet[MAXBUF];	//packet buffer
	UINT packetLen;
	INT16 priority = 0;
	PWINDIVERT_IPHDR ip_header;
	PWINDIVERT_IPV6HDR ipv6_header;
	PWINDIVERT_ICMPHDR icmp_header;
	PWINDIVERT_ICMPV6HDR icmpv6_header;
	PWINDIVERT_TCPHDR tcp_header;
	PWINDIVERT_UDPHDR udp_header;
	char* payload;
	UINT payload_len;
	int ch = '1';


	//filter : tcp/ip packet with http
	handle = WinDivertOpen("ip and ""(tcp.DstPort==80 or tcp.SrcPort==80)", WINDIVERT_LAYER_NETWORK, priority, 0);

	if (handle == INVALID_HANDLE_VALUE) {
		//HANDLE ERROR
		if (GetLastError() == ERROR_INVALID_PARAMETER) {
			fprintf(stderr, "error:filter syntax error\n");
			exit(EXIT_FAILURE);
		}
		fprintf(stderr, "error:failed to open the Windivert device (%d)\n", GetLastError());
		exit(EXIT_FAILURE);
	}
	
	printf("========Press 'f' to finish the program========\n");
	//capture-modify-inject loop
	while (ch!='f') {//repeat until gets 'f'
		
		if (!WinDivertRecv(handle, packet, sizeof(packet), &addr, &packetLen)) {
			//handle recv error
			fprintf(stderr, "warning : failed to read packet\n");
			continue;
		}
		WinDivertHelperParsePacket(packet, packetLen, &ip_header, &ipv6_header, &icmp_header, &icmpv6_header, &tcp_header, &udp_header, &payload, &payload_len);
		//if it is outbound packet and has payload
		if (addr.Direction == 0 && payload_len>0) {
			UINT8 *encoding = strstr(payload, "Accept-Encoding: ");
			
			if (encoding != NULL) {
				//printf("Found encoding\n");
				UINT8 *start = strstr(payload, "gzip");
				if (start != NULL ) {
					//printf("Found gzip\n");
					
					memcpy(start, " ", sizeof(" ") - 1);
					//printf("memcpy succeed");
					WinDivertHelperCalcChecksums(packet, packetLen, 0);
				}
			}
		}
		//if it is inbound packet and has payload
		else if(addr.Direction==1 && payload_len>0) {
			UINT8 *start = strstr(payload, pattern);
			if (start != NULL) {
				printf("Found %s\n", pattern);
				memcpy(start, replacement, sizeof(replacement) - 1);
				WinDivertHelperCalcChecksums(packet, packetLen, 0);
			}
		}
		if (!WinDivertSend(handle, packet, packetLen, &addr, NULL)) {
			fprintf(stderr, "error : failed to send packet(%d)\n", GetLastError());
			continue;
		}
		//if keyboard input
		if (kbhit()) {
			ch = getch();
		}
		

	}
}