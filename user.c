#include "user.h"

// TODO:
// - Combine client_thread, server_thread; or otherwise utilize
// 	handle_user more than the other two as there should be one 
// 	connection/thread

/*
 * Global Variables
 */
//user_t direct_users[MAX_DIRECT_USERS];
int this_port = 0;
int this_sockfd;
pthread_mutex_t mtx = PTHREAD_MUTEX_INITIALIZER;
int rt_count = 0;

//void gossip(user_info_t host, user_info_t user, bool is_client)
//{
//	pthread_mutex_lock(&mtx);

//	pthread_mutex_unlock(&mtx);
//}

/*
 * Retrieves user info and fills a user struct pointer
 */
int get_user_info(user_t* user, int sockfd, bool remote)
{
	if (!user) fprintf(stderr, "Null user_t pointer\n");

	// IP
	struct sockaddr_in addr;
	socklen_t addr_len = sizeof(addr);
	if (remote)
	{
		if (getpeername(sockfd, (struct sockaddr*)&addr, &addr_len) != 0)
		{
			fprintf(stderr, "Unable to retrieve peer name\n");
			strcpy(user->ip_addr, "0.0.0.0");
			strcpy(user->hostname, "unknown_host");
			return -1;
		}
	}
	else
	{
		if (getsockname(sockfd, (struct sockaddr*)&addr, &addr_len) != 0)
		{
			fprintf(stderr, "Unable to retrieve local name\n");
			strcpy(user->ip_addr, "0.0.0.0");
			strcpy(user->hostname, "unknown_host");
			return -1;
		}
	}

	if (inet_ntop(AF_INET, &addr.sin_addr, user->ip_addr, IP_MAX) == NULL)
	{
		fprintf(stderr, "Unable to retrieve IP\n");
		strcpy(user->ip_addr, "0.0.0.0");
		strcpy(user->hostname, "unknown_host");
		return -1;
	}

	// Hostname
	struct hostent* host_entry = gethostbyaddr(
        	&addr.sin_addr,
        	sizeof(addr.sin_addr),
        	AF_INET
    	);

    	if (host_entry != NULL && host_entry->h_name != NULL)
    	{
    	    strncpy(user->hostname, host_entry->h_name, HOSTNAME_MAX - 1);
    	    user->hostname[HOSTNAME_MAX - 1] = '\0';
    	}
    	else
    	{
        	// Fallback: use IP as hostname
        	strncpy(user->hostname, user->ip_addr, HOSTNAME_MAX - 1);
    	    user->hostname[HOSTNAME_MAX - 1] = '\0';
    	}

	return 0;
}

void* handle_user(void* arg)
{
	int user_sockfd = *(int*)arg;
	free(arg);

	char buff[BUFF_MAX];
	int n;
	struct sockaddr_in user_addr;
	socklen_t user_len = sizeof(user_addr);

	user_t* from = calloc(1, sizeof(user_t));
	user_t* to = calloc(1, sizeof(user_t));

	if (!from || !to) 
	{
		perror("Failed to allocate memory");
		free(from);
		free(to);
		close(user_sockfd);
		return NULL;
	}

	if (getpeername(user_sockfd, (struct sockaddr*)&user_addr, &user_len) != 0) perror("Peer not found");

	packet_t* recv_pkt = packet_receive(user_sockfd);

	if (get_user_info(from, user_sockfd, true) || get_user_info(to, user_sockfd, false))
	{
		fprintf(stderr, "Error retrieving user info\n");
		free(from);
		free(to);
		close(user_sockfd);
		return NULL;
	}
	
//	strncpy(from->hostname, recv_pkt->header.from.hostname, HOSTNAME_MAX - 1);
//	strncpy(from->ip_addr, recv_pkt->header.from.ip_addr, IP_MAX - 1);
	from->port = recv_pkt->header.from.port;
	from->connected = recv_pkt->header.from.connected;
//	
//	strncpy(to->hostname, recv_pkt->header.to.hostname, HOSTNAME_MAX - 1);
//	strncpy(to->ip_addr, recv_pkt->header.to.ip_addr, IP_MAX - 1);
	to->port = this_port;
	to->connected = 1;
	
	switch (recv_pkt->header.type)
	{
		case (MSG):
#ifdef DEBUG
			printf("[User %s]->[User %s]: %s\n",
					from->ip_addr,
					to->ip_addr,
					recv_pkt->data);
#endif	
			break;
		case (ACK):
#ifdef DEBUG
			printf("[User %s]: ACK\n", 
					from->ip_addr);
#endif
			break;
		default:
			printf("Warning: Unknown Packet Type\n");
	}

	// Check if gossip msg

	packet_t* ack_pkt = packet_init(ACK, to, from, NULL, 0);
	n = packet_send(user_sockfd, ack_pkt);
	if (n < 0)
	{
		perror("Error sending ACK packet");
		close(user_sockfd);
		return NULL;
	}

	close(user_sockfd);
	packet_free(recv_pkt);
	packet_free(ack_pkt);
	free(from);
	free(to);

	return NULL;
}

