#include <stdio.h>
#include <stdint.h>
#include <inttypes.h>

#include <llvm-c/Core.h>
#include <llvm-c/ExecutionEngine.h>
#include <llvm-c/Target.h>
#include <llvm-c/Analysis.h>
#include <llvm-c/BitWriter.h>

#include "compiler/scanner.h"

#define ASSERT(X) if (!(X)) return (return_type){0, SVT_NONE};
#define ASSERT_TOKEN(X, X_STR) if (tok->type != X) {\
	if (tok->type == T_IDENT) {\
        print_error(file_name, MISSING_TOKEN_FOUND_OTHER, line_num, X_STR, "identifier");\
    }\
    else if (tok->type == T_LITERAL && tok->subtype != T_ST_TRUE && tok->subtype != T_ST_FALSE) {\
        print_error(file_name, MISSING_TOKEN_FOUND_OTHER, line_num, X_STR, tok->display_name);\
    }\
    else {\
        print_error(file_name, MISSING_TOKEN_FOUND_TOKEN, line_num, X_STR, tok->display_name);\
    }\
	return (return_type){0, SVT_NONE};\
}
#define ASSERT_OTHER(X, X_STR) if (!(X)) {\
	if (tok->type == T_IDENT) {\
        print_error(file_name, MISSING_OTHER_FOUND_OTHER, line_num, X_STR, "identifier");\
    }\
    else if (tok->type == T_LITERAL && tok->subtype != T_ST_TRUE && tok->subtype != T_ST_FALSE) {\
        print_error(file_name, MISSING_OTHER_FOUND_OTHER, line_num, X_STR, tok->display_name);\
    }\
    else {\
        print_error(file_name, MISSING_OTHER_FOUND_TOKEN, line_num, X_STR, tok->display_name);\
    }\
	return (return_type){0, SVT_NONE};\
}

#define VALID (return_type){1, SVT_NONE}
#define INVALID (return_type){0, SVT_NONE}

typedef struct return_type {
    int is_valid;
    symbol_value_type type;
} return_type;

return_type expression(FILE *file);

return_type argument_list_prime(FILE *file, token *proc, int i) {
    return_type expr_res = expression(file);
	ASSERT(expr_res.is_valid)
    if (expr_res.type != proc->proc_arg_types[i]) {
        print_error(file_name, INVALID_ARG_TYPE, line_num,
                    proc->display_name, type_string(proc->proc_arg_types[i]), i + 1, expr_res.type);
        return INVALID;
    }
	scan(file);
	if (tok->type == T_COMMA) {
        if (i + 1 >= proc->num_args) {
            print_error(file_name, UNEXPECTED_TOKEN_IN_PROC_CALL, line_num,
                        tok->display_name, proc->display_name, proc->num_args);
        }
		scan(file);
		ASSERT(argument_list_prime(file, proc, i + 1).is_valid)
	}
	else unscan(tok);
	return VALID;
}

return_type argument_list(FILE *file, token *proc) {
	if (tok->type != T_RPAREN) {
		if (proc->num_args == 0) {
            print_error(file_name, UNEXPECTED_TOKEN_IN_PROC_CALL, line_num,
                        tok->display_name, proc->display_name, proc->num_args);
            return INVALID;
        }
        ASSERT(argument_list_prime(file, proc, 0).is_valid)
	}
	else {
        if (proc->num_args > 0) {
            print_error(file_name, MISSING_ARG, line_num, type_string(proc->proc_arg_types[0]), proc->display_name);
            return INVALID;
        }
        else unscan(tok);
    }
	return VALID;
}

return_type location_tail(FILE *file, token *arr) {
    return_type expr_res = expression(file);
	ASSERT(expr_res.is_valid)
    if (expr_res.type != SVT_INT) {
        print_error(file_name, ILLEGAL_ARRAY_INDEX, line_num);
        return INVALID;
    }
	scan(file);
	ASSERT_TOKEN(T_RBRACK, "]")
	return VALID;
}

