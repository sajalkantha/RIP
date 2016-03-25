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
/*#include "rlib.h"*/


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
	socket;
	next;
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
Neighbors neighbor;


void ReadFromFile (String file, InfEntry head) {
}

void CreateListenSocket(int IP, int port) {
}

void SenderThread(InfEntry head, RipEntry rip, Neighbors neighbor) {
}

void ReceiverThread(RipEntry rip, Neighbors neighbor) {
}

void HandleUserInput(RipEntry rip, Neighbors neighbor, InfEntry head) {
}

int main() {

}
