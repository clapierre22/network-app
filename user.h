#ifndef USER_H
#define USER_H

#define DEBUG

#include "util.h"
//#include "routing.h"
#include "packet.h"

#define ANSI_RESET "\x1b[0m"
#define ANSI_RED "\x1b[31m"
#define ANSI_GREEN "\x1b[32m"
#define ANSI_YELLOW "\x1b[33m"

#define BUFF_MAX 1024
#define IP_MAX 16
#define HOSTNAME_MAX 256
#define MAX_DIRECT_USERS 3
#define MAX_TOTAL_USERS 100
#define RUNNING 1
#define MAX_RETRIES 30

//void gossip(user_info_t host, user_info_t user, bool is_client);
void* handle_user(void* arg);
void* server_thread(void* arg);
void* client_thread(void* arg);

int main(int argc, char* argv[]);

#endif // USER_H