return_type location(FILE *file) {
	ASSERT_OTHER(tok->type == T_IDENT, "identifier")
    token *variable = stc_search_local_first(symbol_tables, tok->display_name);
    if (!variable) {
        print_error(file_name, UNDECLARED_SYMBOL, line_num, variable->display_name);
        return INVALID;
    }
    else if (variable->sym_type != ST_VAR) {
        print_error(file_name, NONVAR_ASSMT_DEST, line_num, variable->display_name);
        return INVALID;
    }
	scan(file);
	if (tok->type == T_LBRACK) {
        if (!is_array_type(variable->sym_val_type)) {
            print_error(file_name, NOT_AN_ARRAY, line_num, variable->display_name);
            return INVALID;
        }
		scan(file);
		ASSERT(location_tail(file, variable).is_valid);
        return (return_type){1, type_of_arr_elem(variable->sym_val_type)};
	}
	else {
        unscan(tok);
        return (return_type){1, variable->sym_val_type};
    }
}

return_type procedure_call_tail(FILE *file, token *proc) {
	ASSERT(argument_list(file, proc).is_valid)
	scan(file);
	ASSERT_TOKEN(T_RPAREN, ")")
	return VALID;
}

return_type ident_tail(FILE *file, token *id) {
	if (tok->type == T_LBRACK) {
		if (!is_array_type(id->sym_val_type)) {
            print_error(file_name, NOT_AN_ARRAY, line_num, id->display_name);
            return INVALID;
        }
        scan(file);
		ASSERT(location_tail(file, id).is_valid)
        return (return_type){1, type_of_arr_elem(id->sym_val_type)};
	}
	else if (tok->type == T_LPAREN) {
        if (id->sym_type != ST_PROC) {
            print_error(file_name, NOT_A_PROC, line_num, id->display_name);
            return INVALID;
        }
		scan(file);
		ASSERT(procedure_call_tail(file, id).is_valid)
        return (return_type){1, id->sym_val_type};
	} 
	else {
        unscan(tok);
        return (return_type){1, id->sym_val_type};
    }
}

return_type factor(FILE *file) {
	if (tok->type == T_LPAREN) {
		scan(file);
        return_type expr_res = expression(file);
		ASSERT_OTHER(expr_res.is_valid, "expression")
		scan(file);
		ASSERT_TOKEN(T_RPAREN, ")")
        return expr_res;
	}
	else if (tok->subtype == T_ST_MINUS) {
		scan(file);
		if (tok->type == T_IDENT) {
            token *id = stc_search_local_first(symbol_tables, tok->display_name);
            if (!id) {
                print_error(file_name, UNDECLARED_SYMBOL,  line_num);
                return INVALID;
            }
			scan(file);
            return ident_tail(file, id);
		}
		else {
            ASSERT_OTHER(tok->subtype == T_ST_INT_LIT || tok->subtype == T_ST_FLOAT_LIT, "identifier or numeric literal")
            if (tok->subtype == T_ST_INT_LIT) {
                return (return_type){1, SVT_INT};
            }
            if (tok->subtype == T_ST_FLOAT_LIT) {
                return (return_type){1, SVT_FLT};
            }
        }
	}
	else if (tok->type == T_IDENT) {
        token *id = stc_search_local_first(symbol_tables, tok->display_name);
        if (!id) {
            print_error(file_name, UNDECLARED_SYMBOL, line_num, tok->display_name);
            return INVALID;
        }
		scan(file);
		return ident_tail(file, id);
	}
	else {
        ASSERT_OTHER(tok->type == T_LITERAL, "expression")
        return (return_type){1, svt_from_literal_value_type(tok->subtype)};
    }
}

return_type term_prime(FILE *file, symbol_value_type last_type) {
	if (tok->type == T_TERM_OP) {
        char op_str[256];
        strcpy(op_str, tok->display_name);
        if (last_type != SVT_INT && last_type != SVT_FLT) {
            print_error(file_name, INVALID_OPERAND_TYPE, line_num, op_str, type_string(last_type));
            return INVALID;
        }
		scan(file);
		return_type factor_res = factor(file);
        ASSERT(factor_res.is_valid)
        if (factor_res.type != SVT_INT && factor_res.type != SVT_FLT) {
            print_error(file_name, INVALID_OPERAND_TYPE, line_num, op_str, type_string(factor_res.type));
            return INVALID;
        }
        symbol_value_type current_type;
        if (last_type == SVT_FLT || factor_res.type == SVT_FLT) {
            current_type = SVT_FLT;
        }
        else {
            current_type = SVT_INT;
        }
		scan(file);
        return term_prime(file, current_type);
	}
	else {
        unscan(tok);
        return (return_type){1, last_type};
    }
}

