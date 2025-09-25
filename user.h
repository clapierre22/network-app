#include <stdio.h>
#include <stdlib.h>
#include <string.h>
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

#define BUFF_MAX 1024
#define NAME_MAX 256
#define MAX_DIRECT_USERS 3
#define RUNNING 1

typedef struct
{
	char hostname[NAME_MAX];
	int port;
	int connected;
} user_info_t;

user_info_t direct_users[MAX_DIRECT_USERS];
int direct_count = 0;
int this_port = 0;
int this_sockfd;
pthread_mutex_t user_mutex = PTHREAD_MUTEX_INITIALIZER;

void error(const char* msg);
void status(const char* msg);
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
