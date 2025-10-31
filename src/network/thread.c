#include "thread.h"

void* server_thread(void* arg)
{
	return NULL;
}

void* client_thread(void* arg)
{
	char** args = (char**)arg;
	user_data_t* host, user;

	free(args);

	gossip(host, user);

	return NULL;
}
