#ifndef UTIL_H
#define UTIL_H

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

#define HOSTNAME_MAX 256
#define IP_MAX 16
#define HASH_SIZE 1024

/*
 * User
 */
typedef struct user
{
	char hostname[HOSTNAME_MAX];
	char ip_addr[IP_MAX];
	uint16_t port;
	uint8_t connected;
	struct user** r_table;
	uint16_t r_table_size;
	uint8_t padding;
} user_t;

typedef struct
{
	user_t* a, b;
} route_t;

/*
 * Hash Table
 */
typedef struct hash_entry
{
	char key[HOSTNAME_MAX]; // hostname
	user_t* user;
	struct hash_entry* next;
} hash_entry_t;

typedef struct
{
	hash_entry_t buckets;
	int size;
} hash_table_t;

/*
 * Packets
 */
typedef enum
{
	CON, // Connect
	GOS, // Gossip
	MSG, // Message
	DCN, // Disconnect
	ACK, // Acknowledge
	ERR  // Error
} packet_type_t;

typedef struct
{
	uint8_t type; // packet_type_t
	uint32_t seq_num;
	uint32_t data_len;
	user_t from;
	user_t to;
} packet_header_t;

typedef struct
{
	packet_header_t header;
	char* data;
} packet_t;

#define PACKET_HEADER_SIZE sizeof(packet_header_t)
#define MAX_PACKET_DATA 8192

// Threading
typedef enum
{
	FOR, // Forward
	REC  // Receive
} thread_type_t;

#endif // UTIL_H
