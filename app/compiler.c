#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include "compiler/hash_table.h"

// Token Types

// Single character only (never part of a multi-character token)
#define T_PERIOD '.'
#define T_SEMICOLON ';'
#define T_LPAREN '('
#define T_RPAREN ')'
#define T_COMMA ','
#define T_LBRACK '['
#define T_RBRACK ']'
#define T_AND '&'
#define T_OR '|'
#define T_PLUS '+'
#define T_MINUS '-'
#define T_MULT '*'
#define T_DIVIDE '/' // part of comment indicators, but comments aren't tokens

// Multiple* characters (* -> some are single character but are the same as the starting character of another token)
#define T_PROGRAM 256
#define T_IS 257
#define T_BEGIN 258
#define T_END 259
#define T_GLOBAL 260
#define T_PROCEDURE 261
#define T_VARIABLE 262
#define T_IF 263
#define T_THEN 264
#define T_ELSE 265
#define T_RETURN 266
#define T_TRUE 267
#define T_FALSE 268
#define T_NOT 269

#define T_LTHAN 270
#define T_GTHAN 271
#define T_LTEQL 272
#define T_GTEQL 273
#define T_EQLTO 274
#define T_NOTEQ 275

#define T_COLON 276
#define T_ASSMT 277

#define T_IDENT 278
#define T_INTEGER 279
#define T_FLOAT 280
#define T_STRING 281
#define T_BOOL 282

#define T_INT_LIT 283
#define T_FLOAT_LIT 284
#define T_STR_LIT 285

#define T_EOF 286
#define T_UNKNOWN 287

const char *RSVD_WORDS[18] =
	{"PROGRAM", "IS", "BEGIN", "END",
	 "GLOBAL", "PROCEDURE", "VARIABLE", "IF",
 	 "THEN", "ELSE", "RETURN", "TRUE",
 	 "FALSE", "NOT", "INTEGER", "FLOAT",
 	 "STRING", "BOOL"};

const int RW_TOKEN_SUBTYPES[18] =
	{T_PROGRAM, T_IS, T_BEGIN, T_END,
	 T_GLOBAL, T_PROCEDURE, T_VARIABLE, T_IF,
 	 T_THEN, T_ELSE, T_RETURN, T_TRUE,
 	 T_FALSE, T_NOT, T_INTEGER, T_FLOAT,
 	 T_STRING, T_BOOL};

typedef struct {
	int type;
	int subtype; // might use later
	union {
		int int_val;
		int bool_val;
		float flt_val;
		char str_val[256];
	} lex_val;
} token_t;

ht *symbol_table = NULL;

struct file_data {
	char* name;
	int line_num;
	int col_num;
	int last_line_end;
} file_data = {"", 1, 0, 0};

typedef enum {ILLEGAL_CHAR, UNCLOSED_COMMENT, UNCLOSED_STR, EXTRA_DEC_PT} error_type;

void print_error(error_type type, int line, int c, char* quote) {
	printf("%s:%d:%d: ", file_data.name, line, c);
	switch (type)
	{
	case ILLEGAL_CHAR:
		printf("error: illegal character '%s'", quote);
		break;
	case UNCLOSED_COMMENT:
		printf("error: unterminated comment");
		break;
	case UNCLOSED_STR:
		printf("error: mising terminating \" character");
		break;
	case EXTRA_DEC_PT:
		printf("error: too many decimal points in number");
		break;
	}
	printf("\n");
}

