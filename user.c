#include "user.h"

/*
 * Global Variables
 */
user_info_t direct_users[MAX_DIRECT_USERS];
int direct_count = 0;
int this_port = 0;
int this_sockfd;
pthread_mutex_t user_mutex = PTHREAD_MUTEX_INITIALIZER;
nav_table_t this_nav_table;
uni_table_t routing_table;
int rt_count = 0;
pthread_mutex_t table_mutex = PTHREAD_MUTEX_INITIALIZER;

void error(const char* msg)
{
	fprintf(stderr, "%s[Error]%s %s\n", ANSI_RED, ANSI_RESET, msg);
	exit(1);
}

void status(const char* msg)
{
	// TODO
}

nav_route_t create_route(
	user_info_t host, user_info_t dest, user_info_t next, int step)
{
	nav_route_t route;

	route.host = host;
	route.dest = dest;
	route.next = next;
	route.step = step;

	return route;
}

void update_nav_table(nav_table_t* nav_table, nav_route_t new_route, int i)
{
	if (i < 0 || i > MAX_DIRECT_USERS) error("Invalid index");
	nav_table->routes[i] = new_route;
}

void update_uni_table(nav_table_t* new_table)
{
	pthread_mutex_lock(&table_mutex);

	if (rt_count < MAX_TOTAL_USERS)
	{
		routing_table.tables[rt_count] = *new_table;
		rt_count++;

		printf("%s[Routing]%s Updated universal table (now %d tables)\n",
                       ANSI_GREEN, ANSI_RESET, rt_count);
	}

	pthread_mutex_unlock(&table_mutex);
}

int find_node_index(user_info_t* nodes, int node_count, int port)
{
	for (int i = 0; i < node_count; i++)
	{
		if(nodes[i].port == port) return i;
	}

	return(-1);
}

int find_node(user_info_t* nodes, int count, int port)
{
	for (int i = 0; i < count; i++) if (nodes[i].port == port) return i;

	return(-1);
}

int build_graph(uni_table_t* uni_table, user_info_t* nodes,
		int graph[MAX_TOTAL_USERS][MAX_TOTAL_USERS])
{
	int node_count = 0;

	// Init graph matrix to 0
	memset(graph, 0, sizeof(int) * MAX_TOTAL_USERS * MAX_TOTAL_USERS);

	// Build adjanceny matix from routing table
	for (int t = 0; t < rt_count; t++)
	{
		for (int r = 0; r < MAX_DIRECT_USERS; r++)
		{
			nav_route_t route = uni_table->tables[t].routes[r];

			// Skip routes that are unitialized
			if (route.dest.port == 0) continue;

			// Add host node if not initalized
			int host_idx = find_node_index(
					nodes, node_count, route.host.port);
			if (host_idx < 0 && node_count < MAX_TOTAL_USERS)
			{
				nodes[node_count] = route.host;
				host_idx = node_count++;
			}

			// Add dest node
			int dest_idx = find_node_index(
					nodes, node_count, route.dest.port);
			if (dest_idx < 0 && node_count < MAX_TOTAL_USERS)
			{
				nodes[node_count] = route.dest;
				dest_idx = node_count++;
			}
			
			// Add edge to adjacency matrix
			if (host_idx >= 0 && dest_idx >= 0)
			{
				graph[host_idx][dest_idx] = 1;
			}
		}
	}

	return node_count;
}

int min_distance(int dist[], int spt_set[], int node_count)
{
	int min = INT_MAX;
	int min_idx = -1;

	// Find minimum distance vertex not yet in tree
	for (int i = 0; i < node_count; i++)
	{
		if (spt_set[i] == 0 && dist[i] <= min)
		{
			min = dist[i];
			min_idx = i;
		}
	}

	return min_idx;
}

