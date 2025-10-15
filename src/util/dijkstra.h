#ifndef DIJKSTRA_H
#define DIJKSTRA_H

/*
 * Dijkstra.c/.h holds the dijkstra algorithim for the routing tables 
 * (nav_graph)
 */

void dijkstra(nav_table_t* nav_table);
void display_graph(int** graph);

#endif // DIJKSTRA_H