void init_symbol_table() {
	symbol_table = ht_create();
	size_t kw_len = sizeof(RSVD_WORDS) / sizeof(char*);

	for (size_t i = 0; i < kw_len; i++) {
		token_t *kw_token = (token_t*)malloc(sizeof(token_t));
		kw_token->type = RW_TOKEN_SUBTYPES[i];
		strcpy(kw_token->lex_val.str_val, RSVD_WORDS[i]);
		ht_set(symbol_table, RSVD_WORDS[i], kw_token);
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
	file_data.col_num++;
	if (c == '\n') {
		file_data.last_line_end = file_data.col_num;
		file_data.line_num++;
		file_data.col_num = 0;
	}
	return c;
}

int unget_char(int c, FILE *file) {
	ungetc(c, file);
	file_data.col_num--;
	if (file_data.col_num == -1) {
		file_data.line_num--;
		file_data.col_num = file_data.last_line_end;
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
		int comment_col_num = file_data.col_num;
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
					print_error(UNCLOSED_COMMENT, comment_line_num, comment_col_num, "");
					break;
				}

				next_c = get_char(file);
				if (next_c == EOF) {
					*c_addr = next_c;
					print_error(UNCLOSED_COMMENT, comment_line_num, comment_col_num, "");
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

token_t *scan(FILE *file) {
	int c;

	ignore_comments_whitespace(&c, file);

	token_t *token = (token_t*)malloc(sizeof(token_t));
	*token = (token_t){T_UNKNOWN, 0};

	switch (c) {
	case T_PERIOD:
	case T_SEMICOLON:
	case T_LPAREN:
	case T_RPAREN:
	case T_COMMA:
	case T_LBRACK:
	case T_RBRACK:
	case T_AND:
	case T_OR:
	case T_PLUS:
	case T_MINUS:
	case T_MULT:
	case T_DIVIDE:
		token->type = c;
		break;
	case '<':
		{
			char next_c = get_char(file);
			if (next_c == '=') {
				token->type = T_LTEQL;
			}
			else {
				unget_char(next_c, file);
				token->type = T_LTHAN;
			}
		}
		break;
	case '>':
		{
			char next_c = get_char(file);
			if (next_c == '=') {
				token->type = T_GTEQL;
			}
			else {
				unget_char(next_c, file);
				token->type = T_GTHAN;
			}
		}
		break;
	case '=':
		{
			char next_c = get_char(file);
			if (next_c == '=') {
				token->type = T_EQLTO;
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
				token->type = T_NOTEQ;
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
				token->type = T_ASSMT;
			}
			else {
				unget_char(next_c, file);
				token->type = T_COLON;
			}
		}
		break;
	case '\"':
		{
			token->type = T_STR_LIT;
			int str_line_num = file_data.line_num;
			int str_col_num = file_data.col_num;
			size_t i = 0;
			c = get_char(file);
			while (c != '\"' && c != EOF) {
				token->lex_val.str_val[i] = c;
				c = get_char(file);
				i++;
			}
			token->lex_val.str_val[i] = '\0';

			if (c == EOF)
				print_error(UNCLOSED_STR, str_line_num, str_col_num, "");
		}
		break;
	case 'A'...'Z':
	case 'a'...'z':
		{
			token->type = T_IDENT;
			size_t i = 0;
			do {
				token->lex_val.str_val[i] = toupper(c);
				c = get_char(file);
				i++;
			} while (isalnum(c) || c == '_');
			unget_char(c, file);
			token->lex_val.str_val[i] = '\0';
			
			token_t *existing_token = ht_get(symbol_table, token->lex_val.str_val);
			if (existing_token == NULL) {
				ht_set(symbol_table, token->lex_val.str_val, token);
			} else {
				free(token);
				token = existing_token;
			}
		}
		break;
	case '0'...'9':
		{
			int num_line_num = file_data.line_num;
			int num_col_num = file_data.col_num;
			int dec_pt_cnt = 0;
			size_t i = 0;
			do {
				token->lex_val.str_val[i] = toupper(c);
				c = get_char(file);
				i++;
				if (c == '.') dec_pt_cnt++;
			} while (isdigit(c) || c == '.');
			unget_char(c, file);
			token->lex_val.str_val[i] = '\0';

			if (dec_pt_cnt > 1) {
				print_error(EXTRA_DEC_PT, num_line_num, num_col_num, "");
				token->type = T_FLOAT_LIT;
			}
			else if (dec_pt_cnt == 1) {
				token->type = T_FLOAT_LIT;
				token->lex_val.flt_val = atof(token->lex_val.str_val);
			}
			else {
				token->type = T_INT_LIT;
				token->lex_val.int_val = atoi(token->lex_val.str_val);
			}
		}
		break;
	case EOF:
		token->type = T_EOF;
		break;
	default:
		print_error(ILLEGAL_CHAR, file_data.line_num, file_data.col_num, (char[2]){(char)c, '\0'});
		break;
	}

	return token;
}

int compile(FILE *file) {
	
	init_symbol_table();

	int t_type;
	do {
		token_t *token = scan(file);
		t_type = token->type;
		printf("%d\n", t_type);
		if (ht_get(symbol_table, token->lex_val.str_val) == NULL) {
			// free all tokens not in the symbol table
			free(token);
		}
	} while (t_type != T_EOF);

	hti iter = ht_iterator(symbol_table);
	while(ht_next(&iter)) {
		printf("%s: %p\n", iter.key, iter.value);
	}

	destroy_symbol_table();
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