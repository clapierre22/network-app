#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#define NUM_NODES 20
#define BASE_PORT 8001
#define DIRECT_NODES 3

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
	FILE *fp;
	char* peers[DIRECT_NODES * 10];

	fp = fopen("compose.yaml", "w");

	fprintf(fp, "services:\n");
	
	for (int i = 0; i < NUM_NODES; i++)
	{
		int port;
		port = BASE_PORT + i;
		fprintf(fp, "  node%d:\n", i);
		fprintf(fp, "    build: .\n");
		fprintf(fp, "    container_name: node%d\n", i);
		fprintf(fp, "    hostname: node%d\n", i);
		fprintf(fp, "    networks:\n");
		fprintf(fp, "      - test-net\n");	
		
		if (i > 1)
		{
			int* avail, select;
			int conn_count, avail_count;

			avail_count = i - 1;
			conn_count = (DIRECT_NODES < avail_count) ? DIRECT_NODES : avail_count;
			avail = malloc(avail_count * sizeof(int));

			for (int j = 0; j < avail_count; j++)
			{
				avail[j] = j + 1;
			}

			shuffle(avail, avail_count);

			fprintf(fp, "    depends_on:\n");
			for (int x = 0; x < conn_count; x++)
			{
				fprintf(fp, "      - node%d\n", avail[x]);
			}

			fprintf(fp, "    command: [\"sh\", \"-c\", \"sleep 3 && ./user %d", port);
			for (int y = 0; y < conn_count; y++)
			{
				int peer_idx = avail[y];
				int peer_port = BASE_PORT + peer_idx;
				fprintf(fp, " node%d %d", peer_idx, peer_port);
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

	fclose(fp);

	return 0;
}
