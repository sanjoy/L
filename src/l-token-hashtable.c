#include "l-token-hashtable.h"

#include <string.h>

struct _HashElement {
	int token_id;
	char *token_name;
	struct _HashElement *next;
};

typedef struct _HashElement HashElement;

struct _LTokenHashtable {
	HashElement **table;
	LMempool *other_pool, *token_pool;
	int hash_len;
	int token_id_max;
};

LTokenHashtable *
l_token_hashtable_new (LMempool *other_pool, LMempool *token_pool, int hash_len)
{
	LTokenHashtable *table = l_mempool_alloc (other_pool, sizeof (LTokenHashtable));
	table->hash_len = hash_len;
	table->table = l_mempool_alloc (other_pool, sizeof (HashElement *) * hash_len);
	table->other_pool = other_pool;
	table->token_pool = token_pool;
	return table;
}

static int
string_to_slot (LTokenHashtable *table, char *text)
{
	int slot = 0;
	while (*text)
		slot += *text++;
	slot %= table->hash_len;
	return slot;
}

static HashElement *
hashtable_lookup (LTokenHashtable *table, char *text)
{
	int slot = string_to_slot (table, text);
	HashElement *iter;

	for (iter = table->table [slot]; iter; iter = iter->next) {
		if (strcmp (iter->token_name, text) == 0)
			return iter;
	}
	return NULL;
}

LToken *
l_token_hashtable_hash (LTokenHashtable *table, char *text)
{
	HashElement *bucket = hashtable_lookup (table, text);
	LToken *token = l_mempool_alloc (table->token_pool, sizeof (LToken));

	if (bucket != NULL) {
		token->name = bucket->token_name;
		token->idx = bucket->token_id;
		return token;
	} else {
		int slot = string_to_slot (table, text);
		token->name = l_mempool_alloc (table->other_pool, strlen (text) + 1);
		strcpy (token->name, text);
		token->idx = table->token_id_max++;
		
		bucket = l_mempool_alloc (table->other_pool, sizeof (HashElement));
		bucket->token_name = token->name;
		bucket->token_id = token->idx;
		bucket->next = table->table [slot];
		table->table [slot] = bucket;
		return token;
	}
}
