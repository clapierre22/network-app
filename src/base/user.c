#include "user.h"

/*
 * Global Variables
 */
user_t* direct_users;
user_data_t* this_user;
int this_sockfd;
pthread_mutex_t user_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t table_mutex = PTHREAD_MUTEX_INITIALIZER;

void serialize_table(nav_table_t* table, char* buff)
{
	sprintf(buff, "ROUTES:");
	for (int i = 0; i < MAX_DIRECT_USERS; i++)
	{
		nav_route_t* route = &table->routes[i];
		if (route->dest.port == 0) continue;

		sprintf(buff + strlen(buff), "|%s:%d->%s:%d",
			route->host.hostname, route->host.port,
			route->dest.hostname, route->dest.port);
	}
}

void deserialize_table(nav_table_t* table, char* buff)
{
	memset(table, 0, sizeof(nav_table_t));

	if (strncmp(buff, "ROUTES:", 7) != 0) return;

	char* token = strtok(buff + 7, "|");
	int idx = 0;

	while (token!= NULL && idx < MAX_DIRECT_USERS)
	{
		user_info_t host, dest;

		// Parse buffer: hostname:port->hostname:port
		if (sscanf(token, "%[^:]:%d->%[^:]:%d",
			host.hostname, &host.port,
			dest.hostname, &dest.port) == 4)
		{
			host.connected = true;
			dest.connected = true;
			table->routes[idx++] = create_route(
						host, dest, dest, 1);
		}

		token = strtok(NULL, "|");
	}
}

void share_routing_table(char* host, int port)
{
	int sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if (sockfd < 0) 
	{
		fprintf(stderr, "[Gossip] Error opening socket\n");
		close(sockfd);
		return;
	}

	struct hostent* h = gethostbyname(host);
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
		close(sockfd);
		fprintf(stderr, "[Gossip] Error connecting to socket\n");
		return;
	}

	char buff[BUFF_MAX];
	pthread_mutex_lock(&user_mutex);
	serialize_table(&this_nav_table, buff);
	pthread_mutex_unlock(&user_mutex);

	write(sockfd, buff, strlen(buff));
	close(sockfd);

	printf("[Gossip] Shared routing table with %s:%d\n", host, port);
}

void run_da(uni_table_t* uni_table, nav_table_t* this_table)
{
	pthread_mutex_lock(&table_mutex);

	user_info_t nodes[MAX_TOTAL_USERS];
	int graph[MAX_TOTAL_USERS][MAX_TOTAL_USERS];
	int dist[MAX_TOTAL_USERS];
	int visited[MAX_TOTAL_USERS];
	int parent[MAX_TOTAL_USERS];

	int node_count = build_graph(uni_table, nodes, graph);
	int src = find_node(nodes, node_count, this_port);
	
	for (int i = 0; i < node_count; i++)
	{
		dist[i] = INT_MAX;
		visited[i] = 0;
		parent[i] = -1;
	}

	dist[src] = 0;
	parent[src] = src;

	// Dijkstra's Algorithim
	for (int count = 0; count < (node_count - 1); count++)
	{
		int min = min_distance(dist, visited, node_count);
		if (min == -1) break;

		visited[min] = 1;

		for (int v = 0; v < node_count; v++)
		{
			if (!visited[v] 
				&& graph[min][v]
				&& dist[min] != INT_MAX
				&& dist[min] + graph[min][v] < dist[v])
			{
				dist[v] = dist[min] + graph[min][v];
				parent[v] = min;
			}
		}
	}

	// Build routing table
	int route_idx = 0;
	user_info_t this_node = {.port = this_port};
	strcpy(this_node.hostname, "localhost");

	for (int i = 0; i < node_count && route_idx < MAX_DIRECT_USERS; i++)
	{
		if (i == src || dist[i] == INT_MAX) continue;

		nav_route_t route = create_route(
					this_node,
					nodes[i],
					find_next(parent, nodes, src, i),
					dist[i]);
		update_nav_table(this_table, route, route_idx++);
	}

	printf("[Dijkstra] Computed %d routes\n", route_idx);

	pthread_mutex_unlock(&table_mutex);
}

