#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include "compiler/hash_table.h"
#include "compiler/abstract_syntax_tree.h"
#include "compiler/token.h"
#include "compiler/error.h"

#define ASSERT(X) if (!(X)) return 0;

struct file_data {
	char* name;
	int line_num;
} file_data = {"", 1};

ht *symbol_table = NULL;
ast *parse_tree_root = NULL;
token *tok = NULL;
token *unscanned_tok = NULL;

void init_symbol_table() {
	symbol_table = ht_create();
	size_t rw_len = sizeof(RES_WORDS) / sizeof(char*);

	for (size_t i = 0; i < rw_len; i++) {
		token *rw_token = (token*)malloc(sizeof(token));
		rw_token->type = RW_TOKEN_TYPES[i];
		rw_token->subtype = RW_TOKEN_SUBTYPES[i];
		strcpy(rw_token->lex_val.str_val, RES_WORDS[i]);
		ht_set(symbol_table, RES_WORDS[i], rw_token);
	}
}

void destroy_symbol_table() {
	hti iter = ht_iterator(symbol_table);
	while (ht_next(&iter)) {
		free(iter.value);
	}
	ht_destroy(symbol_table);
}

int get_char(FILE *file) {
	int c = getc(file);
	if (c == '\n') {
		file_data.line_num++;
	}
	return c;
}

void unget_char(int c, FILE *file) {
	ungetc(c, file);
	if (c == '\n') {
		file_data.line_num--;
	}
}

void ignore_whitespace(int *c_addr, FILE *file) {
	do {
		*c_addr = get_char(file);
	} while (isspace(*c_addr));
}

void ignore_comments_whitespace(int *c_addr, FILE *file) {
	ignore_whitespace(c_addr, file);

	if (*c_addr == '/') {
		int comment_line_num = file_data.line_num;
		int next_c = get_char(file);

		// Block
		if (next_c == '*') {
			int comment_level = 1;
			while (comment_level > 0) {
				do {
					*c_addr = get_char(file);
					if (*c_addr == '/') {
						next_c = get_char(file);
						if (next_c == '*') {
							comment_level++;
						}
					}
				} while (*c_addr != '*' && *c_addr != EOF);

				if (*c_addr == EOF) {
					print_error(file_data.name, UNCLOSED_COMMENT, comment_line_num, "");
					break;
				}

				next_c = get_char(file);
				if (next_c == EOF) {
					*c_addr = next_c;
					print_error(file_data.name, UNCLOSED_COMMENT, comment_line_num, "");
					break;
				}
				if (next_c == '/') {
					comment_level--;
				}
			}

			if (*c_addr != EOF && next_c != EOF) {
				ignore_comments_whitespace(c_addr, file);
			}
		}

		// Single line
		else if (next_c == '/') {
			do {
				*c_addr = get_char(file);
			} while (*c_addr != '\n' && *c_addr != EOF);
			
			if (*c_addr != EOF) {
				ignore_comments_whitespace(c_addr, file);
			}
		}

		else {
			unget_char(next_c, file);
		}
	}
}

void unscan(token *t) {
	printf("(Unscanned token: %d)\n", t->type);
	unscanned_tok = t;
}

