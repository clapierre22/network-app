#include "user.h"

void error(const char* msg)
{
	fprintf(stderr, "%s[Error]%s %s\n", ANSI_RED, ANSI_RESET, msg);
	exit(1);
}

void status(const char* msg)
{
	// TODO
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
