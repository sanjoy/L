#include "l-mempool.h"

#include <sys/mman.h>
#include <unistd.h>
#include <limits.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>

#if CHAR_BIT != 8
#error Architecture not supported.
#endif

struct _MempoolNode {
	void *begin, *end, *real_end;
	struct _MempoolNode *next;
};

typedef struct _MempoolNode MempoolNode;

struct _LMempool {
	size_t page_size;
	int dev_zero_fd;
	MempoolNode *blocks;
};

#define MMAP_ALLOC(sz, fd) (mmap (NULL, sz, PROT_READ | PROT_WRITE, MAP_PRIVATE, fd, 0))

#define ALLOC_NEW_NODE(pool, size) do { \
		size_t sz = (size); \
		MempoolNode *new_node = NULL; \
		if ((sz % ((pool)->page_size)) != 0) \
			sz = (sz / ((pool)->page_size) + 1) * ((pool)->page_size); \
		if ((new_node = MMAP_ALLOC (sz, pool->dev_zero_fd)) == (void *) -1) \
			perror ("ALLOC_NEW_NODE"); \
		new_node->end = new_node->begin = ((char *) new_node) + sizeof (MempoolNode); \
		new_node->real_end = ((char *) new_node) + sz; \
		new_node->next = (pool)->blocks; \
		(pool)->blocks = new_node; \
	} while (0)

LMempool *
l_mempool_new (void)
{
	LMempool *pool = malloc (sizeof (LMempool)); /* Irony */
	pool->blocks = NULL;
	pool->dev_zero_fd = open ("/dev/zero", O_RDWR);
	pool->page_size = getpagesize ();
	ALLOC_NEW_NODE (pool, pool->page_size);
	return pool;
}

void *
l_mempool_alloc (LMempool *pool, size_t size)
{
	void *ret;
	if (size == 0)
		return NULL;
	if ((pool->blocks->real_end - pool->blocks->end) < size)
		ALLOC_NEW_NODE (pool, size);
	ret = pool->blocks->end;
	pool->blocks->end += size;
	return ret;
}

void
l_mempool_pre_alloc (LMempool *pool, size_t size)
{
	if ((pool->blocks->real_end - pool->blocks->end) < size)
		ALLOC_NEW_NODE (pool, size + sizeof (MempoolNode));
}

void
l_mempool_destroy (LMempool *pool)
{
	MempoolNode *i, *j;
	i = pool->blocks;

	while (i != NULL) {
		j = i->next;
		munmap (i, i->real_end - i->begin);
		i = j;
	}
	close (pool->dev_zero_fd);
	free (pool);
}