void gossip(user_data_t host, user_data_t user)
{
	pthread_mutex_lock(&user_mutex);
	
	for (int i = 0; i < MAX_TOTAL_USERS; i++);
	{
		for (int j = 0; j < MAX_TOTAL_USERS; j++);
		{
			host->routing_table.nav_graph[i][j] | user->routing_table.nav_graph[i][j];
			user->routing_table.nav_graph[i][j] | host->routing_table.nav_graph[i][j];
			// Note: Since both host and user should call gossip when connecting, maybe only do the host, user table switch?
		}
	}

	// Call D's A here?
	
	pthread_mutex_unlock(&user_mutex);
}

//{
//	pthread_mutex_lock(&user_mutex);
//
//	bool connected = false;
//	for (int i = 0; i < direct_count; i++)
//	{
//		if (direct_users[i].port == user.port)
//		{
//			connected = true;
//			break;
//		}
//	}
//
//	if (!connected && direct_count < MAX_DIRECT_USERS)
//	{
//		direct_users[direct_count] = user;
//		direct_users[direct_count].connected = true;
//
//		nav_route_t direct_route = create_route(host, user, user, 1);
//		update_nav_table(&this_nav_table, direct_route, direct_count);
//		direct_count++;
//
//		printf("[Gossip] Added direct connection to %s:%d\n",
//				user.hostname, user.port);
// Runs to here at least
//		pthread_mutex_unlock(&user_mutex);
//		update_uni_table(&this_nav_table);
//		
//		// Only share if requested (from client side)
//		//if (is_client)
//		//{
//		//	share_routing_table(user.hostname, user.port);
//		//}
//		
//		run_da(&routing_table, &this_nav_table);
//	} else {
//		pthread_mutex_unlock(&user_mutex);
//	}
//}

void* handle_user(void* arg)
{
	int user_sockfd = *(int*)arg;
	free(arg);

	char buff[BUFF_MAX];
	int n;
	struct sockaddr_in user_addr;
	socklen_t user_len = sizeof(user_addr);

	if (getpeername(user_sockfd, (struct sockaddr*)&user_addr, &user_len) != 0) error("Peer not found");

	n = read(user_sockfd, buff, BUFF_MAX - 1);
	if (n < 0)
	{
		error("Error reading from socket");
		close(user_sockfd);
		return NULL;
	}

	buff[n] = '\0';

	// Check if gossip msg
	if (strncmp(buff, "ROUTES:", 7) == 0)
	{
		nav_table_t new_table;
		deserialize_table(&new_table, buff);

		if (new_table.routes[0].host.port != 0)
		{
			update_uni_table(&new_table);
			printf("[Gossip] Recieved routing table from %s\n",
					inet_ntoa(user_addr.sin_addr));

			run_da(&routing_table, &this_nav_table);
		}

		// Send ACK
		char* msg = "ACK";
		n = write(user_sockfd, msg, strlen(msg));
	} else {
		printf("%s[User %s]%s %s\n",
			ANSI_YELLOW,
			inet_ntoa(user_addr.sin_addr),
			ANSI_RESET,
			buff);
	
		user_info_t peer, host;
		strcpy(peer.hostname, inet_ntoa(user_addr.sin_addr));
		peer.port = ntohs(user_addr.sin_port);
		peer.connected = true;

		gethostname(host.hostname, NAME_MAX); // Retrieves hostname from docker container
		host.port = this_port;
		host.connected = true;

		gossip(host, peer, false);

		// Respond with routing table
		//char* msg = "ACK";
		
		char resp[BUFF_MAX];
		pthread_mutex_lock(&user_mutex);
		serialize_table(&this_nav_table, resp);
		pthread_mutex_unlock(&user_mutex);

		#ifdef DEBUG
			printf("[DEBUG] Sending routing table: '%s' (%zu bytes)\n", resp, strlen(resp));
		#endif

		n = write(user_sockfd, resp, strlen(resp));
		if (n < 0)
		{
			error("Error writing to socket");
		} else {
			printf("[Gossip] Sent routing table to %s\n", inet_ntoa(user_addr.sin_addr));
		}
	}
	close(user_sockfd);
	return NULL;
}

