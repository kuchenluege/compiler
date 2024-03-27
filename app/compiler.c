#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include "compiler/symbol_table_chain.h"
#include "compiler/error.h"

#define ASSERT(X) if (!(X)) return (return_type){0, ET_NONE};
#define ASSERT_TOKEN(X, X_STR) if (tok->type != X) {\
	if (tok->type == T_IDENT) {\
        print_error(file_data.name, MISSING_TOKEN_FOUND_OTHER, file_data.line_num, X_STR, "identifier");\
    }\
    else if (tok->type == T_LITERAL && tok->subtype != T_ST_TRUE && tok->subtype != T_ST_FALSE) {\
        print_error(file_data.name, MISSING_TOKEN_FOUND_OTHER, file_data.line_num, X_STR, tok->display_name);\
    }\
    else {\
        print_error(file_data.name, MISSING_TOKEN_FOUND_TOKEN, file_data.line_num, X_STR, tok->display_name);\
    }\
	return (return_type){0, ET_NONE};\
}
#define ASSERT_OTHER(X, X_STR) if (!(X)) {\
	if (tok->type == T_IDENT) {\
        print_error(file_data.name, MISSING_OTHER_FOUND_OTHER, file_data.line_num, X_STR, "identifier");\
    }\
    else if (tok->type == T_LITERAL && tok->subtype != T_ST_TRUE && tok->subtype != T_ST_FALSE) {\
        print_error(file_data.name, MISSING_OTHER_FOUND_OTHER, file_data.line_num, X_STR, tok->display_name);\
    }\
    else {\
        print_error(file_data.name, MISSING_OTHER_FOUND_TOKEN, file_data.line_num, X_STR, tok->display_name);\
    }\
	return (return_type){0, ET_NONE};\
}

struct file_data {
	char* name;
	int line_num;
} file_data = {"", 1};

typedef enum expr_type {ET_NONE, ET_INTEGER, ET_FLOAT, ET_STRING, ET_BOOL} expr_type;

typedef struct return_type {
    int isValid;
    expr_type type;
} return_type;

stc *symbol_tables = NULL;
token *tok = NULL;
int unscanned = 0;

