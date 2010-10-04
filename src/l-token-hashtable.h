#ifndef __TOKEN__HASH__H
#define __TOKEN__HASH__H

#include "l-structures.h"

typedef struct _LTokenHashtable LTokenHashtable;

LTokenHashtable *l_token_hashtable_new (LMempool *, int);

LToken *l_token_hashtable_hash (LTokenHashtable *, char *);
LToken *l_token_hashtable_lookup (LTokenHashtable *, char *);

#endif /* __TOKEN__HASH__H*/