token *scan(FILE *file) {
	if (unscanned_tok) {
		printf("(Rescanned token: %d)\n", unscanned_tok->type);
		token *tmp = unscanned_tok;
		unscanned_tok = NULL;
		return tmp;
	}
	
	int c;
	ignore_comments_whitespace(&c, file);

	token *t = (token*)malloc(sizeof(token));
	t->type = T_UNKNOWN;
	t->subtype = T_ST_NONE;
	t->tag = T_TAG_NONE;

	switch (c) {
	case T_PERIOD:
	case T_SEMICOLON:
	case T_LPAREN:
	case T_RPAREN:
	case T_COMMA:
	case T_LBRACK:
	case T_RBRACK:
		t->type = c;
		break;
	case '&':
		t->type = T_EXPR_OP;
		t->subtype = T_ST_AND; 
		break;
	case '|':
		t->type = T_EXPR_OP;
		t->subtype = T_ST_OR;
		break;
	case '+':
		t->type = T_ARITH_OP;
		t->subtype = T_ST_PLUS; 
		break;
	case '-':
		t->type = T_ARITH_OP;
		t->subtype = T_ST_MINUS;
		break;
	case '*':
		t->type = T_TERM_OP;
		t->subtype = T_ST_MULT;
		break;
	case '/':
		t->type = T_TERM_OP;
		t->subtype = T_ST_DIVIDE;
		break;
	case '<':
		{
			t->type = T_REL_OP;
			char next_c = get_char(file);
			if (next_c == '=') {
				t->subtype = T_ST_LTEQL;
			}
			else {
				unget_char(next_c, file);
				t->subtype = T_ST_LTHAN;
			}
		}
		break;
	case '>':
		{
			t->type = T_REL_OP;
			char next_c = get_char(file);
			if (next_c == '=') {
				t->subtype = T_ST_GTEQL;
			}
			else {
				unget_char(next_c, file);
				t->subtype = T_ST_GTHAN;
			}
		}
		break;
	case '=':
		{
			char next_c = get_char(file);
			if (next_c == '=') {
				t->type = T_REL_OP;
				t->subtype = T_ST_EQLTO;
			}
			else {
				// illegal, ignore
				unget_char(next_c, file);
			}
		}
		break;
	case '!':
		{
			char next_c = get_char(file);
			if (next_c == '=') {
				t->type = T_REL_OP;
				t->subtype = T_ST_NOTEQ;
			}
			else {
				// illegal, ignore
				unget_char(next_c, file);
			}
		}
		break;
	case ':':
		{
			char next_c = get_char(file);
			if (next_c == '=') {
				t->type = T_ASSMT;
			}
			else {
				unget_char(next_c, file);
				t->type = T_COLON;
			}
		}
		break;
	case '\"':
		{
			t->type = T_STR_LIT;
			t->tag = T_TAG_STR;
			int str_line_num = file_data.line_num;
			size_t i = 0;
			c = get_char(file);
			while (c != '\"' && c != EOF) {
				t->lex_val.str_val[i] = c;
				c = get_char(file);
				i++;
			}
			t->lex_val.str_val[i] = '\0';

			if (c == EOF)
				print_error(file_data.name, UNCLOSED_STR, str_line_num, "");
		}
		break;
	case 'A'...'Z':
	case 'a'...'z':
		{
			t->type = T_IDENT;
			size_t i = 0;
			do {
				t->lex_val.str_val[i] = toupper(c);
				c = get_char(file);
				i++;
			} while (isalnum(c) || c == '_');
			unget_char(c, file);
			t->lex_val.str_val[i] = '\0';
			
			token *existing_token = ht_get(symbol_table, t->lex_val.str_val);
			if (existing_token == NULL) {
				ht_set(symbol_table, t->lex_val.str_val, t);
			} else {
				free(t);
				t = existing_token;
			}
		}
		break;
	case '0'...'9':
		{
			t->type = T_NUM_LIT;
			
			int num_line_num = file_data.line_num;
			int dec_pt_cnt = 0;
			size_t i = 0;
			do {
				t->lex_val.str_val[i] = toupper(c);
				c = get_char(file);
				i++;
				if (c == '.') dec_pt_cnt++;
			} while (isdigit(c) || c == '.');
			unget_char(c, file);
			t->lex_val.str_val[i] = '\0';

			if (dec_pt_cnt > 1) {
				print_error(file_data.name, EXTRA_DEC_PT, num_line_num, "");
				t->type = T_UNKNOWN;
			}
			else if (dec_pt_cnt == 1) {
				t->subtype = T_ST_FLOAT_LIT;
				t->tag = T_TAG_FLT;
				t->lex_val.flt_val = atof(t->lex_val.str_val);
			}
			else {
				t->subtype = T_ST_INT_LIT;
				t->tag = T_TAG_INT;
				t->lex_val.int_val = atoi(t->lex_val.str_val);
			}
		}
		break;
	case EOF:
		t->type = T_EOF;
		break;
	default:
		print_error(file_data.name, UNREC_TOKEN, file_data.line_num, (char[2]){(char)c, '\0'});
		break;
	}
	printf("Scanned token: %d\n", t->type);
	return t;
}

int expression(FILE *file);

int argument_list_prime(FILE *file) {
	ASSERT(expression(file))
	tok = scan(file);
	if (tok->type == T_COMMA) {
		tok = scan(file);
		ASSERT(argument_list_prime(file))
	}
	else unscan(tok);
	return 1;
}

int argument_list(FILE *file) {
	if (tok->type != T_RPAREN) {
		ASSERT(argument_list_prime(file))
	}
	else unscan(tok);
	return 1;
}

int location_tail(FILE *file) {
	ASSERT(expression(file))
	tok = scan(file);
	ASSERT(tok->type == T_RBRACK)
	return 1;
}

int location(FILE *file) {
	ASSERT(tok->type == T_IDENT)
	tok = scan(file);
	if (tok->type == T_LBRACK) {
		tok = scan(file);
		ASSERT(location_tail(file));
	}
	else unscan(tok);
	return 1;
}

int procedure_call_tail(FILE *file) {
	ASSERT(argument_list(file))
	tok = scan(file);
	ASSERT(tok->type == T_RPAREN)
	return 1;
}

