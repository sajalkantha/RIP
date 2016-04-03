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
#include <stdint.h>
/*#include "UDPIPInterface.h"
#include "UDPSocket.h"
#include "IPRIPInterface.h"*/
#include <pthread.h>

#define MAX_BACK_LOG (5)
#define MAX_STATES 128
#define MAX_MSG_LENGTH (1300)
#define MAX_LINE_SIZE (1000)
#define MAX_IP_LEN (30)
/*#include "rlib.h"*/


struct inf_entry {
	char* targetIP;
	uint16_t targetPort;
	char* targetInfIP;
	char* myInfIP;
	int myInf;
	struct inf_entry* next;
};
typedef struct inf_entry inf_entry_t;

struct rip_entry{
	char* destIP;
	int nextHop;
	int32_t cost;
	struct rip_entry* next;
};
typedef struct rip_entry rip_entry_t;

struct neighbors{
	char* IP;
	uint16_t port;
	int socket;
	int id;
	int up;
	struct neighbors* next;
};
typedef struct neighbors neighbors_t;

struct entries{
	int32_t cost;
	uint32_t address;
} __attribute__ ((packed)) ;
typedef struct entries entries_t;

struct rip_payload {
	uint16_t command;
	uint16_t num_entries;
	struct entries data[MAX_STATES];
} __attribute__ ((packed)) ;
typedef struct rip_payload rip_payload_t;
	
struct rip_packet {
	uint8_t version_and_headerlen;
	uint8_t tos;
	uint16_t totallen;
	uint16_t id;
	uint16_t fragoffset;
	uint8_t ttl;
	uint8_t protocol;
	uint8_t cksum;
	uint32_t sourceIP;
	uint32_t destIP;
	uint32_t options_and_padding;
	struct rip_payload ripPayload;
} __attribute__ ((packed)) ;
typedef struct rip_packet rip_packet_t;

uint16_t port;
char* IP;
inf_entry_t* infHead;
rip_entry_t* ripHead;
int numEntries;
neighbors_t* neighbor;
int listenSocket;
pthread_t sThread;


