#include "hash_set.h"


hash_set_iterator hash_set_iterate(hash_set *set)
{
	return hash_map_iterate(set);
}

const void *hash_set_iterator_key(hash_set_iterator *iterator)
{
	return hash_map_iterator_key(iterator);
}

int hash_set_iterator_next(hash_set_iterator *iterator)
{
	return hash_map_iterator_next(iterator);
}

void hash_set_create(
	hash_set *set,
	size_t key_size,
	hash_function_t hash,
	void *hash_user_data)
{
	hash_map_create(set, key_size, 0, hash, hash_user_data);
}

void hash_set_destroy(hash_set *set)
{
	hash_map_destroy(set);
}

int hash_set_resize(hash_set *set, size_t bucket_count)
{
	return hash_map_resize(set, bucket_count);
}

int hash_set_grow(hash_set *set)
{
	return hash_map_grow(set);
}

int hash_set_insert(hash_set *set, const void *key)
{
	return hash_map_insert(set, key, 0);
}

int hash_set_contains(const hash_set *set, const void *key)
{
	return hash_map_find(set, key) != 0;
}

int hash_set_erase(hash_set *set, const void *key)
{
	return hash_map_erase(set, key);
}

size_t hash_set_size(hash_set *set)
{
	return hash_map_size(set);
}