return_type term(FILE *file) {
    return_type factor_res = factor(file);
	ASSERT(factor_res.is_valid)
	scan(file);
	return term_prime(file, factor_res.type);
}

return_type relation_prime(FILE *file, symbol_value_type last_type) {
	if (tok->type == T_REL_OP) {
        token_subtype op_st = tok->subtype;
        char op_str[256];
        strcpy(op_str, tok->display_name);
        if ((last_type == SVT_STR && op_st != T_ST_EQLTO && op_st != T_ST_NOTEQ) || is_array_type(last_type))
        {
            print_error(file_name, INVALID_OPERAND_TYPE, line_num, op_str, type_string(last_type));
            return INVALID;
        }
		scan(file);
        return_type term_res = term(file);
		ASSERT(term_res.is_valid)
        if ((term_res.type == SVT_STR && op_st != T_ST_EQLTO && op_st != T_ST_NOTEQ) || is_array_type(term_res.type))
        {
            print_error(file_name, INVALID_OPERAND_TYPE, line_num, op_str, type_string(term_res.type));
            return INVALID;
        }
        if (last_type != term_res.type && !compatible_types(last_type, term_res.type)) {
            print_error(file_name, INVALID_OPERAND_TYPES, line_num,
                        op_str, type_string(last_type), type_string(term_res.type));
            return INVALID;
        }
		scan(file);
        return relation_prime(file, SVT_BOOL);
	}
	else {
        unscan(tok);
        return (return_type){1, last_type};
    }
}

return_type relation(FILE *file) {
	return_type term_res = term(file);
    ASSERT(term_res.is_valid)
	scan(file);
	return relation_prime(file, term_res.type);
}

return_type arith_op_prime(FILE *file, symbol_value_type last_type) {
	if (tok->type == T_ARITH_OP) {
        char op_str[256];
        strcpy(op_str, tok->display_name);
        if (last_type != SVT_INT && last_type != SVT_FLT) {
            print_error(file_name, INVALID_OPERAND_TYPE, line_num, op_str, type_string(last_type));
            return INVALID;
        }
		scan(file);
        return_type rel_res = relation(file);
		ASSERT(rel_res.is_valid)
        if (rel_res.type != SVT_INT && rel_res.type != SVT_FLT) {
            print_error(file_name, INVALID_OPERAND_TYPE, line_num, op_str, type_string(rel_res.type));
            return INVALID;
        }
        symbol_value_type current_type = (last_type == SVT_FLT || rel_res.type == SVT_FLT)?
                                         SVT_FLT : SVT_FLT;
		scan(file);
		return arith_op_prime(file, current_type);
	}
	else {
        unscan(tok);
        return (return_type){1, last_type};
    }
}

return_type arith_op(FILE *file) {
	return_type rel_res = relation(file);
    ASSERT(rel_res.is_valid)
	scan(file);
	return arith_op_prime(file, rel_res.type);
}

return_type expression_prime(FILE *file, symbol_value_type last_type) {
	if (tok->type == T_EXPR_OP) {
        char op_str[256];
        strcpy(op_str, tok->display_name);
        if (last_type != SVT_INT && last_type != SVT_BOOL) {
            print_error(file_name, INVALID_OPERAND_TYPE, line_num, op_str, type_string(last_type));
            return INVALID;
        }
		scan(file);
		return_type arop_res = arith_op(file);
        ASSERT(arop_res.is_valid)
        if (arop_res.type != SVT_INT && arop_res.type != SVT_BOOL) {
            print_error(file_name, INVALID_OPERAND_TYPE, line_num, op_str, type_string(arop_res.type));
            return INVALID;
        }
        symbol_value_type current_type = (last_type == SVT_INT || arop_res.type == SVT_INT)?
                                         SVT_INT : SVT_BOOL;
		scan(file);
		return expression_prime(file, current_type);
	}
	else {
        unscan(tok);
        return (return_type){1, last_type};
    }
}

