#ifndef TABLE_H
#define TABLE_H

/*
 * Table.c/.h hold all logic for serailization and deserialization for
 * routing tables (nav_graph)
 */

void serialize_table(nav_table_t* table, char* buff);
void deserialize_table(nav_table_t* table, char* buff);

#endif // TABLE_H
