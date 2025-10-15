#ifndef NETWORK_H
#define NETWORK_H

/*
 * Network.c/.h holds all networking logic including server, client
 * threading and user handling, along with sharing routing tables 
 * (nav_graph)
 */

#include "util/util.h"
#include "table.h"

void share_routing_table(char* host, int port);
void* handle_user(void* arg);
void* server_thread(void* arg);
void* client_thread(void* arg);

#endif // NETWORK_H
