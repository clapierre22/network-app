#ifndef UTIL_H
#define UTIL_H

/*
 * Util.h contains all "global" defs, structs, and enums
 */

#define BUFF_MAX 1024
#define HOSTNAME_MAX 256
#define MAX_DIRECT_USERS 3
#define MAX_TOTAL_USERS 256
#define MAX_RETRIES 30
#define RUNNING 1

// nav_graph:
// u 1 2 3 4 5 6
// 1 0 0 0 0 0 0
// 2 0 0 0 0 0 0
// 3 0 0 0 0 0 0
// 4 0 0 0 0 0 0
// 5 0 0 0 0 0 0
// 6 0 0 0 0 0 0

typedef struct
{
	char hostname[HOSTNAME_MAX];
	int port;
	bool connected;
	//nav_table_t routing_table;
} user_t;

typedef struct
{
	user_t host, dest;
} nav_route_t;

typedef struct
{
	nav_route_t direct_routes[MAX_DIRECT_USERS];
	int nav_graph[MAX_TOTAL_USERS][MAX_TOTAL_USERS];
} nav_table_t;

typedef struct
{
	user_t user;
	nav_table_t routing_table;
} user_data_t;
// PROBLEM: check the logic on the tables, should hold 3 direct connections (maybe new struct nav_connection_t) which are what the peer can actually accesss, then the routes are how to get to non direct peers, which the table holds
//
// not 1 for true, 0 for false, but rather num from node id (node0)
//
// TODO: Implement proper graph functionality
// List of direct neighbors
// Create graph by gossip with neighbors to find their neighbors
// When need to route, find route through D's A then contact the direct 
// neighbor and tell them to pass on msg

#endif // UTIL_H
