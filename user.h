#ifndef USER_H
#define USER_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <stdbool.h>
#include <unistd.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <signal.h>

#define ANSI_RESET "\x1b[0m"
#define ANSI_RED "\x1b[31m"
#define ANSI_GREEN "\x1b[32m"
#define ANSI_YELLOW "\x1b[33m"

//#define DEBUG

#define BUFF_MAX 1024
#define HOSTNAME_MAX 256
#define MAX_DIRECT_USERS 3
#define MAX_TOTAL_USERS 100
#define RUNNING 1
#define MAX_RETRIES 30

typedef struct
{
	char hostname[NAME_MAX];
	int port;
	bool connected;
} user_info_t;

// nav_table exists but only to determine which of direct connections to
// send to first, then the same process is run on those connections for 
// their direct connections and so until it reaches destination
typedef struct
{ // Holds the host (for returns), destination, and next jump
  // Also hold step count to prevent infinite loops (end if step>max_users)
	user_info_t host, dest, next;
	int step;
} nav_route_t;

typedef struct // Contains a list of routes to every known user
{
	nav_route_t routes[MAX_DIRECT_USERS];
} nav_table_t;

typedef struct
{ // Contains a list of nav tables, used to run D's A
  // When a transmission with a direct user is made, call gossip()
  // to then add that user's nav_table to the list
  nav_table_t tables[MAX_TOTAL_USERS];
} uni_table_t;

void error(const char* msg);
void status(const char* msg);

nav_route_t create_route(
		user_info_t host, 
		user_info_t dest, 
		user_info_t next,
		int step);
bool table_exists(int host_port);
void update_nav_table(nav_table_t* nav_table, nav_route_t new_route, int i);
void update_uni_table(nav_table_t* new_table);

void serialize_table(nav_table_t* table, char* buff);
void deserialize_table(nav_table_t* table, char* buff);
void share_routing_table(char* host, int port);

int find_node_index(user_info_t* nodes, int node_count, int port);
int find_node(user_info_t* nodes, int count, int port);
int build_graph(uni_table_t* uni_table, user_info_t* nodes, 
		int graph[MAX_TOTAL_USERS][MAX_TOTAL_USERS]);
int min_distance(int dist[], int spt_set[], int node_count);
user_info_t find_next(
		int src[], user_info_t* nodes, int src_idx, int dest_idx);
void run_da(uni_table_t* uni_table, nav_table_t* this_table);

void gossip(user_info_t host, user_info_t user, bool is_client);
void search_direct(user_info_t host);

void* handle_user(void* arg);
void* server_thread(void* arg);
void* client_thread(void* arg);

int main(int argc, char* argv[]);

// TODO: (NOT IN DECLARED ORDER)
// void send_msg()
// void run_da()
// typedef struct nav_table
// void update_table()
// void gossip()
// void* handle_routing()
// void connect_to()

#endif // USER_H
