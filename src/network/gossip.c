#include "gossip.h"

void gossip(user_data_t* host, user_data_t* user)
{
	// TODO: takes the host routing table and connecting user
	// routing table and does a bitwise operation to produce
	// a graph with all connections from both

	// Note: this is O(nsquared), improve in future
	// CHANGE: serialize/deserialize by making this into
	// array/single line of 1s, 0s which then have the bitwise OR
	// performed on them
	for (int i = 0; i < MAX_TOTAL_USERS; i++)
	{
		for (int j = 0; j < MAX_TOTAL_USERS; j++)
		{
			host->routing_table.nav_graph[i][j] | user->routing_table.nav_graph[i][j];
			user->routing_table.nav_graph[i][j] | host->routing_table.nav_graph[i][j];
		}
	}
}