return_type expression(FILE *file) {
	if (tok->type == T_NOT) {
        char op_str[256];
        strcpy(op_str, tok->display_name);
		scan(file);
        return_type arop_res = arith_op(file);
        ASSERT(arop_res.is_valid)
        if (arop_res.type != SVT_INT && arop_res.type != SVT_BOOL) {
            print_error(file_name, INVALID_OPERAND_TYPE, line_num, op_str, type_string(arop_res.type));
            return INVALID;
        }
        scan(file);
        return expression_prime(file, arop_res.type);
	}
    else {
        return_type arop_res = arith_op(file);
        ASSERT(arop_res.is_valid)
        scan(file);
        return expression_prime(file, arop_res.type);
    }
}

return_type assignment_statement(FILE *file) {
	return_type loc_res = location(file);
    ASSERT(loc_res.is_valid)
	scan(file);
	ASSERT_TOKEN(T_ASSMT, ":=")
	scan(file);
    return_type expr_res = expression(file);
	ASSERT(expr_res.is_valid);
    if (loc_res.type != expr_res.type && !compatible_types(loc_res.type, expr_res.type)) {
        print_error(file_name, INCOMPATIBLE_TYPE_ASSMT, line_num,
                    type_string(expr_res.type), type_string(loc_res.type));
        return INVALID;
    }
	scan(file);
	ASSERT_TOKEN(T_SEMICOLON, ";")
	return VALID;
}

return_type statement(FILE *file);

return_type if_statement(FILE *file) {
	ASSERT_TOKEN(T_IF, "IF")
	scan(file);
	ASSERT_TOKEN(T_LPAREN, "(")
	scan(file);
    return_type expr_res = expression(file);
	ASSERT(expr_res.is_valid)
    if (expr_res.type != SVT_BOOL && !compatible_types(expr_res.type, SVT_BOOL)) {
        print_error(file_name, NONBOOL_CONDITION, line_num);
        return INVALID;
    }
	scan(file);
	ASSERT_TOKEN(T_RPAREN, ")")
	scan(file);
	ASSERT_TOKEN(T_THEN, "THEN")
	scan(file);
	while (tok->type != T_END && tok->type != T_ELSE) {
		ASSERT(statement(file).is_valid)
		scan(file);
	}
	if (tok->type == T_ELSE) {
		scan(file);
		while (tok->type != T_END) {
			ASSERT(statement(file).is_valid)
			scan(file);
		}
	}
	scan(file);
	ASSERT_TOKEN(T_IF, "IF")
	scan(file);
	ASSERT_TOKEN(T_SEMICOLON, ";")
	return VALID;
}

return_type for_statement(FILE *file) {
	ASSERT_TOKEN(T_FOR, "FOR")
	scan(file);
	ASSERT_TOKEN(T_LPAREN, "(")
	scan(file);
	ASSERT(assignment_statement(file).is_valid)
	scan(file);
	return_type expr_res = expression(file);
	ASSERT(expr_res.is_valid)
    if (expr_res.type != SVT_BOOL && !compatible_types(expr_res.type, SVT_BOOL)) {
        print_error(file_name, NONBOOL_CONDITION, line_num);
        return INVALID;
    }
	scan(file);
	ASSERT_TOKEN(T_RPAREN, ")")
	scan(file);
	while (tok->type != T_END) {
		ASSERT(statement(file).is_valid)
		scan(file);
	}
	scan(file);
	ASSERT_TOKEN(T_FOR, "FOR")
	scan(file);
	ASSERT_TOKEN(T_SEMICOLON, ";")
	return VALID;
}

return_type return_statement(FILE *file) {
	ASSERT_TOKEN(T_RETURN, "RETURN")
	scan(file);
	ASSERT(expression(file).is_valid)
	scan(file);
	ASSERT_TOKEN(T_SEMICOLON, ";")
	return VALID;
}

return_type statement(FILE *file) {
	switch (tok->type) {
	case T_IDENT:
		ASSERT(assignment_statement(file).is_valid)
		break;
	case T_IF:
		ASSERT(if_statement(file).is_valid)
		break;
	case T_FOR:
		ASSERT(for_statement(file).is_valid)
		break;
	case T_RETURN:
		ASSERT(return_statement(file).is_valid)
		break;
	default:
		ASSERT_OTHER(0, "statement")
	}
	return VALID;
}

