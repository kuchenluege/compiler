#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include "compiler/hash_table.h"
#include "compiler/abstract_syntax_tree.h"
#include "compiler/token.h"
#include "compiler/error.h"

struct file_data {
	char* name;
	int line_num;
} file_data = {"", 1};

ht *symbol_table = NULL;
ast *parse_tree_root = NULL;
token *next_token = NULL;

typedef struct {
	int is_valid;
} return_type;

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

int unget_char(int c, FILE *file) {
	ungetc(c, file);
	if (c == '\n') {
		file_data.line_num--;
	}
	return c;
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

token *scan(FILE *file) {
	int c;

	ignore_comments_whitespace(&c, file);

	token *tok = (token*)malloc(sizeof(token));
	tok->type = T_UNKNOWN;
	tok->subtype = T_ST_NONE;
	tok->tag = T_TAG_NONE;

	switch (c) {
	case T_PERIOD:
	case T_SEMICOLON:
	case T_LPAREN:
	case T_RPAREN:
	case T_COMMA:
	case T_LBRACK:
	case T_RBRACK:
		tok->type = c;
		break;
	case '&':
		tok->type = T_EXPR_OP;
		tok->subtype = T_ST_AND; 
		break;
	case '|':
		tok->type = T_EXPR_OP;
		tok->subtype = T_ST_OR;
		break;
	case '+':
		tok->type = T_ARITH_OP;
		tok->subtype = T_ST_PLUS; 
		break;
	case '-':
		tok->type = T_ARITH_OP;
		tok->subtype = T_ST_MINUS;
		break;
	case '*':
		tok->type = T_TERM_OP;
		tok->subtype = T_ST_MULT;
		break;
	case '/':
		tok->type = T_TERM_OP;
		tok->subtype = T_ST_DIVIDE;
		break;
	case '<':
		{
			tok->type = T_REL_OP;
			char next_c = get_char(file);
			if (next_c == '=') {
				tok->subtype = T_ST_LTEQL;
			}
			else {
				unget_char(next_c, file);
				tok->subtype = T_ST_LTHAN;
			}
		}
		break;
	case '>':
		{
			tok->type = T_REL_OP;
			char next_c = get_char(file);
			if (next_c == '=') {
				tok->subtype = T_ST_GTEQL;
			}
			else {
				unget_char(next_c, file);
				tok->subtype = T_ST_GTHAN;
			}
		}
		break;
	case '=':
		{
			char next_c = get_char(file);
			if (next_c == '=') {
				tok->type = T_REL_OP;
				tok->subtype = T_ST_EQLTO;
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
				tok->type = T_REL_OP;
				tok->subtype = T_ST_NOTEQ;
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
				tok->type = T_ASSMT;
			}
			else {
				unget_char(next_c, file);
				tok->type = T_COLON;
			}
		}
		break;
	case '\"':
		{
			tok->type = T_STR_LIT;
			tok->tag = T_TAG_STR;
			int str_line_num = file_data.line_num;
			size_t i = 0;
			c = get_char(file);
			while (c != '\"' && c != EOF) {
				tok->lex_val.str_val[i] = c;
				c = get_char(file);
				i++;
			}
			tok->lex_val.str_val[i] = '\0';

			if (c == EOF)
				print_error(file_data.name, UNCLOSED_STR, str_line_num, "");
		}
		break;
	case 'A'...'Z':
	case 'a'...'z':
		{
			tok->type = T_IDENT;
			size_t i = 0;
			do {
				tok->lex_val.str_val[i] = toupper(c);
				c = get_char(file);
				i++;
			} while (isalnum(c) || c == '_');
			unget_char(c, file);
			tok->lex_val.str_val[i] = '\0';
			
			token *existing_token = ht_get(symbol_table, tok->lex_val.str_val);
			if (existing_token == NULL) {
				ht_set(symbol_table, tok->lex_val.str_val, tok);
			} else {
				free(tok);
				tok = existing_token;
			}
		}
		break;
	case '0'...'9':
		{
			tok->type = T_NUM_LIT;
			
			int num_line_num = file_data.line_num;
			int dec_pt_cnt = 0;
			size_t i = 0;
			do {
				tok->lex_val.str_val[i] = toupper(c);
				c = get_char(file);
				i++;
				if (c == '.') dec_pt_cnt++;
			} while (isdigit(c) || c == '.');
			unget_char(c, file);
			tok->lex_val.str_val[i] = '\0';

			if (dec_pt_cnt > 1) {
				print_error(file_data.name, EXTRA_DEC_PT, num_line_num, "");
				tok->type = T_UNKNOWN;
			}
			else if (dec_pt_cnt == 1) {
				tok->subtype = T_ST_FLOAT_LIT;
				tok->tag = T_TAG_FLT;
				tok->lex_val.flt_val = atof(tok->lex_val.str_val);
			}
			else {
				tok->subtype = T_ST_INT_LIT;
				tok->tag = T_TAG_INT;
				tok->lex_val.int_val = atoi(tok->lex_val.str_val);
			}
		}
		break;
	case EOF:
		tok->type = T_EOF;
		break;
	default:
		print_error(file_data.name, UNREC_TOKEN, file_data.line_num, (char[2]){(char)c, '\0'});
		break;
	}

	return tok;
}
/*
return_type factor(FILE *file) {
	next_token = scan(file);
	if (next_token->type == '(') {
		return_type ret = factor(file);
		if (!ret.is_valid) {
			return ret;
		}
		next_token = scan(file);
		if (next_token->type == ')') {
			return (return_type){1};
		}
		return (return_type){2};
	}
	if (next_token->type == '-') {
		next_token = scan(file);
		if (next_token->type == T_IDENT) {
			return (return_type){1};
		}
		if (next_token->type == T_NUM_LIT) {
			return (return_type){1};
		}
		return (return_type){2};
	}
	if (next_token->type == )
}

return_type parse(FILE *file) {
	return factor(file);
}
*/

int compile(FILE *file) {
	
	init_symbol_table();
	parse_tree_root = ast_create();

	// return_type output = parse(file);

	while (scan(file)->type != T_EOF) {}

	destroy_symbol_table();
	//ast_destroy(root);

	return 0;
	//return output.is_valid;
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