#include "l-mempool.h"
#include "l-context.h"
#include "l-structures.h"

#include <sys/mman.h>
#include <string.h>
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

#define MARK_NTH_BIT(block, __n) do { \
		char *__tmp = (char *) ((block)->begin); \
		int n = (__n); \
		assert ((n / 8 + 1) <= (block)->offset); \
		__tmp [n / 8] |= 1 << (n % 8); \
	} while (0)

#define UNMARK_NTH_BIT(block, __n) do { \
		char *__tmp = (char *) ((block)->begin); \
		int n = (__n); \
		assert ((n / 8 + 1) <= (block)->offset); \
		__tmp [n / 8] &= ~(1 << (n % 8)); \
	} while (0)

#define GET_NTH_BIT(bits, n) \
	(((((char *) (bits)) [n / 8]) & (1 << (n % 8))) != 0)

struct _MempoolNode {
	void *begin, *end, *real_end;
	int offset; /* For debugging. */
	struct _MempoolNode *next;
};

#define GET_MPOOL_NODE_SIZE(node) \
	((size_t) ((node)->real_end - (node)->begin))

typedef struct _MempoolNode MempoolNode;

struct _LMempool {
	size_t page_size;
	int dev_zero_fd, gc;
	size_t total_bytes;
	MempoolNode *blocks;
};

#define MMAP_ALLOC(sz, fd) (mmap (NULL, sz, PROT_READ | PROT_WRITE, MAP_PRIVATE, fd, 0))

#define SIZEOF_PTR (sizeof (void *))

#define ALLOC_NEW_NODE(pool, size) do { \
		size_t sz = (size), offset; \
		MempoolNode *new_node = NULL; \
		if ((sz % ((pool)->page_size)) != 0) \
			sz = (sz / ((pool)->page_size) + 1) * ((pool)->page_size); \
		if ((new_node = MMAP_ALLOC (sz, pool->dev_zero_fd)) == (void *) -1) \
			perror ("ALLOC_NEW_NODE"); \
		new_node->begin = ((char *) new_node) + sizeof (MempoolNode); \
		offset = (sz / (SIZEOF_PTR * 8) + 1); \
		if (offset % SIZEOF_PTR != 0) \
			offset = (offset / SIZEOF_PTR + 1) * SIZEOF_PTR; \
		new_node->end = ((char *) (new_node->begin)) + offset; \
		new_node->real_end = ((char *) new_node) + sz; \
		new_node->next = (pool)->blocks; \
		new_node->offset = offset; \
		assert (new_node->offset >= (((new_node->real_end - new_node->begin) / SIZEOF_PTR) / 8 + 1)); \
		(pool)->blocks = new_node; \
		(pool)->total_bytes += sz; \
	} while (0)

LMempool *
l_mempool_new (int gc)
{
	LMempool *pool = malloc (sizeof (LMempool)); /* Irony */
	pool->blocks = NULL;
	pool->gc = gc;
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

	if (pool->gc) {
		if (size % SIZEOF_PTR != 0)
			size = (size / SIZEOF_PTR + 1) * SIZEOF_PTR;
	}

	if ((pool->blocks->real_end - pool->blocks->end) < size) {
		//		printf ("Need %d bytes (from %d), allocating new block\n", size, (pool->blocks->real_end - pool->blocks->end));
		ALLOC_NEW_NODE (pool, size);
	}

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
		munmap (i, i->real_end - (void *) i);
		i = j;
	}
	close (pool->dev_zero_fd);
	free (pool);
}

static int
find_and_mark (MempoolNode *blocks, void *to_find, int sz)
{
	MempoolNode *iter;
	int i;
	for (iter = blocks; iter; iter = iter->next) {
		if (to_find >= iter->begin && to_find < iter->end) {
			assert ((((size_t) (to_find - iter->begin)) % SIZEOF_PTR) == 0);
			for (i = 0; i < sz / SIZEOF_PTR; i++) {
				MARK_NTH_BIT (iter, ((size_t) (to_find - iter->begin)) / SIZEOF_PTR + i);
			}
			return 1;
		}
	}
	return 0;
}

