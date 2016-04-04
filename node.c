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
#include <arpa/inet.h>
#include "ipsum.c"
/*#include "UDPIPInterface.h"
#include "UDPSocket.h"
#include "IPRIPInterface.h"*/
#include <pthread.h>

#define MAX_BACK_LOG (5)
#define MAX_STATES 64
#define MAX_MSG_LENGTH (1300)
#define MAX_LINE_SIZE (1000)
#define MAX_IP_LEN (30)
#define BUFSIZE 2048
#define TIMEOUT_LEN (12000)
/*#include "rlib.h"*/


struct inf_entry {
	char* targetIP;
	uint16_t targetPort;
	char* targetInfIP;
	char* myInfIP;
	int myInf;
	int up;
	int socket;
	struct sockaddr_in server_addr;
	struct inf_entry* next;
	pthread_t tid;
};
typedef struct inf_entry inf_entry_t;

struct rip_entry{
	char* destIP;
	int nextHop;
	int32_t cost;
	long timestamp;
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
	struct in_addr address;
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
	struct in_addr sourceIP;
	struct in_addr destIP;
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
pthread_t sThread, rThread;

long long current_timestamp() {
    struct timeval te;
    gettimeofday(&te, NULL);
    long long milliseconds = te.tv_sec*1000LL + te.tv_usec/1000;
    return milliseconds;
}

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
			cur->up=1;
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

void initializeRoutingTable() {
	inf_entry_t* runner = infHead;
	rip_entry_t* rRunner = ripHead;
	while (runner!=NULL) {
		rip_entry_t* rip = (rip_entry_t*)malloc(sizeof(rip_entry_t));
		rip->destIP = runner->targetInfIP;
		rip->nextHop = runner->myInf;
		rip->cost = 1;
		rip->timestamp = current_timestamp();
		rip->next = NULL;
		if (rRunner==NULL) {
				ripHead=rip;
				rRunner=ripHead;
		}
		else {
			rRunner->next=rip;
			rRunner=rRunner->next;
		}
		runner=runner->next;
	}
	runner = infHead;
	while (runner!=NULL) {
		rip_entry_t* rip = (rip_entry_t*)malloc(sizeof(rip_entry_t));
		rip->destIP = runner->myInfIP;
		rip->nextHop = 0;
		rip->cost = 0;
		rip->timestamp = 0;
		rip->next = NULL;
		rRunner->next=rip;
		rRunner=rRunner->next;
		runner=runner->next;
	}
}
void CreateListenSocket(char* IP, uint16_t port) {
	int sock;
	struct sockaddr_in server_addr, client_addr;
	if ((sock = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
		perror("Create socket error:");
		return;
	}
	//server_addr.sin_addr.s_addr = INADDR_ANY;
	inet_aton(IP, &server_addr.sin_addr); //IP="63.161.169.137"
	server_addr.sin_family = AF_INET;
	server_addr.sin_port = htons(port);
	if (bind(sock,(struct sockaddr*)&server_addr,sizeof(server_addr))<0){
	  perror("Bind socket error:");
	}
	listenSocket=sock;
	//listen(sock,MAX_BACK_LOG); //dont need for udp?
}

void sendRipRequests() {
	printf("after sleep, sending rip requests\n");
	inf_entry_t* runner = infHead;
	rip_packet_t* ripPacket;
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
		inet_aton(runner->myInfIP,&ripPacket->sourceIP);
		inet_aton(runner->targetInfIP,&ripPacket->destIP);
		ripPacket->options_and_padding=0;
		ripPacket->ripPayload.command=1;
		ripPacket->ripPayload.num_entries=0;
		ripPacket->cksum=ip_sum((char*)ripPacket,sizeof(rip_packet_t));
		if (runner->up) {
			fflush(stdout);
			//char *my_message = "this is a test message";
			//if (sendto(runner->socket, my_message, strlen(my_message), 0, (struct sockaddr*)(&runner->server_addr), sizeof(runner->server_addr)) < 0) {
			if (sendto(runner->socket, ripPacket, sizeof(rip_packet_t), 0, (struct sockaddr*)(&runner->server_addr), sizeof(runner->server_addr)) < 0) {
				perror("Send request error");
				return;
			}
			//sleep(1);
		}
		runner=runner->next;
	}
}

void sendRipUpdates() {
	inf_entry_t* runner = infHead;
	rip_entry_t* ripRunner;
	rip_packet_t* ripPacket;
	while (!(runner==NULL)) {
		ripRunner = ripHead;
		ripPacket = (rip_packet_t*)malloc(sizeof(rip_packet_t));
		ripPacket->version_and_headerlen=0;
		ripPacket->tos=0;
		ripPacket->totallen=6*32*+32+64*numEntries;
		ripPacket->id=0;
		ripPacket->fragoffset=0;
		ripPacket->ttl=16;
		ripPacket->protocol=200;
		ripPacket->cksum=0;
		inet_aton(runner->myInfIP,&ripPacket->sourceIP);
		inet_aton(runner->targetInfIP,&ripPacket->destIP);
		ripPacket->options_and_padding=0;
		ripPacket->ripPayload.command=2;
		ripPacket->ripPayload.num_entries=numEntries;
		int counter=0;
		while(!(ripRunner==NULL)) {
			if (runner->myInf==ripRunner->nextHop) {
				ripPacket->ripPayload.data[counter].cost=16;
			}
			else {
				ripPacket->ripPayload.data[counter].cost=ripRunner->cost;
			}
			inet_aton(ripRunner->destIP,&ripPacket->ripPayload.data[counter].address);
			ripRunner=ripRunner->next;
			counter++;
		}
		ripPacket->ripPayload.data[counter].cost=-1;
		ripPacket->cksum=ip_sum((char*)ripPacket,sizeof(rip_packet_t));
		//send packet
		if (runner->up) {
			printf("sending...,%d",runner->socket);
			fflush(stdout);
			//char *my_message = "this is a test message";
			//if (sendto(runner->socket, my_message, strlen(my_message), 0, (struct sockaddr*)(&runner->server_addr), sizeof(runner->server_addr)) < 0) {
			if (sendto(runner->socket, ripPacket, sizeof(rip_packet_t), 0, (struct sockaddr*)(&runner->server_addr), sizeof(runner->server_addr)) < 0) {
				perror("Send error");
				return;
			}
			//sleep(1);
		}
		runner=runner->next;
	}
}

void* SenderThread() {
	int sock;
	//struct sockaddr_in server_addr;
	inf_entry_t* runner = infHead;
	while (runner!=NULL) {
		if ((sock = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
			printf("error creating socket");
			continue;
		}
		runner->socket=sock;
		memset((char*)&runner->server_addr, 0, sizeof(runner->server_addr));
		//server_addr=(struct sockaddr_in*)malloc(sizeof(struct sockaddr_in));
		inet_aton(runner->targetIP, &runner->server_addr.sin_addr); //IP="63.161.169.137"
		runner->server_addr.sin_family = AF_INET;
		runner->server_addr.sin_port = htons(runner->targetPort);
		//runner->server_addr=&server_addr;
		/*if (connect(runner->socket, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
			return;
		}*/
		runner=runner->next;
	}
	while(1) {
		sendRipUpdates();
		sleep(5);
	}
}

inf_entry_t* findTargetInfEntry(struct in_addr destIP) {
	inf_entry_t* runner = infHead;
	printf("!!lookup %s\n",inet_ntoa(destIP));
	while(runner!=NULL) {
		if (strcmp(runner->targetInfIP,inet_ntoa(destIP))==0) {
			return runner;
		}
		runner=runner->next;
	}
	printf("returning null inf\n");
	return 0;
}

inf_entry_t* findInfEntry(struct in_addr destIP) {
	inf_entry_t* runner = infHead;
	printf("!!lookup %s\n",inet_ntoa(destIP));
	while(runner!=NULL) {
		if (strcmp(runner->myInfIP,inet_ntoa(destIP))==0) {
			return runner;
		}
		runner=runner->next;
	}
	printf("returning null inf\n");
	return 0;
}

rip_entry_t* findRipEntry(struct in_addr sourceIP) {
	//printf("start find:%s\n",inet_ntoa(sourceIP));
	rip_entry_t* runner = ripHead;
	while(runner!=NULL) {
		//printf("%s\n",runner->destIP);
		if (strcmp(runner->destIP,inet_ntoa(sourceIP))==0) {
			//printf("returnign runner\n");
			return runner;
		}
		runner=runner->next;
	}
	//printf("returning null\n");
	return 0;
}

void* listenForInput() {
	while(1) {
        /*unsigned char buf[BUFSIZE];     /* receive buffer */
        int recvlen;                    /* # bytes received */
        struct sockaddr_in remaddr;     /* remote address */
        socklen_t addrlen = sizeof(remaddr);            /* length of addresses */
        rip_packet_t* ripPacket = (rip_packet_t*)malloc(sizeof(rip_packet_t));
        //printf("waiting to read...\n");
        recvlen = recvfrom(listenSocket, ripPacket, sizeof(rip_packet_t), 0, (struct sockaddr *)&remaddr, &addrlen);
        //recvlen = recvfrom(fd, buf, BUFSIZE, 0, (struct sockaddr *)&remaddr, &addrlen);
        //printf("received %d bytes\n", recvlen);
        if (recvlen > 0) {
        	if (findInfEntry(ripPacket->destIP)->up) {
                //buf[recvlen] = 0;
                printf("received packet, source ip: %s\n", inet_ntoa(ripPacket->sourceIP));
                printf("first entry cost=%d\n",ripPacket->ripPayload.data[0].cost);
                if (ripPacket->ripPayload.command==1) {
                	printf("just a received a rip request, sending triggered response\n");
                	sendRipUpdates();
                	continue;
                }
                int i;
                for (i=0;i<MAX_STATES;i++) {
                	printf("entry%d\n",i);
                	if (ripPacket->ripPayload.data[i].cost==-1) {
                		printf("nulled%d\n",i);
                		break;
                	}
                	rip_entry_t* runner = ripHead;
                	int new=1;
                	while (runner!=NULL) {
                		printf("%s:%s\n",runner->destIP,inet_ntoa(ripPacket->ripPayload.data[i].address));
                		if (strcmp(runner->destIP,inet_ntoa(ripPacket->ripPayload.data[i].address))==0) {
                			printf("%s match %s\n", runner->destIP, inet_ntoa(ripPacket->ripPayload.data[i].address));
                			rip_entry_t* sender = findRipEntry(ripPacket->sourceIP);
                			if (runner->timestamp!=0) {
                				inf_entry_t* isender = findTargetInfEntry(ripPacket->ripPayload.data[i].address);
                				if (isender!=NULL) {
                					if (strcmp(isender->targetInfIP,inet_ntoa(ripPacket->sourceIP))==0) {
                						printf("!!!!!!!!%s update timestamp\n",isender->targetInfIP);
                						runner->timestamp=current_timestamp();
                						runner->cost=1;
                					}
                					else {
                						printf("!!!!!!!%s:%s received indirect rip, no time update\n",isender->targetInfIP,runner->destIP);
                					}
                				}
                				else {
                					printf("!!!!!not neighbor, update ts\n");
            						runner->timestamp=current_timestamp();
                				}
                			}
                			printf("%d+%d<%d?\n",ripPacket->ripPayload.data[i].cost,sender->cost,runner->cost);
                			printf("%s\n",inet_ntoa(ripPacket->ripPayload.data[i].address));
                			inf_entry_t* infOfSender = findTargetInfEntry(ripPacket->sourceIP);
                			if (ripPacket->ripPayload.data[i].cost+sender->cost<runner->cost) { //new path has shorter cost
                				printf("rip updated");
                				runner->cost=(ripPacket->ripPayload.data[i].cost+sender->cost);
                				runner->nextHop=sender->nextHop;
                			}
                			else if (runner->nextHop==infOfSender->myInf) { //old path cost changed
                				printf("path cost changed %d/%d",runner->nextHop,infOfSender->myInf);
                				runner->cost=(ripPacket->ripPayload.data[i].cost+sender->cost);
                			}
                			new=0;
                			break;
                		}
                		runner=runner->next;
                	}
                	if (new==1) {
                		printf("ADDINGGGGGGGGGG\n");
                		rip_entry_t* arunner = ripHead;
                		while(arunner->next!=NULL) {
                			arunner=arunner->next;
                		}
            			rip_entry_t* sender = findRipEntry(ripPacket->sourceIP);
                		rip_entry_t* rip = (rip_entry_t*)malloc(sizeof(rip_entry_t));
                		printf("!%s!\n", inet_ntoa(ripPacket->ripPayload.data[i].address));
                		rip->destIP=(char*)malloc(strlen(inet_ntoa(ripPacket->ripPayload.data[i].address))+1);
                		memcpy (rip->destIP, inet_ntoa(ripPacket->ripPayload.data[i].address), strlen(inet_ntoa(ripPacket->ripPayload.data[i].address))+1);
                		printf("destIP=%s", rip->destIP);
                		rip->nextHop = sender->nextHop;
                		rip->cost = ripPacket->ripPayload.data[i].cost+sender->cost;
                		rip->timestamp = current_timestamp();
                		rip->next = NULL;
                		arunner->next=rip;
                		arunner=arunner->next;
                	}
                }
        	}
        }
        else if (recvlen<0) {
			perror("Receive error");
        }
	}
}

void* ReceiverThread() {
	inf_entry_t* runner = infHead;

	while (runner!=NULL) {
		pthread_create(&runner->tid, NULL, &listenForInput, NULL);
		runner=runner->next;
	}

	rip_entry_t* rRunner;
	rip_entry_t* prev;
	while(1) {
		sleep(12);
		rRunner = ripHead;
		prev = NULL;
		while (rRunner!=NULL) {
			if (rRunner->timestamp!=0) {
				long currentTime = current_timestamp();
				long elapsedTime = currentTime - rRunner->timestamp;
				if (elapsedTime>TIMEOUT_LEN) {
					printf("...rip entry (dest %s) timeout...\n",rRunner->destIP);
					rRunner->cost=16;
					printf("sending triggered update\n");
					sendRipUpdates();
					/*if (prev!=NULL) {
						prev->next=rRunner->next;
						rRunner=prev->next;
						continue;
					}
					else {
						ripHead=ripHead->next;
						rRunner=ripHead;
						continue;
					}*/
				}
			}
			prev=rRunner;
			rRunner=rRunner->next;
		}
	}
}


void HandleUserInput() {
	char* input = (char*)malloc(MAX_MSG_LENGTH);
	int restart=0;
	while(1) {
		fgets (input, MAX_MSG_LENGTH, stdin);
		char* first = calloc(strlen(input)+1, sizeof(char));
		strcpy(first, input);
		input=strtok(input,"\n");
		if(strcmp(input,"routes")==0) {
			rip_entry_t* runner=ripHead;
			while(runner!=NULL) {
				if (runner->cost!=-2) {
					printf("%s\t%d\t%d\n",runner->destIP,runner->nextHop,runner->cost);
				}
				runner=runner->next;
			}
		}
		else if(strcmp(input,"ifconfig")==0) {
			inf_entry_t* runner=infHead;
			while (runner!=NULL) {
				if (runner->up==1) {
					printf("%d\t%s\tup\n",runner->myInf,runner->myInfIP);
				}
				else {
					printf("%d\t%s\tdown\n",runner->myInf,runner->myInfIP);
				}
				runner=runner->next;
			}
		}
		else {
			first=strtok(first," ");
			if(strcmp(first,"down")==0) {
				first=strtok(NULL," ");
				int id = (int)atoi(first);
				if (id==0) {
					printf("invalid interface\n");
					restart=1;
				}
				inf_entry_t* runner=infHead;
				while (runner!=NULL) {
					if (runner->myInf==id) {
						runner->up=0;
						printf("interface %d down\n",id);
						restart=1;
					}
					runner=runner->next;
				}
				if (restart==1) {
					restart=0;
					continue;
				}
				printf("Interface %d not found\n",id);
			}
			else if(strcmp(first,"up")==0) {
				first=strtok(NULL," ");
				int id = (int)atoi(first);
				if (id==0) {
					printf("invalid interface\n");
					restart=1;
				}
				inf_entry_t* runner=infHead;
				while (runner!=NULL) {
					if (runner->myInf==id) {
						runner->up=1;
						printf("interface %d up\n",id);
						restart=1;
					}
					runner=runner->next;
				}
				if (restart==1) {
					restart=0;
					continue;
				}
				printf("Interface %d not found\n",id);
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
		printf("my socket: %d\n",runner->socket);
		runner=runner->next;
	}
}

int main(int argc, char* argv[]) {
	if (argc!=2) {
		printf("Incorrect usage\n");
		return 1;
	}
	ReadFromFile(argv[1]);
	initializeRoutingTable();
	CreateListenSocket(IP,port);
	print_debug();
	pthread_create(&sThread, NULL, &SenderThread, NULL); //sThread holds senderThread
	pthread_create(&rThread, NULL, &ReceiverThread, NULL);
	sleep(1);
	sendRipRequests();
	HandleUserInput(NULL,NULL,NULL);
	return 0;
}
