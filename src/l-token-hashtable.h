#ifndef __TOKEN__HASH__H
#define __TOKEN__HASH__H

#include "l-structures.h"

/* Create a new LTokenHashtable from the memory pool
 * and the hash length (preferably a prime number).
 * Note that the LMempool is permamently stored in the LTokenHashtable
 * structure.
 */
LTokenHashtable *l_token_hashtable_new (LMempool *, LMempool *, int);

LToken *l_token_hashtable_hash (LTokenHashtable *, char *);

#endif /* __TOKEN__HASH__H*/
