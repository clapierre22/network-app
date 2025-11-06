#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>

#define NUM_NODES 6
#define BASE_PORT 8008
#define DIRECT_NODES 3

//#define SUBNET "10.0.0.0/24"
#define BASE_HOST 5

void shuffle(int* arr, int n)
{
	srand(time(NULL));

	for (int i = n - 1; i > 0; i--)
	{
		int j = rand() % (i + 1);
		int temp = arr[i];
		arr[i] = arr[j];
		arr[j] = temp;
	}
}

int main(int argc, char* argv[])
{
	fprintf(stdout, "Usage: ./gen_conn [IP Subnet]\n");

	char* subnet = "10.0.0.0/24";

	if (argc > 1)
	{
		subnet = argv[1];
		fprintf(stdout, "Setting IP Subnet to %s\n", subnet);
	}
	else
	{
		fprintf(stdout, "Using default IP Subnet (10.0.0.0/24)\n");
	}

	FILE *fp;
	char* peers[DIRECT_NODES * 10];

	//char* subnet = (argc > 1) ? argv[1] : "10.0.0.0/24";
	char base_ip[16];

	sscanf(subnet, "%[^/]", base_ip);
    	char* c = strrchr(base_ip, '.');
    	if (c) *c = '\0';

	fp = fopen("compose.yaml", "w");

	fprintf(fp, "services:\n");
	
	for (int i = 0; i < NUM_NODES; i++)
	{
		int port = BASE_PORT; // CHANGED: All use same port as each container will have unique IP ADDR
		//port = BASE_PORT + i;

		int this_octet = BASE_HOST + i;

		fprintf(fp, "  node%d:\n", i);
		fprintf(fp, "    build: .\n");
		fprintf(fp, "    container_name: node%d\n", i);
		fprintf(fp, "    hostname: node%d\n", i);
		fprintf(fp, "    networks:\n");
		fprintf(fp, "      test-net:\n");
		fprintf(fp, "        ipv4_address: %s.%d\n", base_ip, this_octet);
		
		if (i > 0)
		{
			int* avail, select;
			int conn_count, avail_count;

			avail_count = i;
			conn_count = (DIRECT_NODES < avail_count) ? DIRECT_NODES : avail_count;
			avail = malloc(avail_count * sizeof(int));

			for (int j = 0; j < avail_count; j++)
			{
				avail[j] = j;
			}

			shuffle(avail, avail_count);

			fprintf(fp, "    depends_on:\n");
			for (int x = 0; x < conn_count; x++)
			{
				fprintf(fp, "      - node%d\n", avail[x]);
			}

			fprintf(fp, "    command: [\"sh\", \"-c\", \"sleep %d && ./user %d", 2 + (i / 4), port);
			for (int y = 0; y < conn_count; y++)
			{
				int peer_idx = avail[y];
				int peer_octet = BASE_HOST + peer_idx;
				int peer_port = BASE_PORT;
				fprintf(fp, " %s.%d %d", base_ip, peer_octet, peer_port);
			}
			fprintf(fp, "\"]\n");

			free(avail);
		} else {
			fprintf(fp, "    command: [\"./user\", \"%d\"]\n", port);
		}
		fprintf(fp, "\n");
	}

	fprintf(fp, "networks:\n");
	fprintf(fp, "  test-net:\n");
	fprintf(fp, "    driver: bridge\n");
	fprintf(fp, "    ipam:\n");
	fprintf(fp, "      config:\n");
	fprintf(fp, "        - subnet: %s\n", subnet);

	fclose(fp);

	return 0;
}
