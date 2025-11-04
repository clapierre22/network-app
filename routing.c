#include "routing.h"

unsigned long hash(const char* str, int size)
{
}// DJB2

hash_table_t* init_hash_table(int size)
{
}

user_t* init_user(const char* hostname, const char* ip_addr, int port)
{
}

bool insert_user(hash_table_t* table, user_t* user)
{
}

user_t* find_user(hash_table_t* table, const char* hostname)
{
}

bool add_route(user_t from, user_t* to)
{
}
