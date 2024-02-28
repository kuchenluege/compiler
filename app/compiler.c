#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include "compiler/symbol_table_chain.h"
#include "compiler/token.h"
#include "compiler/error.h"

#define ASSERT(X) if (!(X)) return 0;
#define ASSERT_TOKEN(X, X_STR) if (tok->type != X) {\
	print_error(file_data.name, MISSING_TOKEN, file_data.line_num, X_STR, tok->display_name);\
	return 0;\
}

struct file_data {
	char* name;
	int line_num;
} file_data = {"", 1};

stc *symbol_tables = NULL;
token *tok = NULL;
int unscanned = 0;

void init_keywords() {
	size_t rw_len = sizeof(RES_WORDS) / sizeof(char*);

	for (size_t i = 0; i < rw_len; i++) {
		token *rw_token = (token*)malloc(sizeof(token));
		rw_token->type = RW_TOKEN_TYPES[i];
		rw_token->subtype = RW_TOKEN_SUBTYPES[i];
		rw_token->display_name = RW_DISP_NAMES[i];
		rw_token->isSymbol = 1;
		strcpy(rw_token->lex_val.str_val, RES_WORDS[i]);
		stc_put_global(symbol_tables, RES_WORDS[i], rw_token);
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
	printf("(Unscanned token: %d)\n", t->type);
	unscanned = 1;
}

void scan(FILE *file) {
	if (unscanned) {
		printf("(Rescanned token: %d)\n", tok->type);
		unscanned = 0;
		return;
	}
	else if (tok != NULL && !tok->isSymbol) {
		free(tok);
	}
	
	int c;
	c = get_char(file);
	ignore_comments_whitespace(&c, file);

	tok = (token*)malloc(sizeof(token));
	tok->type = T_UNKNOWN;
	tok->subtype = T_ST_NONE;
	tok->isSymbol = 0;
	tok->tag = T_TAG_NONE;

	switch (c) {
	case T_PERIOD:
		tok->type = c;
		tok->display_name = "'.'";
		break;
	case T_SEMICOLON:
		tok->type = c;
		tok->display_name = "';'";
		break;
	case T_LPAREN:
		tok->type = c;
		tok->display_name = "'('";
		break;
	case T_RPAREN:
		tok->type = c;
		tok->display_name = "')'";
		break;
	case T_COMMA:
		tok->type = c;
		tok->display_name = "','";
		break;
	case T_LBRACK:
		tok->type = c;
		tok->display_name = "'['";
		break;
	case T_RBRACK:
		tok->type = c;
		tok->display_name = "']'";
		break;
	case '&':
		tok->type = T_EXPR_OP;
		tok->subtype = T_ST_AND; 
		tok->display_name = "'&'";
		break;
	case '|':
		tok->type = T_EXPR_OP;
		tok->subtype = T_ST_OR;
		tok->display_name = "'|'";
		break;
	case '+':
		tok->type = T_ARITH_OP;
		tok->subtype = T_ST_PLUS;
		tok->display_name = "'+'";
		break;
	case '-':
		tok->type = T_ARITH_OP;
		tok->subtype = T_ST_MINUS;
		tok->display_name = "'-'";
		break;
	case '*':
		tok->type = T_TERM_OP;
		tok->subtype = T_ST_MULT;
		tok->display_name = "'*'";
		break;
	case '/':
		tok->type = T_TERM_OP;
		tok->subtype = T_ST_DIVIDE;
		tok->display_name = "'/'";
		break;
	case '<':
		{
			tok->type = T_REL_OP;
			char next_c = get_char(file);
			if (next_c == '=') {
				tok->subtype = T_ST_LTEQL;
				tok->display_name = "'<='";
			}
			else {
				unget_char(next_c, file);
				tok->subtype = T_ST_LTHAN;
				tok->display_name = "'<'";
			}
		}
		break;
	case '>':
		{
			tok->type = T_REL_OP;
			char next_c = get_char(file);
			if (next_c == '=') {
				tok->subtype = T_ST_GTEQL;
				tok->display_name = "'>='";
			}
			else {
				unget_char(next_c, file);
				tok->subtype = T_ST_GTHAN;
				tok->display_name = "'>'";
			}
		}
		break;
	case '=':
		{
			char next_c = get_char(file);
			if (next_c == '=') {
				tok->type = T_REL_OP;
				tok->subtype = T_ST_EQLTO;
				tok->display_name = "'=='";
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
				tok->display_name = "'!='";
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
				tok->display_name = "':='";
			}
			else {
				unget_char(next_c, file);
				tok->type = T_COLON;
				tok->display_name = "':'";
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
			tok->display_name = "string literal";

			if (c == EOF)
				print_error(file_data.name, UNCLOSED_STR, str_line_num, "");
		}
		break;
	case 'A'...'Z':
	case 'a'...'z':
		{
			tok->isSymbol = 1;
			tok->tag = T_TAG_STR;
			size_t i = 0;
			do {
				tok->lex_val.str_val[i] = toupper(c);
				c = get_char(file);
				i++;
			} while (isalnum(c) || c == '_');
			unget_char(c, file);
			tok->lex_val.str_val[i] = '\0';
			
			token *existing_token = stc_search(symbol_tables, tok->lex_val.str_val);
			if (existing_token == NULL) {
				tok->type = T_IDENT;
				tok->display_name = "identifier";
				stc_put_current_scope(symbol_tables, tok->lex_val.str_val, tok);
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
		tok->display_name = "end of file";
		break;
	default:
		print_error(file_data.name, UNREC_TOKEN, file_data.line_num, (char[2]){(char)c, '\0'});
		break;
	}
	printf("Scanned token: %d\n", tok->type);
}

int expression(FILE *file);

int argument_list_prime(FILE *file) {
	ASSERT(expression(file))
	scan(file);
	if (tok->type == T_COMMA) {
		scan(file);
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
	scan(file);
	ASSERT_TOKEN(T_RBRACK, "']'")
	return 1;
}

int location(FILE *file) {
	ASSERT_TOKEN(T_IDENT, "identifier")
	scan(file);
	if (tok->type == T_LBRACK) {
		scan(file);
		ASSERT(location_tail(file));
	}
	else unscan(tok);
	return 1;
}

int procedure_call_tail(FILE *file) {
	ASSERT(argument_list(file))
	scan(file);
	ASSERT_TOKEN(T_RPAREN, "')'")
	return 1;
}

int ident_tail(FILE *file) {
	if (tok->type == T_LBRACK) {
		scan(file);
		ASSERT(location_tail(file))
	}
	else if (tok->type == T_LPAREN) {
		scan(file);
		ASSERT(procedure_call_tail(file))
	} 
	else unscan(tok);
	return 1;
}

int factor(FILE *file) {
	if (tok->type == T_LPAREN) {
		scan(file);
		ASSERT(expression(file))
		scan(file);
		ASSERT_TOKEN(T_RPAREN, "')'")
	}
	else if (tok->subtype == T_ST_MINUS) {
		scan(file);
		if (tok->type == T_IDENT) {
			scan(file);
			ASSERT(ident_tail(file))
		}
		else ASSERT_TOKEN(T_NUM_LIT, "identifier or numeric literal")
	}
	else if (tok->type == T_IDENT) {
		scan(file);
		ASSERT(ident_tail(file));
	}
	else if (tok->type != T_NUM_LIT && tok->type != T_STR_LIT && tok->type != T_BOOL_LIT) {
		print_error(file_data.name, MISSING_TOKEN, file_data.line_num, "factor", tok->display_name);
		return 0;
	}
	return 1;
}

int term_prime(FILE *file) {
	if (tok->type == T_TERM_OP) {
		scan(file);
		ASSERT(factor(file))
		scan(file);
		ASSERT(term_prime(file))
	}
	else unscan(tok);
	return 1;
}

int term(FILE *file) {
	ASSERT(factor(file))
	scan(file);
	ASSERT(term_prime(file))
	return 1;
}

int relation_prime(FILE *file) {
	if (tok->type == T_REL_OP) {
		scan(file);
		ASSERT(term(file))
		scan(file);
		ASSERT(relation_prime(file))
	}
	else unscan(tok);
	return 1;
}

int relation(FILE *file) {
	ASSERT(term(file))
	scan(file);
	ASSERT(relation_prime(file))
	return 1;
}

int arith_op_prime(FILE *file) {
	if (tok->type == T_ARITH_OP) {
		scan(file);
		ASSERT(relation(file))
		scan(file);
		ASSERT(arith_op_prime(file))
	}
	else unscan(tok);
	return 1;
}

int arith_op(FILE *file) {
	ASSERT(relation(file))
	scan(file);
	ASSERT(arith_op_prime(file))
	return 1;
}

int expression_prime(FILE *file) {
	if (tok->type == T_EXPR_OP) {
		scan(file);
		ASSERT(arith_op(file))
		scan(file);
		ASSERT(expression_prime(file))
	}
	else unscan(tok);
	return 1;
}

int expression(FILE *file) {
	if (tok->type == T_NOT) {
		scan(file);
	}
	ASSERT(arith_op(file))
	scan(file);
	ASSERT(expression_prime(file))
	return 1;
}

int assignment_statement(FILE *file) {
	ASSERT(location(file))
	scan(file);
	ASSERT_TOKEN(T_ASSMT, "':='")
	scan(file);
	ASSERT(expression(file));
	scan(file);
	ASSERT_TOKEN(T_SEMICOLON, "';'")
	return 1;
}

int statement(FILE *file);

int if_statement(FILE *file) {
	ASSERT_TOKEN(T_IF, "'IF'")
	scan(file);
	ASSERT_TOKEN(T_LPAREN, "'('")
	scan(file);
	ASSERT(expression(file))
	scan(file);
	ASSERT_TOKEN(T_RPAREN, "')'")
	scan(file);
	ASSERT_TOKEN(T_THEN, "'THEN'")
	scan(file);
	while (tok->type != T_END && tok->type != T_ELSE) {
		ASSERT(statement(file))
		scan(file);
	}
	if (tok->type == T_ELSE) {
		scan(file);
		while (tok->type != T_END) {
			ASSERT(statement(file))
			scan(file);
		}
	}
	scan(file);
	ASSERT_TOKEN(T_IF, "'IF'")
	scan(file);
	ASSERT_TOKEN(T_SEMICOLON, "';'")
	return 1;
}

int for_statement(FILE *file) {
	ASSERT_TOKEN(T_FOR, "'FOR'")
	scan(file);
	ASSERT_TOKEN(T_LPAREN, "'('")
	scan(file);
	ASSERT(assignment_statement(file))
	scan(file);
	ASSERT(expression(file))
	scan(file);
	ASSERT_TOKEN(T_RPAREN, "')'")
	scan(file);
	while (tok->type != T_END) {
		ASSERT(statement(file))
		scan(file);
	}
	scan(file);
	ASSERT_TOKEN(T_FOR, "'FOR'")
	scan(file);
	ASSERT_TOKEN(T_SEMICOLON, "';'")
	return 1;
}

int return_statement(FILE *file) {
	ASSERT_TOKEN(T_RETURN, "'RETURN'")
	scan(file);
	ASSERT(expression(file))
	scan(file);
	ASSERT_TOKEN(T_SEMICOLON, "';'")
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
		print_error(file_data.name, MISSING_TOKEN, file_data.line_num, "statement", tok->display_name);
		return 0;
	}
	return 1;
}

int variable_declaration(FILE *file) {
	ASSERT_TOKEN(T_IDENT, "identifier")
	scan(file);
	ASSERT_TOKEN(T_COLON, "':'")
	scan(file);
	ASSERT_TOKEN(T_TYPE, "type")
	scan(file);
	if (tok->type == T_LBRACK) {
		scan(file);
		ASSERT(tok->subtype == T_ST_INT_LIT)
		scan(file);
		ASSERT_TOKEN(T_RBRACK, "']'");
	}
	else unscan(tok);
	return 1;
}

int parameter_list(FILE *file) {
	ASSERT(variable_declaration(file))
	scan(file);
	if (tok->type == T_COMMA) {
		scan(file);
		ASSERT_TOKEN(T_VARIABLE, "'VARIABLE'")
		scan(file);
		ASSERT(parameter_list(file))
	}
	else unscan(tok);
	return 1;
}

int declaration(FILE *file);

int procedure_body(FILE *file) {
	while (tok->type != T_BEGIN) {
		ASSERT(declaration(file))
		scan(file);
	}
	scan(file);
	while (tok->type != T_END) {
		ASSERT(statement(file))
		scan(file);
	}
	scan(file);
	ASSERT_TOKEN(T_PROCEDURE, "'PROCEDURE'")
	stc_del_local_table(symbol_tables);
	return 1;
}

int procedure_declaration(FILE *file) {
	ASSERT_TOKEN(T_IDENT, "identifier")
	scan(file);
	ASSERT_TOKEN(T_COLON, "':'")
	scan(file);
	ASSERT_TOKEN(T_TYPE, "type")
	scan(file);
	ASSERT_TOKEN(T_LPAREN, "'('")
	scan(file);
	if (tok->type == T_VARIABLE) {
		scan(file);
		ASSERT(parameter_list(file))
	}
	else unscan(tok);
	scan(file);
	ASSERT_TOKEN(T_RPAREN, "')'")
	scan(file);
	ASSERT(procedure_body(file))
	return 1;
}

int declaration(FILE *file) {
	if (tok->type == T_GLOBAL) {
		scan(file);
	}
	if (tok->type == T_PROCEDURE) {
		stc_add_local_table(symbol_tables);
		scan(file);
		ASSERT(procedure_declaration(file))
	}
	else if (tok->type == T_VARIABLE) {
		scan(file);
		ASSERT(variable_declaration(file))
	}
	else {
		print_error(file_data.name, MISSING_TOKEN, file_data.line_num, "declaration", tok->display_name);
		return 0;
	}
	scan(file);
	ASSERT_TOKEN(T_SEMICOLON, "';'")
	return 1;
}

int program_body(FILE *file) {
	while (tok->type != T_BEGIN) {
		ASSERT(declaration(file))
		scan(file);
	}
	scan(file);
	while (tok->type != T_END) {
		ASSERT(statement(file))
		scan(file);
	}
	scan(file);
	ASSERT_TOKEN(T_PROGRAM, "'PROGRAM'")
	return 1;
}

int program(FILE *file) {
	ASSERT_TOKEN(T_PROGRAM, "'PROGRAM'")
	scan(file);
	ASSERT_TOKEN(T_IDENT, "identifier")
	scan(file);
	ASSERT_TOKEN(T_IS, "'IS'")
	scan(file);
	ASSERT(program_body(file))
	scan(file);
	ASSERT_TOKEN(T_PERIOD, "'.'")
	return 1;
}

int parse(FILE *file) {
	scan(file);
	ASSERT(program(file))
	scan(file);
	ASSERT_TOKEN(T_EOF, "end of file")
	if (!tok->isSymbol) {
		free(tok);
	}
	return 1;
}

int compile(FILE *file) {
	
	symbol_tables = stc_create();
	init_keywords();

	int output = parse(file);

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