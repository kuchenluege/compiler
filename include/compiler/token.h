#ifndef TOKEN_H
#define TOKEN_H

#define MAX_TOKEN_LEN 256

// Token Types

typedef enum token_type {
	T_PERIOD = '.',
	T_SEMICOLON = ';',
	T_COLON = ':',
	T_LPAREN = '(',
	T_RPAREN = ')',
	T_COMMA = ',',
	T_LBRACK = '[',
	T_RBRACK = ']',
	T_PROGRAM,
	T_IS,
	T_BEGIN,
	T_END,
	T_GLOBAL,
	T_PROCEDURE,
	T_VARIABLE,
	T_IF,
	T_THEN,
	T_ELSE,
	T_FOR,
	T_RETURN,
	T_NOT,
	T_EXPR_OP,
	T_ARITH_OP,
	T_REL_OP,
	T_TERM_OP,
	T_ASSMT,
	T_IDENT,
	T_TYPE,
    T_LITERAL,
	T_EOF,
	T_UNKNOWN
} token_type;

typedef enum token_subtype {
	T_ST_NONE = 0,
	T_ST_AND = '&',
	T_ST_OR = '|',
	T_ST_PLUS = '+',
	T_ST_MINUS = '-',
	T_ST_MULT = '*',
	T_ST_DIVIDE = '/',
	T_ST_LTHAN,
	T_ST_GTHAN,
	T_ST_LTEQL,
	T_ST_GTEQL,
	T_ST_EQLTO,
	T_ST_NOTEQ,
	T_ST_INTEGER,
	T_ST_FLOAT,
	T_ST_STRING,
	T_ST_BOOL,
	T_ST_INT_LIT,
	T_ST_FLOAT_LIT,
    T_ST_STR_LIT,
	T_ST_TRUE,
    T_ST_FALSE
} token_subtype;

typedef enum symbol_type {
    ST_NONE = 0, ST_RW, ST_PROG, ST_VAR, ST_PROC
} symbol_type;

typedef enum symbol_value_type {
	SVT_NONE = 0, SVT_INT, SVT_INT_ARR, SVT_BOOL, SVT_BOOL_ARR, SVT_FLT, SVT_FLT_ARR, SVT_STR, SVT_STR_ARR
} symbol_value_type;

extern char *const RES_WORDS[19];
extern const token_type RW_TOKEN_TYPES[19];
extern const token_subtype RW_TOKEN_SUBTYPES[19];

typedef struct token token;
struct token {
	token_type type;
	token_subtype subtype;
	char display_name[MAX_TOKEN_LEN];
	union { 
		int int_val;
		float flt_val;
		char str_val[MAX_TOKEN_LEN];
	} lit_val;
    
    symbol_type sym_type;
	symbol_value_type sym_val_type;
    int sym_len;
    union {
        int *int_ptr;
        int *bool_ptr;
        float *float_ptr;
        char *str_ptr;
    } sym_val;
    int num_args;
    symbol_value_type *proc_arg_types;
};

void free_symbol_token(token *tok);
int is_symbol(token *tok);
char *type_string(symbol_value_type type);
symbol_value_type svt_from_literal_value_type(token_subtype lit_val_type);
symbol_value_type svt_from_type_literal(token_subtype type_lit, int is_array);
int is_array_type(symbol_value_type type);
symbol_value_type type_of_arr_elem(symbol_value_type arr_type);
int compatible_types(symbol_value_type type1, symbol_value_type type2);

#endif