return_type variable_declaration(FILE *file, token *owning_procedure, int is_parameter) {
	intptr_t is_global = !owning_procedure;
    ASSERT_OTHER(tok->type == T_IDENT, "identifier")
    token *variable = malloc(sizeof(token));
    memcpy(variable, tok, sizeof(token));
    if (stc_search_local(symbol_tables, variable->display_name) ||
        is_global && stc_search_global(symbol_tables, variable->display_name))
    {
        print_error(file_name, DUPLICATE_DECLARATION, line_num, variable->display_name);
        free(variable);
        return INVALID;
    }
	scan(file);
	ASSERT_TOKEN(T_COLON, ":")
	scan(file);
	ASSERT_OTHER(tok->type == T_TYPE, "type")
    token_subtype type_lit = tok->subtype;
	scan(file);
    int len = 1;
    int is_array = 0;
	if (tok->type == T_LBRACK) {
        is_array = 1;
		scan(file);
		if (tok->subtype != T_ST_INT_LIT || tok->lit_val.int_val < 1) {
            print_error(file_name, ILLEGAL_ARRAY_LEN, line_num);
            free(variable);
            return INVALID;
        }
        len = tok->lit_val.int_val;
		scan(file);
		ASSERT_TOKEN(T_RBRACK, "]");
	}
	else unscan(tok);
    variable->sym_type = ST_VAR;
    variable->sym_val_type = svt_from_type_literal(type_lit, is_array);
    variable->sym_len = len;
    switch (variable->sym_val_type)
    {
    case SVT_INT:
    case SVT_INT_ARR:
        variable->sym_val.int_ptr = malloc(len * sizeof(int));
        break;
    case SVT_BOOL:
    case SVT_BOOL_ARR:
        variable->sym_val.bool_ptr = malloc(len * sizeof(int));
        break;
    case SVT_FLT:
    case SVT_FLT_ARR:
        variable->sym_val.float_ptr = malloc(len * sizeof(float));
        break;
    case SVT_STR:
    case SVT_STR_ARR:
        variable->sym_val.str_ptr = malloc(len * MAX_TOKEN_LEN * sizeof(char));
        break;
    }
    stc_put_local(symbol_tables, variable->display_name, variable);
    if (owning_procedure && is_parameter) {
        owning_procedure->num_args++;

        if (!owning_procedure->num_args) {
            owning_procedure->proc_arg_types = malloc(sizeof(symbol_value_type));
            if (!owning_procedure->proc_arg_types) {
                print_error(file_name, OUT_OF_MEMORY, line_num);
                return INVALID;
            }
            owning_procedure->proc_arg_types[0] = variable->sym_val_type;
        }
        else {
            symbol_value_type *tmp = realloc(owning_procedure->proc_arg_types,
                                             sizeof(owning_procedure->proc_arg_types) + sizeof(symbol_value_type));
            if (tmp == NULL) {
                print_error(file_name, OUT_OF_MEMORY, line_num);
                return INVALID;
            }
            owning_procedure->proc_arg_types = tmp;
            owning_procedure->proc_arg_types[owning_procedure->num_args - 1] = variable->sym_val_type;
        }
    }
	return VALID;
}

return_type parameter_list(FILE *file, token *owning_procedure) {
    ASSERT(variable_declaration(file, owning_procedure, 1).is_valid)
	scan(file);
	if (tok->type == T_COMMA) {
		scan(file);
		ASSERT_TOKEN(T_VARIABLE, "VARIABLE")
		scan(file);
		ASSERT(parameter_list(file, owning_procedure).is_valid)
	}
	else unscan(tok);
	return VALID;
}

return_type declaration(FILE *file, token *owning_procedure);

return_type procedure_body(FILE *file, token *owning_procedure) {
	while (tok->type != T_BEGIN) {
		ASSERT(declaration(file, owning_procedure).is_valid)
		scan(file);
	}
	scan(file);
	while (tok->type != T_END) {
		ASSERT(statement(file).is_valid)
		scan(file);
	}
	scan(file);
	ASSERT_TOKEN(T_PROCEDURE, "PROCEDURE")
	stc_del_local(symbol_tables);
	return VALID;
}

