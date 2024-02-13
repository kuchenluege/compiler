#include "compiler/abstract_syntax_tree.h"

ast *ast_create() {
	ast *root = (ast*)malloc(sizeof(ast));
	return root;
}

void ast_destroy(ast *root) {
	switch (root->num_children)
	{
	case ONE:
		ast_destroy(root->child);
		break;
	case TWO:
		ast_destroy(root->children.left);
		ast_destroy(root->children.right);
		break;
	}
	free(root->data);
	free(root);
}