#define MAX_ROUTES 128 /* maximum size of routing table */
#define MAX_TTL 120 /* time (in seconds) until route expires */


uint16_t command;
uint16_t num_entries;
struct {
	uint32_t cost;
	uint32_t address;
} entries[num_entries];



/* below this is all example code from book, we will have to modify this since we don't have NodeAddr, etc.*/

typedef struct {
	NodeAddr Destination; /* address of destination */
	NodeAddr NextHop; /* address of next hop */
	int Cost; /* distance metric */
	u_short TTL; /* time to live */
} Route;

int numRoutes = 0;
Route routingTable[MAX_ROUTES];

void mergeRoute (Route *new) {
	int i;
	for (i = 0; i < numRoutes; ++i) {
		if (new->Destination == routingTable[i].Destination) {
			if (new->Cost + 1 < routingTable[i].Cost) {
				/* found a better route: */
				break;
			} 
			else if (new->NextHop == routingTable[i].NextHop) {
				/* metric for current next-hop may have changed: */
				break;
			}
			else {
				/* route is uninteresting---just ignore it */
				return;
			}
		}
	}
	if (i == numRoutes) {
		/* this is a completely new route; is there room for it? */
		if (numRoutes < MAXROUTES) {
			++numRoutes;
		} 
		else {
			/* can't fit this route in table so give up */
			return;
		}
	}
	routingTable[i] = *new;
	/* reset TTL */
	routingTable[i].TTL = MAX_TTL;
	/* account for hop to get to next node */
	++routingTable[i].Cost;
}

void updateRoutingTable (Route *newRoute, int numNewRoutes) {
	int i;
	for (i=0; i < numNewRoutes; ++i) {
		mergeRoute(&newRoute[i]);
	}
}