return_type procedure_declaration(FILE *file, token *owning_procedure) {
	intptr_t is_global = !owning_procedure;
    ASSERT_OTHER(tok->type == T_IDENT, "identifier")
    token *procedure = malloc(sizeof(token));
    if (procedure == NULL) {
        print_error(file_name, OUT_OF_MEMORY, line_num);
        return INVALID;
    }
    memcpy(procedure, tok, sizeof(token));
    if (stc_search_local(symbol_tables, procedure->display_name) ||
        is_global && stc_search_global(symbol_tables, procedure->display_name))
    {
        print_error(file_name, DUPLICATE_DECLARATION, line_num, procedure->display_name);
        free(procedure);
        return INVALID;
    }
	scan(file);
	ASSERT_TOKEN(T_COLON, ":")
	scan(file);
	ASSERT_OTHER(tok->type == T_TYPE, "type")
    token_subtype type_lit = tok->subtype;
	scan(file);
    int len = 1;
    int is_array = 0;
	if (tok->type == T_LBRACK) {
        is_array = 1;
		scan(file);
		if (tok->subtype != T_ST_INT_LIT || tok->lit_val.int_val < 1) {
            print_error(file_name, ILLEGAL_ARRAY_LEN, line_num);
            free(procedure);
            return INVALID;
        }
        len = tok->lit_val.int_val;
		scan(file);
		ASSERT_TOKEN(T_RBRACK, "]");
	}
	else unscan(tok);
    procedure->sym_type = ST_PROC;
    procedure->sym_val_type = svt_from_type_literal(type_lit, is_array);
    procedure->sym_len = len;
    stc_put_local(symbol_tables, procedure->display_name, procedure);
    stc_add_local(symbol_tables);
	scan(file);
	ASSERT_TOKEN(T_LPAREN, "(")
	scan(file);
	if (tok->type == T_VARIABLE) {
		scan(file);
		ASSERT(parameter_list(file, procedure).is_valid)
	}
	else unscan(tok);
	scan(file);
	ASSERT_TOKEN(T_RPAREN, ")")
	scan(file);
	ASSERT(procedure_body(file, procedure).is_valid)
	return VALID;
}

return_type declaration(FILE *file, token *owning_procedure) {
    token *opt_owning_procedure = owning_procedure;
	if (tok->type == T_GLOBAL) {
        opt_owning_procedure = NULL;
		scan(file);
	}
	if (tok->type == T_PROCEDURE) {
		scan(file);
		ASSERT(procedure_declaration(file, opt_owning_procedure).is_valid)
	}
	else if (tok->type == T_VARIABLE) {
		scan(file);
		ASSERT(variable_declaration(file, opt_owning_procedure, 0).is_valid)
	}
	else {
		ASSERT_OTHER(0, "declaration")
	}
	scan(file);
	ASSERT_TOKEN(T_SEMICOLON, ";")
	return VALID;
}

return_type program_body(FILE *file) {
	while (tok->type != T_BEGIN) {
		ASSERT(declaration(file, NULL).is_valid)
		scan(file);
	}
	scan(file);
	while (tok->type != T_END) {
		ASSERT(statement(file).is_valid)
		scan(file);
	}
	scan(file);
	ASSERT_TOKEN(T_PROGRAM, "PROGRAM")
	return VALID;
}

return_type program(FILE *file) {
	ASSERT_TOKEN(T_PROGRAM, "PROGRAM")
	scan(file);
	ASSERT_OTHER(tok->type == T_IDENT, "identifier")
    tok->sym_type = ST_PROG;
    stc_put_local(symbol_tables, tok->display_name, tok);
	scan(file);
	ASSERT_TOKEN(T_IS, "IS")
	scan(file);
	ASSERT(program_body(file).is_valid)
	scan(file);
	ASSERT_TOKEN(T_PERIOD, ".")
	return VALID;
}

return_type parse(FILE *file) {
	scan(file);
	ASSERT(program(file).is_valid)
	scan(file);
	ASSERT_OTHER(tok->type == T_EOF, "end of file")
	return VALID;
}

int compile(FILE *file) {
    symbol_tables = stc_create();
	init_res_words();

	return_type output = parse(file);
    if (!is_symbol(tok)) free(tok);

	if (output.is_valid) {
        printf("Valid Parse.\n");
    }

    stc_destroy(symbol_tables);

	return 0;
}

int main(int argc, char *argv[]) {
    
    if (argc < 2) {
        printf("error: No input files\n");
        return 1;
    }
	
    file_name = argv[1];
    FILE *input_file = fopen(file_name, "r");

    if (input_file == NULL) {
        perror("error: ");
        return 1;
    }

    compile(input_file);
    fclose(input_file);
    exit(0);
}