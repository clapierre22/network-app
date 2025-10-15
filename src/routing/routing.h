#ifndef ROUTING_H
#define ROUTING_H

/*
 * Routing.c/.h holds all logic relating to nav tables and routes
 */

nav_route_t create_route(user_info_t host, user_info_t dest);
void update_nav_table(nav_table_t* new_table);

#endif // ROUTING_H
