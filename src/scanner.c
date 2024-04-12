#include <stddef.h>

#include "compiler/scanner.h"

char *file_name = NULL;
int line_num = 1;
token *tok = NULL;
stc *symbol_tables = NULL;

static int unscanned = 0;

void init_res_words() {
	size_t rw_len = sizeof(RES_WORDS) / sizeof(char*);

	for (size_t i = 0; i < rw_len; i++) {
		token *rw_token = (token*)malloc(sizeof(token));
		rw_token->type = RW_TOKEN_TYPES[i];
		rw_token->subtype = RW_TOKEN_SUBTYPES[i];
		strcpy(rw_token->display_name, RES_WORDS[i]);
        rw_token->sym_type = ST_RW;
        rw_token->sym_val_type = SVT_NONE;
		stc_put_res_word(symbol_tables, RES_WORDS[i], rw_token);
	}
}

int get_char(FILE *file) {
	int c = getc(file);
	if (c == '\n') {
		line_num++;
	}
	return c;
}

void unget_char(int c, FILE *file) {
	ungetc(c, file);
	if (c == '\n') {
		line_num--;
	}
}

void ignore_whitespace(int *c_addr, FILE *file) {
	while (isspace(*c_addr)) {
		*c_addr = get_char(file);
	}
}

void ignore_comments_whitespace(int *c_addr, FILE *file) {
	ignore_whitespace(c_addr, file);

	if (*c_addr == '/') {
		int comment_line_num = line_num;
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
					print_error(file_name, UNCLOSED_COMMENT, comment_line_num);
					break;
				}

				next_c = get_char(file);
				if (next_c == EOF) {
					*c_addr = next_c;
					print_error(file_name, UNCLOSED_COMMENT, comment_line_num);
					break;
				}
				if (next_c == '/') {
					comment_level--;
				}
			}

			if (*c_addr != EOF && next_c != EOF) {
				*c_addr = get_char(file);
				ignore_comments_whitespace(c_addr, file);
			}
		}

		// Single line
		else if (next_c == '/') {
			do {
				*c_addr = get_char(file);
			} while (*c_addr != '\n' && *c_addr != EOF);
			
			if (*c_addr != EOF) {
				*c_addr = get_char(file);
				ignore_comments_whitespace(c_addr, file);
			}
		}

		else {
			unget_char(next_c, file);
		}
	}
}

void unscan(token *t) {
	//printf("(Unscanned token: %d)\n", t->type);
	unscanned = 1;
}

