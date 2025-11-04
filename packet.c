#include "packet.h"

/*
 * Init packet, creates a packet that has all allocated memory, data
 */
packet_t* packet_init(packet_type_t type, 
			const user_t* from, 
			const user_t* to, 
			const char* data, 
			uint32_t data_len)
{
	packet_t* pkt = calloc(1, sizeof(packet_t));
	if (!pkt) return NULL;

	pkt->header.type = type;
	pkt->header.seq_num = 0; // TODO: Implement sequence counting
	pkt->header.data_len = data_len;

	if (from) 
	{
    		strncpy(pkt->header.from.hostname, from->hostname, HOSTNAME_MAX - 1);
    		strncpy(pkt->header.from.ip_addr, from->ip_addr, IP_MAX - 1);
    		pkt->header.from.port = from->port;
    		pkt->header.from.connected = from->connected;
	}
	if (to)
	{
		strncpy(pkt->header.to.hostname, to->hostname, HOSTNAME_MAX - 1);
		strncpy(pkt->header.to.ip_addr, to->ip_addr, IP_MAX - 1);
		pkt->header.to.port = to->port;
		pkt->header.to.connected = to->connected;
	}

	if (data && data_len > 0) 
	{
    		pkt->data = malloc(data_len);
    		if (pkt->data) 
		{
        		memcpy(pkt->data, data, data_len);
    		}
	} else {
    		pkt->data = NULL;
	}

	return pkt;
}

/*
 * Free packet, cleanup including data if present
 */
void packet_free(packet_t* pkt)
{
	if (!pkt) return;
	
	if (pkt->data) free(pkt->data);
	free(pkt);
}

/*
 * Serialize packet, serializes and writes to the buffer
 * Returns the packet size
 */
int packet_serialize(const packet_t* pkt, char* buff, size_t buff_size)
{
	if (!pkt || !buff) return -1;

	size_t total_size = PACKET_HEADER_SIZE + pkt->header.data_len;
	if (buff_size < total_size) return -1;

	packet_header_t n_header = pkt->header;
	n_header.seq_num = htonl(n_header.seq_num);
	n_header.data_len = htonl(n_header.data_len);
	n_header.from.port = htons(n_header.from.port);
	n_header.to.port = htons(n_header.to.port);

	memcpy(buff, &n_header, PACKET_HEADER_SIZE);
	if (pkt->data && pkt->header.data_len > 0)
	{
		memcpy(buff + PACKET_HEADER_SIZE, pkt->data, pkt->header.data_len);
	}

	return total_size;
}

/*
 * Deserialize packet, takes buffer and returns packet with data
 */
packet_t* packet_deserialize(const char* buff, size_t buff_size)
{
	if (!buff || buff_size < PACKET_HEADER_SIZE) return NULL;

	packet_t* pkt = calloc(1, sizeof(packet_t));
	if (!pkt) return NULL;

	memcpy(&pkt->header, buff, PACKET_HEADER_SIZE);
	pkt->header.seq_num = ntohl(pkt->header.seq_num);
	pkt->header.data_len = ntohl(pkt->header.data_len);
	pkt->header.from.port = ntohs(pkt->header.from.port);
	pkt->header.to.port = ntohs(pkt->header.to.port);

	if (pkt->header.data_len > MAX_PACKET_DATA)
	{
		packet_free(pkt);
		return NULL;
	}

	if (pkt->header.data_len > 0)
	{
		if (buff_size < PACKET_HEADER_SIZE + pkt->header.data_len)
		{
			packet_free(pkt);
			return NULL;
		}

		pkt->data = malloc(pkt->header.data_len);
		if (!pkt->data)
		{
			packet_free(pkt);
			return NULL;
		}

		memcpy(pkt->data, buff + PACKET_HEADER_SIZE, pkt->header.data_len);
	}

	return pkt;
}

/*
 * Send packet, given sockfd send given packet
 */
int packet_send(int sockfd, const packet_t* pkt)
{
	char buff[PACKET_HEADER_SIZE + MAX_PACKET_DATA];
	int size = packet_serialize(pkt, buff, sizeof(buff));
	if (size < 0) return -1;

	int n = write(sockfd, buff, size);
	return n == size ? 0 : -1;
}

/*
 * Recieve packet, given sockfd read in buffer and return pointer to packet
 */
packet_t* packet_receive(int sockfd)
{
	char h_buff[PACKET_HEADER_SIZE];
	int n = read(sockfd, h_buff, PACKET_HEADER_SIZE);
	if (n != PACKET_HEADER_SIZE) return NULL;

	packet_header_t r_header;
	memcpy(&r_header, h_buff, PACKET_HEADER_SIZE);
	uint32_t data_len = ntohl(r_header.data_len);

	if (data_len > MAX_PACKET_DATA) return NULL;

	char* buff = malloc(PACKET_HEADER_SIZE + data_len);
	if (!buff) return NULL;

	memcpy(buff, h_buff, PACKET_HEADER_SIZE);
	if (data_len > 0)
	{
		n = read(sockfd, buff + PACKET_HEADER_SIZE, data_len);
		if (n != data_len)
		{
			free(buff);
			return NULL;
		}
	}

	packet_t* pkt = packet_deserialize(buff, PACKET_HEADER_SIZE + data_len);
	free(buff);
	return pkt;
}

//void packet_get_hosts(packet_t* pkt, user_t* from, user_t* to)
//{
	// User From
//	from->ip_addr = pkt->header.from.ip_addr;

	// User To
//	to->ip_addr = pkt->header.to.ip_addr;
//}