void init_res_words() {
	size_t rw_len = sizeof(RES_WORDS) / sizeof(char*);

	for (size_t i = 0; i < rw_len; i++) {
		token *rw_token = (token*)malloc(sizeof(token));
		rw_token->type = RW_TOKEN_TYPES[i];
		rw_token->subtype = RW_TOKEN_SUBTYPES[i];
		strcpy(rw_token->display_name, RES_WORDS[i]);
		rw_token->is_symbol = 1;
        rw_token->sym_type = ST_RW;
        rw_token->sym_val_type = ST_NONE;
		stc_put_res_word(symbol_tables, RES_WORDS[i], rw_token);
	}
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
	while (isspace(*c_addr)) {
		*c_addr = get_char(file);
	}
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
					print_error(file_data.name, UNCLOSED_COMMENT, comment_line_num);
					break;
				}

				next_c = get_char(file);
				if (next_c == EOF) {
					*c_addr = next_c;
					print_error(file_data.name, UNCLOSED_COMMENT, comment_line_num);
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
	else if (tok != NULL && !tok->is_symbol) {
		free(tok);
	}
	
	int c;
	c = get_char(file);
	ignore_comments_whitespace(&c, file);

	tok = malloc(sizeof(token));
	tok->type = T_UNKNOWN;
	tok->subtype = T_ST_NONE;
	tok->is_symbol = 0;
    tok->sym_type = ST_NONE;
	tok->sym_val_type = SVT_NONE;
    tok->is_array = 0;
    tok->sym_len = 0;

	switch (c) {
	case T_PERIOD:
	case T_SEMICOLON:
	case T_LPAREN:
	case T_RPAREN:
	case T_COMMA:
	case T_LBRACK:
	case T_RBRACK:
		tok->type = c;
		tok->display_name[0] = c;
        tok->display_name[1] = '\0';
		break;
	case T_ST_AND:
	case T_ST_OR:
		tok->type = T_EXPR_OP;
		tok->subtype = c; 
		tok->display_name[0] = c;
        tok->display_name[1] = '\0';
		break;
	case '+':
	case '-':
		tok->type = T_ARITH_OP;
		tok->subtype = c; 
		tok->display_name[0] = c;
        tok->display_name[1] = '\0';
		break;
	case '*':
	case '/':
		tok->type = T_TERM_OP;
		tok->subtype = c; 
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

			int str_line_num = file_data.line_num;
            size_t i;
			c = get_char(file);
			for (i = 0; c != '\"' && c != EOF && i < MAX_TOKEN_LEN - 1; i++) {
				tok->lit_val.str_val[i] = c;
				c = get_char(file);
			}
            tok->lit_val.str_val[i] = '\0';

            if (i == MAX_TOKEN_LEN - 1) {
                print_error(file_data.name, TOKEN_TOO_LONG, str_line_num, tok->display_name);
            }
            if (c == EOF) {
                print_error(file_data.name, UNCLOSED_STRING, str_line_num);
            }

		}
		break;
	case 'A'...'Z':
	case 'a'...'z':
		{
			int id_line_num = file_data.line_num;
            size_t i;
			for (i = 0; isalnum(c) || c == '_' && i < MAX_TOKEN_LEN - 1; i++) {
				tok->display_name[i] = toupper(c);
				c = get_char(file);
			}
            tok->display_name[i] = '\0';
			unget_char(c, file);

            if (i == MAX_TOKEN_LEN - 1) {
                print_error(file_data.name, TOKEN_TOO_LONG, id_line_num, "identifier");
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
			
			int num_line_num = file_data.line_num;
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
                print_error(file_data.name, TOKEN_TOO_LONG, num_line_num, tok->display_name);
                scan(file);
                return;
            }

			if (dec_pt_cnt > 1) {
				print_error(file_data.name, EXTRA_DECIMAL_POINT, num_line_num);
				scan(file);
                return;
			}
			else if (dec_pt_cnt == 1) {
				tok->subtype = T_ST_FLOAT_LIT;
				tok->sym_val_type = SVT_FLT;
				tok->lit_val.flt_val = atof(tok->display_name);
			}
			else {
				tok->subtype = T_ST_INT_LIT;
				tok->sym_val_type = SVT_INT;
				tok->lit_val.int_val = atoi(tok->display_name);
			}
		}
		break;
	case EOF:
		tok->type = T_EOF;
		strcpy(tok->display_name, "end of file");
		break;
	default:
		print_error(file_data.name, UNRECOGNIZED_TOKEN, file_data.line_num, (char[2]){(char)c, '\0'});
        scan(file);
		return;
	}
	//printf("Scanned token: %d\n", tok->type);
}

return_type expression(FILE *file);

return_type argument_list_prime(FILE *file) {
	ASSERT(expression(file).isValid)
	scan(file);
	if (tok->type == T_COMMA) {
		scan(file);
		ASSERT(argument_list_prime(file).isValid)
	}
	else unscan(tok);
	return (return_type){1, ET_NONE};
}

return_type argument_list(FILE *file) {
	if (tok->type != T_RPAREN) {
		ASSERT(argument_list_prime(file).isValid)
	}
	else unscan(tok);
	return (return_type){1, ET_NONE};
}

return_type location_tail(FILE *file) {
	ASSERT(expression(file).isValid)
	scan(file);
	ASSERT_TOKEN(T_RBRACK, "]")
	return (return_type){1, ET_NONE};
}

return_type location(FILE *file) {
	ASSERT_OTHER(tok->type == T_IDENT, "identifier")
    token *variable = stc_search_local_first(symbol_tables, tok->display_name);
    if (!variable) {
        print_error(file_data.name, UNDECLARED_SYMBOL, file_data.line_num, variable->display_name);
        return (return_type){0, ET_NONE};
    }
    else if (variable->sym_type != ST_VAR) {
        print_error(file_data.name, NONVAR_ASSMT_DEST, file_data.line_num, variable->display_name);
        return (return_type){0, ET_NONE};
    }
	scan(file);
	if (tok->type == T_LBRACK) {
        if (!variable->is_array) {
            print_error(file_data.name, NOT_AN_ARRAY, file_data.line_num, variable->display_name);
            return (return_type){0, ET_NONE};
        }
		scan(file);
		ASSERT(location_tail(file).isValid);
	}
	else unscan(tok);
	return (return_type){1, ET_NONE};
}

return_type procedure_call_tail(FILE *file) {
	ASSERT(argument_list(file).isValid)
	scan(file);
	ASSERT_TOKEN(T_RPAREN, ")")
	return (return_type){1, ET_NONE};
}