int ident_tail(FILE *file) {
	if (tok->type == T_LBRACK) {
		tok = scan(file);
		ASSERT(location_tail(file))
	}
	else if (tok->type == T_LPAREN) {
		tok = scan(file);
		ASSERT(procedure_call_tail(file))
	} 
	else unscan(tok);
	return 1;
}

int factor(FILE *file) {
	if (tok->type == T_LPAREN) {
		tok = scan(file);
		ASSERT(expression(file))
		tok = scan(file);
		ASSERT(tok->type == T_RPAREN)
	}
	else if (tok->subtype == T_ST_MINUS) {
		tok = scan(file);
		if (tok->type == T_IDENT) {
			tok = scan(file);
			ASSERT(ident_tail(file))
		}
		else ASSERT(tok->type == T_NUM_LIT)
	}
	else if (tok->type == T_IDENT) {
		tok = scan(file);
		ASSERT(ident_tail(file));
	}
	else ASSERT(tok->type == T_NUM_LIT || tok->type == T_STR_LIT || tok->type == T_BOOL_LIT)
	return 1;
}

int term_prime(FILE *file) {
	if (tok->type == T_TERM_OP) {
		tok = scan(file);
		ASSERT(factor(file))
		tok = scan(file);
		ASSERT(term_prime(file))
	}
	else unscan(tok);
	return 1;
}

int term(FILE *file) {
	ASSERT(factor(file))
	tok = scan(file);
	ASSERT(term_prime(file))
	return 1;
}

int relation_prime(FILE *file) {
	if (tok->type == T_REL_OP) {
		tok = scan(file);
		ASSERT(term(file))
		tok = scan(file);
		ASSERT(relation_prime(file))
	}
	else unscan(tok);
	return 1;
}

int relation(FILE *file) {
	ASSERT(term(file))
	tok = scan(file);
	ASSERT(relation_prime(file))
	return 1;
}

int arith_op_prime(FILE *file) {
	if (tok->type == T_ARITH_OP) {
		tok = scan(file);
		ASSERT(relation(file))
		tok = scan(file);
		ASSERT(arith_op_prime(file))
	}
	else unscan(tok);
	return 1;
}

int arith_op(FILE *file) {
	ASSERT(relation(file))
	tok = scan(file);
	ASSERT(arith_op_prime(file))
	return 1;
}

int expression_prime(FILE *file) {
	if (tok->type == T_EXPR_OP) {
		tok = scan(file);
		ASSERT(arith_op(file))
		tok = scan(file);
		ASSERT(expression_prime(file))
	}
	else unscan(tok);
	return 1;
}

int expression(FILE *file) {
	if (tok->type == T_NOT) {
		tok = scan(file);
	}
	ASSERT(arith_op(file))
	tok = scan(file);
	ASSERT(expression_prime(file))
	return 1;
}

int assignment_statement(FILE *file) {
	if (tok->type == T_IDENT) {
		ASSERT(location(file))
		tok = scan(file);
		ASSERT(tok->type == T_ASSMT)
		tok = scan(file);
		ASSERT(expression(file));
		tok = scan(file);
		ASSERT(tok->type == T_SEMICOLON)
	}
	else return 0;
	return 1;
}

int statement(FILE *file);

int if_statement(FILE *file) {
	if (tok->type == T_IF) {
		tok = scan(file);
		ASSERT(tok->type == T_LPAREN)
		tok = scan(file);
		ASSERT(expression(file))
		tok = scan(file);
		ASSERT(tok->type == T_RPAREN)
		tok = scan(file);
		ASSERT(tok->type == T_THEN)
		tok = scan(file);
		while (tok->type != T_END && tok->type != T_ELSE) {
			ASSERT(statement(file))
			tok = scan(file);
			ASSERT(tok->type != T_EOF)
		}
		if (tok->type == T_ELSE) {
			tok = scan(file);
			while (tok->type != T_END) {
				ASSERT(statement(file))
				tok = scan(file);
				ASSERT(tok->type != T_EOF)
			}
		}
		tok = scan(file);
		ASSERT(tok->type == T_IF)
		tok = scan(file);
		ASSERT(tok->type == T_SEMICOLON)
	}
	else return 0;
	return 1;
}

int for_statement(FILE *file) {
	if (tok->type == T_FOR) {
		tok = scan(file);
		ASSERT(tok->type == T_LPAREN)
		tok = scan(file);
		ASSERT(assignment_statement(file))
		tok = scan(file);
		ASSERT(expression(file))
		tok = scan(file);
		ASSERT(tok->type == T_RPAREN)
		tok = scan(file);
		while (tok->type != T_END) {
			ASSERT(statement(file))
			tok = scan(file);
			ASSERT(tok->type != T_EOF)
		}
		tok = scan(file);
		ASSERT(tok->type == T_FOR)
		tok = scan(file);
		ASSERT(tok->type == T_SEMICOLON)
	}
	else return 0;
	return 1;
}

