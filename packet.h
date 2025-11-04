#ifndef PACKET_H
#define PACKET_H

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <arpa/inet.h>

#include "util.h"

packet_t* packet_init(packet_type_t type, const user_t* from, const user_t* to, const char* data, uint32_t data_len);
void packet_free(packet_t* pkt);
int packet_serialize(const packet_t* pkt, char* buff, size_t buff_size);
packet_t* packet_deserialize(const char* buff, size_t buff_size);
int packet_send(int sockfd, const packet_t* pkt);
packet_t* packet_receive(int sockfd);

#endif // PACKET_H
