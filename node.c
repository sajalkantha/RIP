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
#include <netinet/in.h>
#include <signal.h>
#include <pthread.h>
#define MAX_BACK_LOG (5)
/*#include "rlib.h"*/


struct inf_entry {
	char* targetIP;
	uint16_t targetPort;
	int targetInf;
	int myInf;
	struct int_entry* next;
};
typedef struct inf_entry inf_entry_t;

struct rip_entry{
	char* destIP;
	int nextHop;
	int cost;
	struct rip_entry* nextEntry;
};
typedef struct rip_entry rip_entry_t;

struct neighbors{
	char* IP;
	uint16_t port;
	int socket;
	struct neighbors* next;
};
typedef struct neighbors neighbors_t;
/*
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
*/
uint16_t port;
char* IP;
inf_entry_t* infHead;
rip_entry_t* ripHead;
neighbors_t* neighbor;
int listenSocket;
pthread_t sThread;

/*
void ReadFromFile (String file, InfEntry head) {
}
*/
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

	while(1) {
		runner = neighbor;
		while (!(runner==NULL)) {
			//do and send update
		}
		sleep(5);
	}
}
/*
void ReceiverThread(RipEntry rip, Neighbors neighbor) {
}

void HandleUserInput(RipEntry rip, Neighbors neighbor, InfEntry head) {
}
*/
int main() {
	CreateListenSocket("10.0.2.15",8000);
	pthread_create(&sThread, NULL, &SenderThread, NULL); //sThread holds senderThread

	return 0;
}