void* server_thread(void* arg)
{
	int port = *(int*)arg;
	struct sockaddr_in addr;
	
	this_sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if (this_sockfd < 0) error("Error creating socket");

	int opt = 1;
	setsockopt(
		this_sockfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

	memset(&addr, 0, sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = INADDR_ANY;
	addr.sin_port = htons(port);

	if (bind(this_sockfd, (struct sockaddr*)&addr, sizeof(addr)) < 0)
		error("Error binding socket");

	if (listen(this_sockfd, 10) < 0) error("Error listening on socket");
	printf("Listening on port %d\n", port);

	while(RUNNING)
	{
		int* client_sockfd = malloc(sizeof(int));
		*client_sockfd = accept(this_sockfd, NULL, NULL);
		if (*client_sockfd < 0)
		{
			error("Error accepting user");
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
	// TODO: edit solution to take three direct peers
	// IDEA: maybe have all direct peers be within the same container, 
	// then they each have a connection to a different container
	//char* host = ((char**)arg)[0];
	//int port = atoi(((char**)arg)[1]);
	char** args = (char**)arg;
        char* host = args[0];
        int port = atoi(args[1]);
	int count = 0;
	int user_id = port;
	bool connected = false;
	bool running = true;
	int retry_count = 0;

	//char** argv = (char**)arg;
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
			//perror("Error connecting to socket");
			close(client_sockfd);
			retry_count++;
			sleep(1);
			continue;
		}

		char* msg = "Test message";
		if (write(client_sockfd, msg, strlen(msg)) < 0)
		{
			perror("Error writing to socket");
			close(client_sockfd);
			retry_count++;
			continue;
		} 

		#ifdef DEBUG
			printf("[DEBUG] Sent test message to %s:%d, waiting for response...\n", host, port);
		#endif
		//else {
			//char ack[BUFF_MAX];
			//int n = read(client_sockfd, ack, BUFF_MAX - 1);
	
		printf("%s[Peer %d]%s Connected\n",
			ANSI_GREEN,
			user_id,
			ANSI_RESET);
		user_info_t peer, this_host;
		strcpy(peer.hostname, host);
		peer.port = port;
		peer.connected = true;
		
		gethostname(this_host.hostname, NAME_MAX);
		this_host.port = this_port;
		this_host.connected = true;
// PROBLEM HERE
		// Connected? Share routing table
		gossip(this_user, peer);
		#ifdef DEBUG
			printf("[DEBUG] Shared routing table from %s to %s:%d\n", this_host.hostname, host, port);
		#endif

		// Receive routing table
		char resp[BUFF_MAX];
		int n = read(client_sockfd, resp, BUFF_MAX - 1);
// PROBLEM END (below does not run)
		#ifdef DEBUG
			printf("[DEBUG] Received %d bytes from %s:%d\n", n, host, port);
		#endif

		if (n > 0)
		{
			resp[n] = '\0';
			
			#ifdef DEBUG
				printf("[DEBUG] Received: %s\n", resp);
			#endif

			printf("%s[Peer %d]%s Connected\n",
				ANSI_GREEN,
				user_id,
				ANSI_RESET);

			// Process received routing table
			if (strncmp(resp, "ROUTES:", 7) == 0)
			{
				nav_table_t received_table;
				deserialize_table(&received_table, resp);
		
				if (received_table.routes[0].host.port != 0)
				{
					update_uni_table(&received_table);
					printf("[Gossip] Received routing table from %s:%d\n", host, port);
					run_da(&routing_table, &this_nav_table);
				} else {
					printf("[DEBUG] Received table was empty\n");
				}
			} else {
				printf("[DEBUG] Response was not a routing table\n");
			}
		} else if (n == 0) {
			printf("[DEBUG] Connection closed by peer\n");
		} else {
			printf("[DEBUG] Read error: %d\n", n);
		}
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
		error("Usage: [this_port] [user_hostname] [user_port]");

	this_user->user.port = atoi(argv[1]);	

	memset(&this_user->routing_table.nav_graph, 0, sizeof(nav_table_t));

	pthread_t server_t;
	pthread_create(&server_t, NULL, server_thread, &this_user->user.port);

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
