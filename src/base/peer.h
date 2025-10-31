#ifndef PEER_H
#define PEER_H

#include <stdio.h>
#include <stdlib.h>

#include "../util/util.h"

#define MAX_DIRECT_PEERS 3

// What I want to do is to have simple communications between peers (only 
// implement the server, client, threads) then those peers know the peers they are
// directly connected to, then they start gossiping when they turn on and 
// communicate with every peer, including those on other systems
// Peers need to have the same IP mask as the host machine, BD. (ex: GE0/0 = 
// 10.0.1.1, 255.255.255.0; switch is 10.0.1.2, hm is 10.0.1.3, containers/peers
// are 10.0.1.4+)
// Peer's network graph starts out with only the direct nodes, then resizes each
// time a new node/peer is discovered (through gossiping), this requires the peer
// to recognize that there is a peer in the graph recieved that they do not have
// on their graph owned, then resize the graph and add that peer. This is done for
// every container in system, including GE0/1 (10.0.2.x). Decide if "end game" or 
// just continues to run this after the system works (in other words, dont worry
// until after system works and communicates)
// Need to figure out how to communicate with containers across switches, router
#endif // PEER_H
