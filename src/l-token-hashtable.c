#include "l-token-hashtable.h"

#include <string.h>

struct _LTokenWrapper {
	LToken *token;
	struct _LTokenWrapper *next;
};

typedef struct _LTokenWrapper LTokenWrapper;

struct _LTokenHashtable {
	LTokenWrapper **table;
	LMempool *mempool;
	int hash_len;
	int token_id_max;
};

LTokenHashtable *
l_token_hashtable_new (LMempool *pool, int hash_len)
{
	LTokenHashtable *table = l_mempool_alloc (pool, sizeof (LTokenHashtable));
	table->hash_len = hash_len;
	table->table = l_mempool_alloc (pool, sizeof (LTokenWrapper *) * hash_len);
	table->mempool = pool;
	return table;
}

LToken *
l_token_hashtable_lookup (LTokenHashtable *table, char *text)
{
	int slot = 0;
	LTokenWrapper *iter;
	char *text_iter = text;

	while (*text_iter)
		slot += *text_iter++;
	slot %= table->hash_len;

	for (iter = table->table [slot]; iter; iter = iter->next) {
		if (strcmp (iter->token->name, text) == 0)
			return iter->token;
	}
	return NULL;
}

LToken *
l_token_hashtable_hash (LTokenHashtable *table, char *text)
{
	LToken *prev = l_token_hashtable_lookup (table, text);

	if (prev == NULL) {
		LToken *token = l_mempool_alloc (table->mempool, sizeof (LToken));
		token->name = l_mempool_alloc (table->mempool, strlen (text) + 1);
		strcpy (token->name, text);
		token->idx = table->token_id_max++;

		{

			int slot = 0;
			LTokenWrapper *wrapper = l_mempool_alloc (table->mempool, sizeof (LTokenWrapper));
			wrapper->token = token;

			while (*text)
				slot += *text++;
			slot %= table->hash_len;

			wrapper->next = table->table [slot];
			table->table [slot] = wrapper;

		}

		return token;
	}

	return prev;
}