void* server_thread(void* arg)
{
	int port = *(int*)arg;
	struct sockaddr_in addr;
	
	this_sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if (this_sockfd < 0) perror("Error creating socket");

	int opt = 1;
	setsockopt(
		this_sockfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

	memset(&addr, 0, sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = INADDR_ANY;
	addr.sin_port = htons(port);

	if (bind(this_sockfd, (struct sockaddr*)&addr, sizeof(addr)) < 0)
		perror("Error binding socket");

	if (listen(this_sockfd, 10) < 0) perror("Error listening on socket");
	printf("Listening on port %d\n", port);

	while(RUNNING)
	{
		int* client_sockfd = malloc(sizeof(int));
		*client_sockfd = accept(this_sockfd, NULL, NULL);
		if (*client_sockfd < 0)
		{
			perror("Error accepting user");
			free(client_sockfd);
			continue;
		}

		pthread_t t;
		pthread_create(&t, NULL, handle_user, client_sockfd);
		#ifdef DEBUG
			printf("[DEBUG] Created thread for handle_user\n");
		#endif
		pthread_detach(t);
	}

	return NULL;
}

void* client_thread(void* arg)
{
	char** args = (char**)arg;
        char* host = args[0];
        int port = atoi(args[1]);
	int count = 0;
	int user_id = port;
	bool connected = false;
	bool running = true;
	int retry_count = 0;

	#ifdef DEBUG
		printf("[DEBUG] client_thread starting: host=%s, port_str=%s, port=%d\n", 
               host, args[1], port);
	#endif

	free(args);

	while(running && !connected && retry_count < MAX_RETRIES)
	{
		int client_sockfd = socket(AF_INET, SOCK_STREAM, 0);
		if (client_sockfd < 0) 
		{
			perror("Error creating client socket");
			retry_count++;
			sleep(1);
			break;
		}
		
		struct hostent* h = gethostbyname(host);

		if (h == NULL)
		{
			//perror("Error retrieving host by name");
			fprintf(stderr, "Error resolving hostname: %s\n", host);
			close(client_sockfd);
			retry_count++;
			sleep(1);
			continue;
		}
		
		struct sockaddr_in addr;
		memset(&addr, 0, sizeof(addr));
		addr.sin_family = AF_INET;
		memcpy(&addr.sin_addr, h->h_addr, h->h_length);
		addr.sin_port = htons(port);

		if (connect(
			client_sockfd, 
			(struct sockaddr*)&addr, sizeof(addr)) < 0)
		{
			close(client_sockfd);
			retry_count++;
			sleep(1);
			continue;
		}

		struct sockaddr_in peer_addr;
		socklen_t peer_len = sizeof(peer_addr);
		char peer_ip[INET_ADDRSTRLEN];
		if (getpeername(client_sockfd, (struct sockaddr*)&peer_addr, &peer_len) == 0)
		{
			inet_ntop(AF_INET, &peer_addr.sin_addr, peer_ip, INET_ADDRSTRLEN);
		}

#ifdef DEBUG
			printf("[DEBUG] Sent test message to %s:%d, waiting for response...\n", host, port);
#endif

		// TODO: Add ip_addr to peer, host
	
		user_t* peer = calloc(1, sizeof(user_t));
		user_t* this_host = calloc(1, sizeof(user_t));
		if (get_user_info(peer, client_sockfd, true) 
			|| get_user_info(this_host, client_sockfd, false))
		{
			fprintf(stderr, "Error retrieving user info\n");
			free(peer);
			free(this_host);
			close(client_sockfd);
			retry_count++;
			continue;
		}

//		strcpy(peer.hostname, host);
		peer->port = port;
		peer->connected = 1;
//		
//		gethostname(this_host.hostname, NAME_MAX);
		this_host->port = this_port;
		this_host->connected = 1;


		char* msg = "Test Message";
		packet_t* pkt = packet_init(MSG, this_host, peer, msg, (uint32_t)strlen(msg));

		if (!pkt) 
		{
			fprintf(stderr, "Failed to create packet\n");
			close(client_sockfd);
			retry_count++;
			continue;
		}

		int n = packet_send(client_sockfd, pkt);
		if (n < 0)
		{
			close(client_sockfd);
			packet_free(pkt);
			retry_count++;
			sleep(1);
			continue;
		}

		printf("%s[User %s]%s->%s[User %s]%s: %s\n",
			ANSI_GREEN,
			this_host->ip_addr,
			ANSI_RESET,
			ANSI_YELLOW,
			peer->ip_addr,
			ANSI_RESET,
			pkt->data);

		packet_free(pkt);

		// Share routing table
		#ifdef DEBUG
			printf("[DEBUG] Shared Message from %s to %s:%d\n", this_host->hostname, peer->ip_addr, peer->port);
		#endif

		// Receive routing table
	
		packet_t* recv_pkt = packet_receive(client_sockfd);

		if (recv_pkt)
		{
#ifdef DEBUG
			printf("Recieved ACK packet from %s:%d; %s\n", host, port, peer->ip_addr);
#endif
			printf("[User %s]->[User %s]: ACK\n", peer->ip_addr, this_host->ip_addr);
			packet_free(recv_pkt);
		}
		
		free(peer);
		free(this_host);
		close(client_sockfd);
		connected = true;
		sleep(1);
	}

	if (!connected)
        {
                fprintf(stderr, 
		"[Warning] Failed to connect to %s:%d after %d retries\n", 
                        host, port, MAX_RETRIES);
        }

	return NULL;
}

int main(int argc, char* argv[])
{
	setbuf(stdout, NULL);
	setbuf(stderr, NULL);

	printf("Good build\n");

	if (argc < 2) 
		perror("Usage: [this_port] [user_hostname] [user_port]");

	int port = atoi(argv[1]);
	this_port = port;

	pthread_t server_t;
	pthread_create(&server_t, NULL, server_thread, &port);

	if (argc >= 4)
	{
		// Prevent duplicate threads
		sleep(1);
		// Count peers by pairs (hostname, port)
		int num_peers = (argc - 2) / 2;

		printf("[DEBUG] argc=%d, num_peers=%d\n", argc, num_peers);

		for (int i = 0; i < num_peers; i++)
		{
			char** user_args = malloc(2 * sizeof(char*));
			// Hostname
			user_args[0] = argv[2 + i * 2];
			// Port
			user_args[1] = argv[2 + i * 2 + 1];

			pthread_t client_t;
			pthread_create(&client_t, NULL, client_thread, user_args);
			pthread_detach(client_t);
		}
	}

	pthread_join(server_t, NULL);

	return 0;
}
