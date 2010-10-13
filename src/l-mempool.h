#ifndef __L__MEMPOOL__H
#define __L__MEMPOOL__H

#include <stddef.h>

typedef struct _LMempool LMempool;

LMempool *l_mempool_new (void);
void *l_mempool_alloc (LMempool *, size_t);
void l_mempool_pre_alloc (LMempool *, size_t);
void l_mempool_destroy (LMempool *);

/* The second argument a pointer to the LContext
 * concerned. */
void l_mempool_gc (LMempool *, void *);

#endif /* __L__MEMPOOL__H */
