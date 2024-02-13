#ifndef ABSTRACT_SYNTAX_TREE_H
#define ABSTRACT_SYNTAX_TREE_H

#include <stdlib.h>

typedef enum {NONE, ONE, TWO} tag;

typedef struct ast ast;

struct ast {
	void *data;
	tag num_children;
	union {
		ast *child;
		struct {
			ast *left;
			ast *right;
		} children;
	};
};

ast *ast_create();
void ast_destroy(ast *root);

#endif