int ReadFromFile (char*  file) {
	char line[MAX_LINE_SIZE];
	FILE *node_file = fopen(file, "r");
	int line_cnt = 0;
	int *sock;
	inf_entry_t *head;
	inf_entry_t *cur;

	while (fgets(line, sizeof(line), node_file) != NULL) {
		//printf("\n%s", line);
		if (line_cnt == 0) { // The first line of the file specifies the IP and port for this node: "[IP-address]:[port]" e.g. localhost:17000
			char* token  = strtok(line, ":");
			if (strcmp(token, "localhost") == 0){ // Assign the node the correct IP address form the first line of the file
				IP = "127.0.0.1";
			}else{
				IP=(char*)malloc(MAX_IP_LEN);
				strcpy(IP,token);
				//IP = token;
			}
			token = strtok(NULL, ":");	// Grab part after ':' of the first line
			//printf("port:%s",token);
			port = (uint16_t) atoi(token); // assing the Node the port of that token
			/*sock = (int *)malloc(sizeof(int));
			create_socket(sock);
			bind_node_addr(sock, IP, (uint16_t) port);*/
		}
		else { //Every line after the first line specifies an interface on this node: [IP-address-of-remote-node]:[port-of-remote-node] [VIP of my interface] [VIP of the remote node's inteface]
			cur = (inf_entry_t*)malloc(sizeof(inf_entry_t));
			cur->targetIP=(char*)malloc(MAX_IP_LEN);
			cur->myInfIP=(char*)malloc(MAX_IP_LEN);
			cur->targetInfIP=(char*)malloc(MAX_IP_LEN);
			char* token = strtok(line, ":");
			if (strcmp(token, "localhost") == 0){ // Assign the node the correct IP address form the first line of the file
				cur->targetIP="127.0.0.1";
			}else{
				strcpy(cur->targetIP,token);
				//cur->targetIP=token;
			}
			//printf("1%s\n",token);
			token = strtok(NULL, " \n");
			cur->targetPort = atoi(token); //Port of remote node
			//printf("1%s\n",token);
			token = strtok(NULL, " \n"); // VIP of the node's interface
			if (token == NULL){
				printf("The format fo this file is not correct1");
				return 1;
			}
			//printf("1%s\n",token);
			strcpy(cur->myInfIP,token);
			//cur->myInfIP = token;
			token = strtok(NULL, " \n"); // The VIP of the remote Node's interface
			if (token == NULL){
				printf("The format fo this file is not correct2");
				return 1;
			}
			strcpy(cur->targetInfIP,token);
			//cur->targetInfIP = token;
			cur->myInf = line_cnt;
			if(line_cnt==1){ // Check to see if the head of the interface list has been populated
				
				head = cur; // If head has not been set, set current to head
				infHead = cur;
				//printf("headtargetip:%s",head->targetIP);
			}
			else{
				head->next = cur; // if head has been set, set head->next to cur
				head=head->next;
			}
		}
		line_cnt++;
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

void* SenderThread() {
	int sock;
	struct sockaddr_in* server_addr;
	neighbors_t* runner = neighbor;
	while (!(runner->next==NULL)) {
		if ((sock = socket(AF_INET, SOCK_STREAM/* use tcp */, 0)) < 0) {
			return;
		}
		runner->socket=sock;
		server_addr=(struct sockaddr_in*)malloc(sizeof(struct sockaddr_in));
		inet_aton(runner->IP, server_addr->sin_addr.s_addr); //IP="63.161.169.137"
		server_addr->sin_family = AF_INET;
		server_addr->sin_port = htons(runner->port);
		if (connect(runner->socket, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
			return;
		}
		runner=runner->next;
	}

	rip_entry_t* ripRunner;
	rip_packet_t* ripPacket;
	while(1) {
		runner = neighbor;
		ripRunner = ripHead;
		while (!(runner==NULL)) {
			ripPacket = (rip_packet_t*)malloc(sizeof(rip_packet_t));
			ripPacket->version_and_headerlen=0;
			ripPacket->tos=0;
			ripPacket->totallen=6*32*+32+64*numEntries;
			ripPacket->id=0;
			ripPacket->fragoffset=0;
			ripPacket->ttl=16;
			ripPacket->protocol=200;
			ripPacket->cksum=0;
			inet_aton(IP,ripPacket->sourceIP);
			inet_aton(runner->IP,ripPacket->destIP);
			ripPacket->options_and_padding=0;
			ripPacket->ripPayload.command=2;
			ripPacket->ripPayload.num_entries=numEntries;
			int counter=0;
			while(!(ripRunner==NULL)) {
				ripPacket->ripPayload.data[counter].cost=ripRunner->cost;
				inet_aton(ripRunner->destIP,ripPacket->ripPayload.data[counter].address);
				ripRunner=ripRunner->next;
			}
			//send packet
			if (send(runner->socket, ripPacket, MAX_MSG_LENGTH, 0) < 0) {
				perror("Send error");
				return;
			}
		}
		sleep(5);
	}
}
/*
void ReceiverThread(RipEntry rip, Neighbors neighbor) {
}
*/

void HandleUserInput() {
	char* input = (char*)malloc(MAX_MSG_LENGTH);
	while(1) {
		fgets (input, MAX_MSG_LENGTH, stdin);
		char* first = calloc(strlen(input)+1, sizeof(char));
		strcpy(first, input);
		input=strtok(input,"\n");
		if(strcmp(input,"routes")==0) {
			printf("3");
		}
		else if(strcmp(input,"ifconfig")==0) {
			printf("4");
		}
		else {
			first=strtok(first," ");
			if(strcmp(first,"down")==0) {
				printf("1");
			}
			else if(strcmp(first,"up")==0) {
				printf("2");
			}
			else if(strcmp(first,"send")==0) {
				printf("%s",input);
			}
		}
	}
}

void print_debug() {
	printf("my ip: %s\n", IP);
	printf("my port: %d\n", port);
	inf_entry_t* runner=infHead;
	while (runner!=NULL) {
		printf("interface #%d:\n",runner->myInf);
		printf("target ip: %s\n",runner->targetIP);
		printf("target port: %d\n",runner->targetPort);
		printf("target interface ip: %s\n",runner->targetInfIP);
		printf("my interface ip: %s\n",runner->myInfIP);
		runner=runner->next;
	}
}

int main(int argc, char* argv[]) {
	if (argc!=2) {
		printf("Incorrect usage\n");
		return 1;
	}
	ReadFromFile(argv[1]);
	CreateListenSocket(IP,port);
	print_debug();
	//pthread_create(&sThread, NULL, &SenderThread, NULL); //sThread holds senderThread
	HandleUserInput(NULL,NULL,NULL);
	return 0;
}