int return_statement(FILE *file) {
	if (tok->type == T_RETURN) {
		tok = scan(file);
		ASSERT(expression(file))
		tok = scan(file);
		ASSERT(tok->type == T_SEMICOLON)
	}
	else return 0;
	return 1;
}

int statement(FILE *file) {
	switch (tok->type) {
	case T_IDENT:
		ASSERT(assignment_statement(file))
		break;
	case T_IF:
		ASSERT(if_statement(file))
		break;
	case T_FOR:
		ASSERT(for_statement(file))
		break;
	case T_RETURN:
		ASSERT(return_statement(file))
		break;
	default:
		return 0;
	}
	return 1;
}

int variable_declaration(FILE *file) {
	ASSERT(tok->type == T_IDENT)
	tok = scan(file);
	ASSERT(tok->type == T_COLON)
	tok = scan(file);
	ASSERT(tok->type == T_TYPE)
	tok = scan(file);
	if (tok->type == T_LBRACK) {
		tok = scan(file);
		ASSERT(tok->subtype == T_ST_INT_LIT)
		tok = scan(file);
		ASSERT(tok->type == T_RBRACK);
	}
	else unscan(tok);
	return 1;
}

int parameter_list(FILE *file) {
	ASSERT(variable_declaration(file))
	tok = scan(file);
	if (tok->type == T_COMMA) {
		tok = scan(file);
		ASSERT(tok->type = T_VARIABLE)
		tok = scan(file);
		ASSERT(parameter_list(file))
	}
	else unscan(tok);
	return 1;
}

int declaration(FILE *file);

int procedure_body(FILE *file) {
	while (tok->type != T_BEGIN) {
		ASSERT(declaration(file))
		tok = scan(file);
	}
	tok = scan(file);
	while (tok->type != T_END) {
		ASSERT(statement(file))
		tok = scan(file);
	}
	tok = scan(file);
	ASSERT(tok->type == T_PROCEDURE)
	return 1;
}

int procedure_declaration(FILE *file) {
	ASSERT(tok->type == T_IDENT)
	tok = scan(file);
	ASSERT(tok->type == T_COLON)
	tok = scan(file);
	ASSERT(tok->type == T_TYPE)
	tok = scan(file);
	ASSERT(tok->type == T_LPAREN)
	tok = scan(file);
	if (tok->type == T_VARIABLE) {
		tok = scan(file);
		ASSERT(parameter_list(file))
	}
	else unscan(tok);
	tok = scan(file);
	ASSERT(tok->type == T_RPAREN)
	tok = scan(file);
	ASSERT(procedure_body(file))
	return 1;
}

int declaration(FILE *file) {
	if (tok->type == T_GLOBAL) {
		tok = scan(file);
	}
	if (tok->type == T_PROCEDURE) {
		tok = scan(file);
		ASSERT(procedure_declaration(file))
	}
	else if (tok->type == T_VARIABLE) {
		tok = scan(file);
		ASSERT(variable_declaration(file))
	}
	tok = scan(file);
	ASSERT(tok->type == T_SEMICOLON)
	return 1;
}

int program_body(FILE *file) {
	while (tok->type != T_BEGIN) {
		ASSERT(declaration(file))
		tok = scan(file);
	}
	tok = scan(file);
	while (tok->type != T_END) {
		ASSERT(statement(file))
		tok = scan(file);
	}
	tok = scan(file);
	ASSERT(tok->type == T_PROGRAM)
	return 1;
}

int program(FILE *file) {
	ASSERT(tok->type == T_PROGRAM)
	tok = scan(file);
	ASSERT(tok->type == T_IDENT)
	tok = scan(file);
	ASSERT(tok->type == T_IS)
	tok = scan(file);
	ASSERT(program_body(file))
	tok = scan(file);
	ASSERT(tok->type == T_PERIOD)
	return 1;
}

int parse(FILE *file) {
	tok = scan(file);
	ASSERT(program(file))
	tok = scan(file);
	ASSERT(tok->type == T_EOF)
	return 1;
}

int compile(FILE *file) {
	
	init_symbol_table();
	//parse_tree_root = ast_create();

	int output = parse(file);

	printf("Parse: %d\n", output);

	destroy_symbol_table();
	//ast_destroy(parse_tree_root);

	return 0;
}

int main(int argc, char *argv[]) {
    
    if (argc < 2) {
        printf("fatal error: No input files\n");
        return 1;
    }
	
	file_data.name = argv[1];
    FILE *input_file = fopen(file_data.name, "r");

    if (input_file == NULL) {
        perror("fatal error: ");
        return 1;
    }

    compile(input_file);
    fclose(input_file);
    exit(0);
}