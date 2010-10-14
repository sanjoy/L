static void CONVERT_TOKEN (recursive_lambda) (LMempool *, LLambda *);

static void
CONVERT_TOKEN (recursive_tree) (LMempool *pool, LTreeNode *node)
{
	if (node == NULL)
		return;

	SCAN_ACTION (pool, node, LTreeNode);

	CONVERT_TOKEN (recursive_tree) (pool, node->left);
	CONVERT_TOKEN (recursive_tree) (pool, node->right);
	CONVERT_TOKEN (recursive_tree) (pool, node->lazy);
	CONVERT_TOKEN (recursive_lambda) (pool, node->lambda);

	if (node->token != NULL)
		SCAN_ACTION (pool, node->token, LToken);
}

static void
CONVERT_TOKEN (recursive_assignment) (LMempool *pool, LAssignment *assignment)
{
	SCAN_ACTION (pool, assignment, LAssignment);
	SCAN_ACTION (pool, assignment->lhs, LToken);
	CONVERT_TOKEN (recursive_tree) (pool, assignment->rhs);
}

static void
CONVERT_TOKEN (mark_list) (LMempool *pool, LListNode *iter)
{
	for (; iter; iter = iter->next) {
		SCAN_ACTION (pool, iter, LListNode);
		SCAN_ACTION (pool, iter->token, LToken);
	}
}

static void
CONVERT_TOKEN (recursive_lambda) (LMempool *pool, LLambda *lambda)
{
	if (lambda == NULL)
		return;
	SCAN_ACTION (pool, lambda, LLambda);
	CONVERT_TOKEN (mark_list) (pool, lambda->args);
	CONVERT_TOKEN (recursive_tree) (pool, lambda->body);
}