return_type ident_tail(FILE *file) {
	if (tok->type == T_LBRACK) {
		scan(file);
		ASSERT(location_tail(file).isValid)
	}
	else if (tok->type == T_LPAREN) {
		scan(file);
		ASSERT(procedure_call_tail(file).isValid)
	} 
	else unscan(tok);
	return (return_type){1, ET_NONE};
}

return_type factor(FILE *file) {
	if (tok->type == T_LPAREN) {
		scan(file);
		ASSERT_OTHER(expression(file).isValid, "expression")
		scan(file);
		ASSERT_TOKEN(T_RPAREN, ")")
	}
	else if (tok->subtype == T_ST_MINUS) {
		scan(file);
		if (tok->type == T_IDENT) {
			scan(file);
			ASSERT(ident_tail(file).isValid)
		}
		else ASSERT_OTHER(tok->subtype == T_ST_INT_LIT || tok->subtype == T_ST_FLOAT_LIT, "identifier or numeric literal")
	}
	else if (tok->type == T_IDENT) {
		scan(file);
		ASSERT(ident_tail(file).isValid);
	}
	else {
        ASSERT_OTHER(tok->type == T_LITERAL, "expression")
    }
	return (return_type){1, ET_NONE};
}

return_type term_prime(FILE *file) {
	if (tok->type == T_TERM_OP) {
		scan(file);
		ASSERT(factor(file).isValid)
		scan(file);
		ASSERT(term_prime(file).isValid)
	}
	else unscan(tok);
	return (return_type){1, ET_NONE};
}

return_type term(FILE *file) {
	ASSERT(factor(file).isValid)
	scan(file);
	ASSERT(term_prime(file).isValid)
	return (return_type){1, ET_NONE};
}

return_type relation_prime(FILE *file) {
	if (tok->type == T_REL_OP) {
		scan(file);
		ASSERT(term(file).isValid)
		scan(file);
		ASSERT(relation_prime(file).isValid)
	}
	else unscan(tok);
	return (return_type){1, ET_NONE};
}

return_type relation(FILE *file) {
	ASSERT(term(file).isValid)
	scan(file);
	ASSERT(relation_prime(file).isValid)
	return (return_type){1, ET_NONE};
}

return_type arith_op_prime(FILE *file) {
	if (tok->type == T_ARITH_OP) {
		scan(file);
		ASSERT(relation(file).isValid)
		scan(file);
		ASSERT(arith_op_prime(file).isValid)
	}
	else unscan(tok);
	return (return_type){1, ET_NONE};
}

return_type arith_op(FILE *file) {
	ASSERT(relation(file).isValid)
	scan(file);
	ASSERT(arith_op_prime(file).isValid)
	return (return_type){1, ET_NONE};
}

return_type expression_prime(FILE *file) {
	if (tok->type == T_EXPR_OP) {
		scan(file);
		ASSERT(arith_op(file).isValid)
		scan(file);
		ASSERT(expression_prime(file).isValid)
	}
	else unscan(tok);
	return (return_type){1, ET_NONE};
}

return_type expression(FILE *file) {
	if (tok->type == T_NOT) {
		scan(file);
	}
	ASSERT(arith_op(file).isValid)
	scan(file);
	ASSERT(expression_prime(file).isValid)
	return (return_type){1, ET_NONE};
}

return_type assignment_statement(FILE *file) {
	ASSERT(location(file).isValid)
	scan(file);
	ASSERT_TOKEN(T_ASSMT, ":=")
	scan(file);
	ASSERT(expression(file).isValid);
	scan(file);
	ASSERT_TOKEN(T_SEMICOLON, ";")
	return (return_type){1, ET_NONE};
}

return_type statement(FILE *file);

return_type if_statement(FILE *file) {
	ASSERT_TOKEN(T_IF, "IF")
	scan(file);
	ASSERT_TOKEN(T_LPAREN, "(")
	scan(file);
	ASSERT(expression(file).isValid)
	scan(file);
	ASSERT_TOKEN(T_RPAREN, ")")
	scan(file);
	ASSERT_TOKEN(T_THEN, "THEN")
	scan(file);
	while (tok->type != T_END && tok->type != T_ELSE) {
		ASSERT(statement(file).isValid)
		scan(file);
	}
	if (tok->type == T_ELSE) {
		scan(file);
		while (tok->type != T_END) {
			ASSERT(statement(file).isValid)
			scan(file);
		}
	}
	scan(file);
	ASSERT_TOKEN(T_IF, "IF")
	scan(file);
	ASSERT_TOKEN(T_SEMICOLON, ";")
	return (return_type){1, ET_NONE};
}

