#include <stdio.h>
#include <assert.h>

#include "l-mempool.h"

#define ARRAY_SZ_A 10000
#define ARRAY_SZ_B 10

int
main ()
{
	LMempool *pool = l_mempool_new ();
	int **arr = l_mempool_alloc (pool, sizeof (int *) * ARRAY_SZ_A);
	int i, j;

	for (i = 0; i < ARRAY_SZ_A; i++) {
		arr [i] = l_mempool_alloc (pool, sizeof (int) * ARRAY_SZ_B);
		for (j = 0; j < ARRAY_SZ_B; j++) {
			assert (arr [i] [j] == 0);
			arr [i] [j] = i + j;
		}
	}

	for (i = 0; i < ARRAY_SZ_A; i++)
		for (j = 0; j < ARRAY_SZ_B; j++)
			assert (arr [i] [j] == (i + j));

	l_mempool_destroy (pool);

	printf ("Test successful.\n");
}
