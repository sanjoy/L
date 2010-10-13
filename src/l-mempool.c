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

static void recursive_mark_lambda (LMempool *, LLambda *);

static void
recursive_mark_tree (LMempool *pool, LTreeNode *node)
{
	if (node == NULL)
		return;

	assert (find_and_mark (pool->blocks, node, sizeof (LTreeNode)));

	recursive_mark_tree (pool, node->left);
	recursive_mark_tree (pool, node->right);
	recursive_mark_tree (pool, node->lazy);
	recursive_mark_lambda (pool, node->lambda);

	if (node->token != NULL)
		assert (find_and_mark (pool->blocks, node->token, sizeof (LToken)));
}

static void
recursive_mark_assignment (LMempool *pool, LAssignment *assignment)
{
	assert (find_and_mark (pool->blocks, assignment, sizeof (LAssignment)));
	assert (find_and_mark (pool->blocks, assignment->lhs, sizeof (LToken)));
	recursive_mark_tree (pool, assignment->rhs);
}

static void
mark_list (LMempool *pool, LListNode *iter)
{
	for (; iter; iter = iter->next) {
		assert (find_and_mark (pool->blocks, iter, sizeof (LListNode)));
		assert (find_and_mark (pool->blocks, iter->token, sizeof (LToken)));
	}
}

static void
recursive_mark_lambda (LMempool *pool, LLambda *lambda)
{
	if (lambda == NULL)
		return;
	assert (find_and_mark (pool->blocks, lambda, sizeof (LLambda)));
	mark_list (pool, lambda->args);
	recursive_mark_tree (pool, lambda->body);
}

static void
calculate_stats (LMempool *pool, int *used, int *total)
{
	MempoolNode *iter;
	int used_count = 0, total_count = 0;
	for (iter = pool->blocks; iter; iter = iter->next) {
		int i;
		total_count += GET_MPOOL_NODE_SIZE (iter) / SIZEOF_PTR;
		for (i = 0; i < GET_MPOOL_NODE_SIZE (iter) / SIZEOF_PTR; i++) {
			if (GET_NTH_BIT (iter->begin, i))
				used_count++;
		}
	}

	*used = used_count;
	*total = total_count;
}

void
l_mempool_gc (LMempool *pool, void *context)
{
	LContext *ctx = context;
	MempoolNode *iter;
	LAssignment *a_root_iter;
	LLambda *l_root_iter;

	int total, used;

	assert (pool->gc);

	/* First clear the tag blocks. */
	for (iter = pool->blocks; iter; iter = iter->next)
		memset (iter->begin, 0, ((size_t) (iter->real_end - iter->begin)) >> SIZEOF_PTR);

	for (a_root_iter = ctx->global_assignments; a_root_iter; a_root_iter = a_root_iter->next)
		recursive_mark_assignment (pool, a_root_iter);
	
	for (l_root_iter = ctx->global_lambdas; l_root_iter; l_root_iter = l_root_iter->next)
		recursive_mark_lambda (pool, l_root_iter);

	calculate_stats (pool, &used, &total);

	printf ("%d / %d\n", used, total);
}