return_type for_statement(FILE *file) {
	ASSERT_TOKEN(T_FOR, "FOR")
	scan(file);
	ASSERT_TOKEN(T_LPAREN, "(")
	scan(file);
	ASSERT(assignment_statement(file).isValid)
	scan(file);
	ASSERT(expression(file).isValid)
	scan(file);
	ASSERT_TOKEN(T_RPAREN, ")")
	scan(file);
	while (tok->type != T_END) {
		ASSERT(statement(file).isValid)
		scan(file);
	}
	scan(file);
	ASSERT_TOKEN(T_FOR, "FOR")
	scan(file);
	ASSERT_TOKEN(T_SEMICOLON, ";")
	return (return_type){1, ET_NONE};
}

return_type return_statement(FILE *file) {
	ASSERT_TOKEN(T_RETURN, "RETURN")
	scan(file);
	ASSERT(expression(file).isValid)
	scan(file);
	ASSERT_TOKEN(T_SEMICOLON, ";")
	return (return_type){1, ET_NONE};
}

return_type statement(FILE *file) {
	switch (tok->type) {
	case T_IDENT:
		ASSERT(assignment_statement(file).isValid)
		break;
	case T_IF:
		ASSERT(if_statement(file).isValid)
		break;
	case T_FOR:
		ASSERT(for_statement(file).isValid)
		break;
	case T_RETURN:
		ASSERT(return_statement(file).isValid)
		break;
	default:
		ASSERT_OTHER(0, "statement")
	}
	return (return_type){1, ET_NONE};
}

return_type variable_declaration(FILE *file, int is_global) {
	ASSERT_OTHER(tok->type == T_IDENT, "identifier")
    token *variable = malloc(sizeof(token));
    memcpy(variable, tok, sizeof(token));
    if (stc_search_local(symbol_tables, variable->display_name) ||
        is_global && stc_search_global(symbol_tables, variable->display_name))
    {
        print_error(file_data.name, DUPLICATE_DECLARATION, file_data.line_num, variable->display_name);
        free(variable);
        return (return_type){0, ET_NONE};
    }
	scan(file);
	ASSERT_TOKEN(T_COLON, ":")
	scan(file);
	ASSERT_OTHER(tok->type == T_TYPE, "type")
    switch (tok->subtype)
    {
    case T_ST_INTEGER:
        variable->sym_val_type = SVT_INT;
        break;
    case T_ST_BOOL:
        variable->sym_val_type = SVT_BOOL;
        break;
    case T_ST_FLOAT:
        variable->sym_val_type = SVT_FLT;
        break;
    case T_ST_STRING:
        variable->sym_val_type = SVT_STR;
        break;
    }
	scan(file);
    int len = 1;
	if (tok->type == T_LBRACK) {
        variable->is_array = 1;
		scan(file);
		if (tok->subtype != T_ST_INT_LIT || tok->lit_val.int_val < 0) {
            print_error(file_data.name, ILLEGAL_ARRAY_LEN, file_data.line_num);
            return (return_type){0, ET_NONE};
        }
        len = tok->lit_val.int_val;
		scan(file);
		ASSERT_TOKEN(T_RBRACK, "]");
	}
	else unscan(tok);
    switch (variable->sym_val_type)
    {
    case SVT_INT:
        variable->sym_val.int_ptr = malloc(len * sizeof(int));
        break;
    case SVT_BOOL:
        variable->sym_val.bool_ptr = malloc(len * sizeof(int));
        break;
    case SVT_FLT:
        variable->sym_val.float_ptr = malloc(len * sizeof(float));
        break;
    case SVT_STR:
        variable->sym_val.str_ptr = malloc(len * MAX_TOKEN_LEN * sizeof(char));
        break;
    }
    variable->is_symbol = 1;
    variable->sym_type = ST_VAR;
    variable->sym_len = len;
    stc_put_local(symbol_tables, variable->display_name, variable);
	return (return_type){1, ET_NONE};
}

return_type parameter_list(FILE *file) {
	ASSERT(variable_declaration(file, 0).isValid)
	scan(file);
	if (tok->type == T_COMMA) {
		scan(file);
		ASSERT_TOKEN(T_VARIABLE, "VARIABLE")
		scan(file);
		ASSERT(parameter_list(file).isValid)
	}
	else unscan(tok);
	return (return_type){1, ET_NONE};
}