user_info_t find_next(
		int src[], user_info_t* nodes, int src_idx, int dest_idx)
{
	// Find first jump from src
	int curr = dest_idx;
	int prev = src[curr];

	// Walk until node parent is src
	while (prev != src_idx && prev != -1)
	{
		curr = prev;
		prev = src[curr];
	}

	return nodes[curr];
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

	printf("Dijkstra computed %d routes\n", route_idx);

	pthread_mutex_unlock(&table_mutex);
}

void gossip(user_info_t host, user_info_t user)
{
	pthread_mutex_lock(&user_mutex);

	bool connected = false;
	for (int i = 0; i < direct_count; i++)
	{
		if (direct_users[i].port == user.port)
		{
			connected = true;
			break;
		}
	}

	if (!connected && direct_count < MAX_DIRECT_USERS)
	{
		direct_users[direct_count] = user;
		direct_users[direct_count].connected = true;

		nav_route_t direct_route = create_route(
						host, user, user, 1);
		update_nav_table(
			&this_nav_table, direct_route, direct_count);
		direct_count++;

		printf("Gossip added direct connection to port %d\n",
				user.port);

		update_uni_table(&this_nav_table);
		run_da(&routing_table, &this_nav_table);
	}

	pthread_mutex_unlock(&user_mutex);
}

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

	gossip(host, peer);

	char* msg = "Message ACK";
	n = write(user_sockfd, msg, strlen(msg));
	if (n < 0) error("Error writing to socket");
	
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
		pthread_detach(t);
	}

	return NULL;
}

void* client_thread(void* arg)
{
	char* host = ((char**)arg)[0];
	int port = atoi(((char**)arg)[1]);
	int count = 0;
	int user_id = port;

	while(RUNNING)
	{
		int client_sockfd = socket(AF_INET, SOCK_STREAM, 0);
		if (client_sockfd < 0) 
		{
			perror("Error creating client socket");
			break;
		}

		struct sockaddr_in addr;
		struct hostent* h = gethostbyname(host);

		if (h == NULL)
		{
			perror("Error retrieving host by name");
			close(client_sockfd);
			sleep(1);
			continue;
		}

		memset(&addr, 0, sizeof(addr));
		addr.sin_family = AF_INET;
		memcpy(&addr.sin_addr, h->h_addr, h->h_length);
		addr.sin_port = htons(port);

		if (connect(
			client_sockfd, 
			(struct sockaddr*)&addr, sizeof(addr)) < 0)
		{
			perror("Error connectig to socket");
			close(client_sockfd);
			sleep(1);
			continue;
		}

		char* msg = "Test message";
		if (write(client_sockfd, msg, strlen(msg)) < 0)
		{
			perror("Error writing to socket");
		} else {
			char ack[BUFF_MAX];
			int n = read(client_sockfd, ack, BUFF_MAX - 1);
			if (n > 0)
			{
				ack[n] = '\0';
				printf("%s[Peer %d]%s %s\n",
					ANSI_GREEN,
					user_id,
					ANSI_RESET,
					msg);

				user_info_t peer, this_host;
				strcpy(peer.hostname, host);
				peer.port = port;
				peer.connected = true;

				gethostname(this_host.hostname, NAME_MAX);
				this_host.port = this_port;
				this_host.connected = true;

				gossip(this_host, peer);
			}
		}

		close(client_sockfd);
		sleep(1);
	}

	return NULL;
}

int main(int argc, char* argv[])
{
	printf("Good build\n");

	if (argc < 2) 
		error("Usage: [this_port] [user_hostname] [user_port]");

	int port = atoi(argv[1]);
	this_port = port;

	memset(&this_nav_table, 0, sizeof(nav_table_t));
	memset(&routing_table, 0, sizeof(uni_table_t));

	pthread_t server_t;
	pthread_create(&server_t, NULL, server_thread, &port);

	if (argc == 4)
	{
		sleep(1);
		char* user_args[] = {argv[2], argv[3]};
		pthread_t client_t;
		pthread_create(&client_t, NULL, client_thread, user_args);
		pthread_detach(client_t);
	}

	pthread_join(server_t, NULL);

	return 0;
}
