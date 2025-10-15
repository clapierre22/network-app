#ifndef GOSSIP_H
#define GOSSIP_H

/*
 * Gossip.c/.h holds the "gossiping" logic for peers. When gossip is 
 * called, the peers share their routing tables (nav_graph) with
 * each other
 */

#include "util/util.h"

void gossip(user_data_t* host, user_data_t* user);

#endif // GOSSIP_H