return_type declaration(FILE *file);

return_type procedure_body(FILE *file) {
	while (tok->type != T_BEGIN) {
		ASSERT(declaration(file).isValid)
		scan(file);
	}
	scan(file);
	while (tok->type != T_END) {
		ASSERT(statement(file).isValid)
		scan(file);
	}
	scan(file);
	ASSERT_TOKEN(T_PROCEDURE, "PROCEDURE")
	stc_del_local(symbol_tables);
	return (return_type){1, ET_NONE};
}

return_type procedure_declaration(FILE *file, int is_global) {
	ASSERT_OTHER(tok->type == T_IDENT, "identifier")
    token *procedure = malloc(sizeof(token));
    memcpy(procedure, tok, sizeof(token));
    if (stc_search_local(symbol_tables, procedure->display_name) ||
        is_global && stc_search_global(symbol_tables, procedure->display_name))
    {
        print_error(file_data.name, DUPLICATE_DECLARATION, file_data.line_num, procedure->display_name);
        free(procedure);
        return (return_type){0, ET_NONE};
    }
	scan(file);
	ASSERT_TOKEN(T_COLON, ":")
	scan(file);
	ASSERT_OTHER(tok->type == T_TYPE, "type")
    switch (tok->subtype)
    {
    case T_ST_INTEGER:
        procedure->sym_val_type = SVT_INT;
        break;
    case T_ST_BOOL:
        procedure->sym_val_type = SVT_BOOL;
        break;
    case T_ST_FLOAT:
        procedure->sym_val_type = SVT_FLT;
        break;
    case T_ST_STRING:
        procedure->sym_val_type = SVT_STR;
        break;
    }
    procedure->is_symbol = 1;
    procedure->sym_type = ST_PROC;
    stc_put_local(symbol_tables, procedure->display_name, procedure);
    stc_add_local(symbol_tables);
	scan(file);
	ASSERT_TOKEN(T_LPAREN, "(")
	scan(file);
	if (tok->type == T_VARIABLE) {
		scan(file);
		ASSERT(parameter_list(file).isValid)
	}
	else unscan(tok);
	scan(file);
	ASSERT_TOKEN(T_RPAREN, ")")
	scan(file);
	ASSERT(procedure_body(file).isValid)
	return (return_type){1, ET_NONE};
}

return_type declaration(FILE *file) {
    int is_global = 0;
	if (tok->type == T_GLOBAL) {
        is_global = 1;
		scan(file);
	}
	if (tok->type == T_PROCEDURE) {
		scan(file);
		ASSERT(procedure_declaration(file, is_global).isValid)
	}
	else if (tok->type == T_VARIABLE) {
		scan(file);
		ASSERT(variable_declaration(file, is_global).isValid)
	}
	else {
		ASSERT_OTHER(0, "declaration")
	}
	scan(file);
	ASSERT_TOKEN(T_SEMICOLON, ";")
	return (return_type){1, ET_NONE};
}

return_type program_body(FILE *file) {
	while (tok->type != T_BEGIN) {
		ASSERT(declaration(file).isValid)
		scan(file);
	}
	scan(file);
	while (tok->type != T_END) {
		ASSERT(statement(file).isValid)
		scan(file);
	}
	scan(file);
	ASSERT_TOKEN(T_PROGRAM, "PROGRAM")
	return (return_type){1, ET_NONE};
}

return_type program(FILE *file) {
	ASSERT_TOKEN(T_PROGRAM, "PROGRAM")
	scan(file);
	ASSERT_OTHER(tok->type == T_IDENT, "identifier")
    tok->is_symbol = 1;
    tok->sym_type = ST_PROG;
    stc_put_local(symbol_tables, tok->display_name, tok);
	scan(file);
	ASSERT_TOKEN(T_IS, "IS")
	scan(file);
	ASSERT(program_body(file).isValid)
	scan(file);
	ASSERT_TOKEN(T_PERIOD, ".")
	return (return_type){1, ET_NONE};
}

return_type parse(FILE *file) {
	scan(file);
	ASSERT(program(file).isValid)
	scan(file);
	ASSERT_OTHER(tok->type == T_EOF, "end of file")
	return (return_type){1, ET_NONE};
}

int compile(FILE *file) {
	
	symbol_tables = stc_create();
	init_res_words();

	int output = parse(file).isValid;
    if (!tok->is_symbol) free(tok);

	printf("Parse: %d\n", output);

	stc_destroy(symbol_tables);

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