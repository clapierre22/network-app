#include "network.h"

// TODO: Wait until gossip logic and rt serialization is completed,
// then implement updated client, server, handle threads

void share_routing_table(char* host, int port)
{
	int sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if (sockfd < 0)
	{
		perror("Error opening sharing rt socket");
		close(sockfd);
		return;
	}

	struct hostent* h gethostbyname(host);
	if (h == NULL)
	{
#ifdef DEBUG
		printf("[DEBUG] Host not found by name\n");
#endif
		close(sockfd);
		return;
	}

	struct sockaddr_in addr;
	memset(&addr, 0, sizeof(addr));
	addr.sin_family = AF_INET;
	memcpy(&addr.sin_addr, h->h_addr, h->h_length);
	addr.sin_port = htons(port);
	
	if (connect(sockfd, (struct sockaddr*)&addr, sizeof(addr)) < 0)
	{
		perror("Error connecting to sharing rt socket\n");
		close(sockfd);
		return;
	}

	char buff[BUFF_MAX];
	pthread_mutex_lock(&user_mutex);
	// Serialize table
	pthread_mutex_unlock(&user_mutex);

	write(sockfd, buff, strlen(buff));
	close(sockfd);

	printf("Shared routing table with %s:%d\n", host, port);
}

void* handle_user(void* arg)
{
	int user_sockfd = *(int*)arg;
	free(arg);

	char buff[BUFF_MAX];
	int n;
	
	struct sockaddr_in user_addr;
	socklen_t user_len = sizeof(user_addr);

	if (getpeername(user_sockfd, (struct sockaddr*)&user_addr, &user_len) != 0) perror("Peer not found");

	n = read(user_sockfd, buff, BUFF_MAX - 1);
	if (n < 0)
	{
		perror("Error reading from socket");
		close(user_sockfd);
		return NULL;
	}

	buff[n] = '\0';

	// TODO: update the gossip logic
	
	close(user_sockfd);
	return NULL;
}

void* server_thread(void* arg)
{
	int port = *(int*)arg;
	struct sockaddr_in addr;

	this_sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if (this_sockfd < 0) perror("Error creating server thread socket");

	// Reuse address by changing socket opt
	int opt = 1;
	setsockopt(this_sockfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

	memset(&addr, 0, sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_family.s_addr = INADDR_ANY;
	addr.sin_port = htons(port);

	if (bind(this_sockfd, (struct sockaddr*)&addr, sizeof(addr)) < 0) perror("Error binding server thread socket");

	if (listen(this_sockfd, 10) < 0) perror("Error listening on server thread socket");

	printf("Listening on port %d\n", port);

	while (RUNNING)
	{
		int* client_sockfd = malloc(sizeof(int));
		*client_sockfd = accept(this_sockfd, NULL, NULL);
		if (*client_sockfd < 0)
		{
			perror("Error accepting user on server thread");
			free(client_sockfd);
			continue;
		}

		pthread t;
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
	int connected = false;
	int retry = 0;

#ifdef DEBUG
	printf("[DEBUG] client_thread starting: host: %s, port (str): %s, port: %d\n", host, args[1], port);
#endif

	free(args);

	while (RUNNING && !connected && retry < MAX_RETRIES)
	{
		int client_sockfd = socket(AF_INET, SOCK_STREAM, 0);
		if (client_sockfd < 0)
		{
			perror("Error creating client socket in client thread");
			retry++;
			sleep(1);
			break;
		}

		struct hostent* h = gethostbyname(host);

		if (h == NULL)
		{
			fprintf(stderr, "Error resolving hostname: %s\n", host);
			close(client_sockfd);
			retry++;
			sleep(1);
			continue;
		}

		struct sockaddr_in addr;
		memset(&addr, 0, sizeof(addr));
		addr.sin_family = AF_INET;
		memcpy(&addr.sin_addr, h->h_addr, h->h_length);
		addr.sin_port = htons(port);

		if (connect(client_sockfd, (struct sockaddr*)&addr, sizeof(addr)) < 0)
		{
			close(client_sockfd);
			retry++;
			sleep(1);
			continue;
		}

		char* msg = "Test message";
		if (write(client_sockfd, msg, strlen(msg)) < 0)
		{
			perror("Error writing to socket");
			close(client_sockfd);
			retry++;
			sleep(1);
			continue;
		}

#ifdef DEBUG
		printf("[DEBUG] Sent test message to %s:%d, waiting for response\n", host, port);
#endif

		printf("%s[Peer %d]%s Connected\n",
				ANSI_GREEN,
				port,
				ANSI_RESET);
		user_info_t peer, this_host;
		strcpy(peer.hostname, host);
		peer.port = port;
		peer.connected = true;

		gethostname(this_host.hostname, NAME_MAX); // This returns localhost, either change or create function so it reads Peer[#]
		this_host.port = this_port;
		this_host.connected = true;

		// TODO: Fix gossip logic
	}

	if (!connected)
	{
		fprintf(stderr, "[Warning] Failed to connect to %s:%d after %d retries\n", host, port, retry);
	}

	return NULL;
}