#define CONVERT_TOKEN(x) mark_ ## x
#define SCAN_ACTION(pool, node, type) \
	assert (find_and_mark ((pool)->blocks, (node), sizeof (type)))

#include "l-mempool-scan.inc"

#undef CONVERT_TOKEN
#undef SCAN_ACTION

#if 0

/* Maybe for a moving GC later. */

static void *
find_space (LMempool *pool, size_t sz)
{
	MempoolNode *iter;
	int words = sz / SIZEOF_PTR, i, j;
	for (iter = pool->blocks; iter; iter = iter->next) {
		for (i = 0; i < GET_MPOOL_NODE_SIZE (iter) / SIZEOF_PTR - words; i++) {
			for (j = 0; j < words; j++)
				if (GET_NTH_BIT (iter->begin, i + j))
					break;
			if (j == words)
				return ((char *) iter) + SIZEOF_PTR * i;
		}
	}
	return NULL;
}

static void
erase_bits (LMempool *pool, void *ptr, size_t sz)
{
	MempoolNode *iter;
	int i;
	for (iter = pool->blocks; iter; iter = iter->next) {
		if (ptr >= iter->begin && ptr < iter->end) {
			assert ((((size_t) (ptr - iter->begin)) % SIZEOF_PTR) == 0);
			for (i = 0; i < sz / SIZEOF_PTR; i++)
				UNMARK_NTH_BIT (iter, ((size_t) (ptr - iter->begin)) / SIZEOF_PTR + i);
			return;
		}
	}
	assert (0);
}

#endif

//#define STATS 1

static void
delete_empty_blocks (LMempool *pool)
{
	MempoolNode *iter, *parent, *iter_next;
#ifdef STATS
	size_t prev_total = pool->total_bytes;
	size_t freed = 0;
#endif

	parent = NULL;
	for (iter = pool->blocks; iter; iter = iter_next) {
		int i, used = 0;
		for (i = 0; i < GET_MPOOL_NODE_SIZE (iter) / SIZEOF_PTR; i++) {
			if (GET_NTH_BIT (iter->begin, i)) {
				used = 1;
				break;
			}
		}
		iter_next = iter->next;
		if (!used) {
			if (parent == NULL)
				pool->blocks = iter->next;
			else
				parent->next = iter->next;
			pool->total_bytes -= (size_t) ((char *) (iter->real_end) - (char *) iter);
#ifdef STATS
			freed += (size_t) ((char *)iter->real_end - (char *) iter);
#endif
			munmap ((void *) iter, (char *) (iter->real_end) - (char *) iter);
		} else 
			parent = iter;
	}

#ifdef STATS
	printf ("Freed %d bytes. Previous total %d bytes. Current total %d bytes.\n",
	        (int) freed, (int) prev_total, (int) pool->total_bytes);
#endif
}

void
l_mempool_gc (LMempool *pool, void *context)
{
	LContext *ctx = context;
	MempoolNode *iter;
	LAssignment *a_root_iter;
	LLambda *l_root_iter;

	assert (pool->gc);

	/* First clear the tag blocks. */
	for (iter = pool->blocks; iter; iter = iter->next)
		memset (iter->begin, 0, ((size_t) (iter->real_end - iter->begin)) >> SIZEOF_PTR);

	for (a_root_iter = ctx->global_assignments; a_root_iter; a_root_iter = a_root_iter->next)
		mark_recursive_assignment (pool, a_root_iter);
	
	for (l_root_iter = ctx->global_lambdas; l_root_iter; l_root_iter = l_root_iter->next)
		mark_recursive_lambda (pool, l_root_iter);

	delete_empty_blocks (pool);
}