void scan(FILE *file) {
	if (unscanned) {
		//print("(Rescanned token: %d)\n", tok->type);
		unscanned = 0;
		return;
	}
	else if (tok != NULL && !is_symbol(tok)) {
		free(tok);
	}
	
	int c;
	c = get_char(file);
	ignore_comments_whitespace(&c, file);

	tok = (token*)malloc(sizeof(token));
	tok->type = T_UNKNOWN;
	tok->subtype = T_ST_NONE;
    tok->sym_type = ST_NONE;
	tok->sym_val_type = SVT_NONE;
    tok->sym_len = 0;
    tok->num_args = 0;
    tok->proc_arg_types = NULL;

	switch (c) {
	case T_PERIOD:
	case T_SEMICOLON:
	case T_LPAREN:
	case T_RPAREN:
	case T_COMMA:
	case T_LBRACK:
	case T_RBRACK:
		tok->type = (token_type)c;
		tok->display_name[0] = c;
        tok->display_name[1] = '\0';
		break;
	case T_ST_AND:
	case T_ST_OR:
		tok->type = T_EXPR_OP;
		tok->subtype = (token_subtype)c; 
		tok->display_name[0] = c;
        tok->display_name[1] = '\0';
		break;
	case '+':
	case '-':
		tok->type = T_ARITH_OP;
		tok->subtype = (token_subtype)c; 
		tok->display_name[0] = c;
        tok->display_name[1] = '\0';
		break;
	case '*':
	case '/':
		tok->type = T_TERM_OP;
		tok->subtype = (token_subtype)c; 
		tok->display_name[0] = c;
        tok->display_name[1] = '\0';
		break;
	case '<':
		{
			tok->type = T_REL_OP;
			tok->display_name[0] = '<';
			char next_c = get_char(file);
			if (next_c == '=') {
				tok->subtype = T_ST_LTEQL;
                tok->display_name[1] = '=';
                tok->display_name[2] = '\0';
			}
			else {
				unget_char(next_c, file);
				tok->subtype = T_ST_LTHAN;
                tok->display_name[1] = '\0';
			}
		}
		break;
	case '>':
		{
			tok->type = T_REL_OP;
            tok->display_name[0] = '>';
			char next_c = get_char(file);
			if (next_c == '=') {
				tok->subtype = T_ST_GTEQL;
				tok->display_name[1] = '=';
                tok->display_name[2] = '\0';
			}
			else {
				unget_char(next_c, file);
				tok->subtype = T_ST_GTHAN;
                tok->display_name[1] = '\0';
			}
		}
		break;
	case '=':
		{
			char next_c = get_char(file);
			if (next_c == '=') {
				tok->type = T_REL_OP;
				tok->subtype = T_ST_EQLTO;
				tok->display_name[0] = '=';
                tok->display_name[1] = '=';
                tok->display_name[2] = '\0';
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
				tok->display_name[0] = '!';
                tok->display_name[1] = '=';
                tok->display_name[2] = '\0';
			}
			else {
				// illegal, ignore
				unget_char(next_c, file);
			}
		}
		break;
	case ':':
		{
			tok->display_name[0] = ':';
            char next_c = get_char(file);
			if (next_c == '=') {
				tok->type = T_ASSMT;
                tok->display_name[1] = '=';
                tok->display_name[2] = '\0';
			}
			else {
				unget_char(next_c, file);
				tok->type = T_COLON;
                tok->display_name[1] = '\0';
			}
		}
		break;
	case '\"':
		{
			tok->type = T_LITERAL;
            tok->subtype = T_ST_STR_LIT;
            strcpy(tok->display_name, "string literal");
			tok->sym_val_type = SVT_STR;

			int str_line_num = line_num;
            size_t i;
			c = get_char(file);
			for (i = 0; c != '\"' && c != EOF && i < MAX_TOKEN_LEN - 1; i++) {
				tok->lit_val.str_val[i] = c;
				c = get_char(file);
			}
            tok->lit_val.str_val[i] = '\0';

            if (i == MAX_TOKEN_LEN - 1) {
                print_error(file_name, TOKEN_TOO_LONG, str_line_num, tok->display_name);
            }
            if (c == EOF) {
                print_error(file_name, UNCLOSED_STRING, str_line_num);
            }

		}
		break;
	case 'A'...'Z':
	case 'a'...'z':
		{
			int id_line_num = line_num;
            size_t i;
			for (i = 0; isalnum(c) || c == '_' && i < MAX_TOKEN_LEN - 1; i++) {
				tok->display_name[i] = toupper(c);
				c = get_char(file);
			}
            tok->display_name[i] = '\0';
			unget_char(c, file);

            if (i == MAX_TOKEN_LEN - 1) {
                print_error(file_name, TOKEN_TOO_LONG, id_line_num, "identifier");
                return;
            }
			
			token *existing_token = stc_search_res_word(symbol_tables, tok->display_name);
			if (existing_token == NULL) {
				tok->type = T_IDENT;
			} else {
				free(tok);
				tok = existing_token;
			}
		}
		break;
	case '0'...'9':
		{
			tok->type = T_LITERAL;
            strcpy(tok->display_name, "numeric literal");
			
			int num_line_num = line_num;
			int dec_pt_cnt = 0;
            char buffer[MAX_TOKEN_LEN];
			size_t i;
			for (i = 0; isdigit(c) || c == '.' && i < MAX_TOKEN_LEN - 1; i++) {
				buffer[i] = c;
				c = get_char(file);
				if (c == '.') dec_pt_cnt++;
			}
            buffer[i] = '\0';
			unget_char(c, file);

            if (i == MAX_TOKEN_LEN - 1) {
                print_error(file_name, TOKEN_TOO_LONG, num_line_num, tok->display_name);
                scan(file);
                return;
            }

			if (dec_pt_cnt > 1) {
				print_error(file_name, EXTRA_DECIMAL_POINT, num_line_num);
				scan(file);
                return;
			}
			else if (dec_pt_cnt == 1) {
				tok->subtype = T_ST_FLOAT_LIT;
				tok->sym_val_type = SVT_FLT;
				tok->lit_val.flt_val = atof(buffer);
			}
			else {
				tok->subtype = T_ST_INT_LIT;
				tok->sym_val_type = SVT_INT;
				tok->lit_val.int_val = atoi(buffer);
			}
		}
		break;
	case EOF:
		tok->type = T_EOF;
		strcpy(tok->display_name, "end of file");
		break;
	default:
		print_error(file_name, UNRECOGNIZED_TOKEN, line_num, (char[2]){(char)c, '\0'});
        scan(file);
		return;
	}
	//printf("Scanned token: %d\n", tok->type);
}