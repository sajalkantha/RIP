#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <stddef.h>
#include <assert.h>
#include <poll.h>
#include <errno.h>
#include <time.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <sys/uio.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <signal.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <signal.h>
#include <sys/types.h>
#include <stdint.h>
#include <pthread.h>
#include "UDPIPInterface.h"
#include "UDPSocket.h"
#include "IPRIPInterface.h"

#define MAX_BACK_LOG (5)
#define MAX_LINE_SIZE 256
/*#include "rlib.h"*/

/*
struct {
	targetIP;
	targetPort;
	targetInf;
	myInf;
	next;
} InfEntry;

struct {
	destIP;
	nextHop;
	cost;
	nextEntry;
} RipEntry;

struct {
	int socket;
	Neighbors* next;
] Neighbors;

struct {
	int32_t cost;
	uint32_t address;
} entries[num_entries];

struct {
	command;
	num_entries;
	entries data[num_entries];
} RipPayload;
	
struct {
	version;
	headerLen;
	...
	RipPayload;
} RipPacket;

int port;
int IP;
InfEntry head;
RipEntry rip;
Neighbors neighbor;*/
int listenSocket;


nt ReadFromFile (char*  file) {
	char line[MAX_LINE_SIZE];
	FILE *node_file = fopen(file, "r");
	int line_cnt = 0;
	int *sock;
	InfEntry *head;
	InfEntry *cur;

	while (fgets(line, sizeof(line), node_file) != NULL) {
		printf("\n%s", line);

		if (count == 0) { // The first line of the file specifies the IP and port for this node: "[IP-address]:[port]" e.g. localhost:17000
			token  = strtok(line, ":");
			if (strcmp(token, "localhost") == 0){ // Assign the node the correct IP address form the first line of the file
				IP = "127.0.0.1";
			}else{
				IP = token;
			}
			token = strtok(line, ":");	// Grab part after ':' of the first line
			port = (uint16_t) atoi(token); // assing the Node the port of that token

			sock = (int *)malloc(sizeof(int));
			create_socket(sock);
			bind_node_addr(sock, IP, (uint16_t) port);
		}
		else { //Every line after the first line specifies an interface on this node: [IP-address-of-remote-node]:[port-of-remote-node] [VIP of my interface] [VIP of the remote node's inteface]
			token = strtok(line, ":");
			cur->targetIP
			token = strtok(token, " ");
			cur->targetPort = atoi(token); //Port of remote node

			token = strtok(NULL, " "); // VIP of the node's interface
			if (token == NULL){
				printf("The format fo this file is not correct");
				return 1;
			}

			cur->myInf = token;
			token = strtok(NULL, " "); // The VIP of the remote Node's interface
			if (token == NULL){
				printf("The format fo this file is not correct");
				return 1;
			}

			cur->targetInf = token;

			if(head->targetIP == NULL){ // Check to see if the head of the interface list has been populated
				head = cur; // If head has not been set, set current to head
			}
			else{
				head->next = cur; // if head has been set, set head->next to cur
			}

		}

		line_cnt ++  

	}

	fclose(node_file);
	return 0;
	


}

void CreateListenSocket(char* IP, uint16_t port) {
	int sock;
	struct sockaddr_in server_addr, client_addr;
	if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
		perror("Create socket error:");
		return;
	}
	//server_addr.sin_addr.s_addr = INADDR_ANY;
	inet_aton(IP, &server_addr.sin_addr.s_addr); //IP="63.161.169.137"
	server_addr.sin_family = AF_INET;
	server_addr.sin_port = htons(port);
	if (bind(sock,(struct sockaddr*)&server_addr,sizeof(server_addr))<0){
	  perror("Bind socket error:");
	}
	listenSocket=sock;
	listen(sock,MAX_BACK_LOG);
}
/*
void SenderThread(InfEntry head, RipEntry rip, Neighbors neighbor) {
}

void ReceiverThread(RipEntry rip, Neighbors neighbor) {
}

void HandleUserInput(RipEntry rip, Neighbors neighbor, InfEntry head) {
}
*/
int main(int argc, char argv[]) {
	CreateListenSocket("10.0.2.15",8000);
	return 0;
}
