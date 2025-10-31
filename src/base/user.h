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

#include "util/util.h"

#define ANSI_RESET "\x1b[0m"
#define ANSI_RED "\x1b[31m"
#define ANSI_GREEN "\x1b[32m"
#define ANSI_YELLOW "\x1b[33m"

//#define DEBUG

// TODO:
// - Fix serialization of table (either remove or incorporate logic for
// 	routing between direct nodes with host:port)
// - Update client, server threads to use new data structures, 
//	and simplified logic
// - Remove test messages, users should gossip when connecting
// - General cleanup of code
// - Test gossip logic
// - Split into seperate files

void serialize_table(nav_table_t* table, char* buff);

void deserialize_table(nav_table_t* table, char* buff);

void share_routing_table(char* host, int port);

void run_da(uni_table_t* uni_table, nav_table_t* this_table);

void gossip(user_info_t host, user_info_t user, bool is_client);

void* handle_user(void* arg);

void* server_thread(void* arg);

void* client_thread(void* arg);

int main(int argc, char* argv[]);

#endif // USER_H
