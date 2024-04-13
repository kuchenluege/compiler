#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include "compiler/error.h"

void log_error(char* file_name, error_type type, int line, ...) {
	va_list args;
    va_start(args, line);

	fprintf(stderr, "%s:%d: error: ", file_name, line);
	switch (type)
	{
	case UNRECOGNIZED_TOKEN:
		vfprintf(stderr, "unrecognized token '%s'", args);
		break;
	case UNCLOSED_COMMENT:
		fprintf(stderr, "unterminated comment");
		break;
    case TOKEN_TOO_LONG:
        vfprintf(stderr, "%s longer than 255 characters", args);
        break;
	case UNCLOSED_STRING:
		fprintf(stderr, "mising terminating \" character");
		break;
	case EXTRA_DECIMAL_POINT:
		fprintf(stderr, "too many decimal points in number");
		break;
	case MISSING_TOKEN_FOUND_TOKEN:
		vfprintf(stderr, "expected '%s' before '%s'", args);
		break;
    case MISSING_TOKEN_FOUND_OTHER:
		vfprintf(stderr, "expected '%s' before %s", args);
		break;
    case MISSING_OTHER_FOUND_TOKEN:
        vfprintf(stderr, "expected %s before '%s'", args);
        break;
    case MISSING_OTHER_FOUND_OTHER:
        vfprintf(stderr, "expected %s before %s", args);
        break;
    case DUPLICATE_DECLARATION:
        vfprintf(stderr, "duplicate declaration of symbol '%s'", args);
        break;
    case ILLEGAL_ARRAY_LEN:
        fprintf(stderr, "array length must be a positive integer");
        break;
    case ILLEGAL_ARRAY_INDEX:
        fprintf(stderr, "array index must be a positive integer");
        break;
    case UNDECLARED_SYMBOL:
        vfprintf(stderr, "undeclared symbol '%s'", args);
        break;
    case NONVAR_ASSMT_DEST:
        vfprintf(stderr, "assignment destination '%s' is not a variable", args);
        break;
    case INCOMPATIBLE_TYPE_ASSMT:
        vfprintf(stderr, "value of type %s cannot be assigned to location of type %s", args);
        break;
    case NOT_AN_ARRAY:
        vfprintf(stderr, "subscripted symbol '%s' is not an array", args);
        break;
    case NOT_A_PROC:
        vfprintf(stderr, "called symbol '%s' is not a procedure", args);
        break;
    case MISSING_ARG:
        vfprintf(stderr, "missing argument of type %s in call to procedure '%s'", args);
        break;
    case UNEXPECTED_TOKEN_IN_PROC_CALL:
        vfprintf(stderr, "unexpected token '%s' in call to procedure '%s' (%d arguments expected)", args);
        break;
    case INVALID_ARG_TYPE:
        vfprintf(stderr, "procedure '%s' expects argument of type %s, but argument %d has type %s", args);
        break;
    case INVALID_OPERAND_TYPE:
        vfprintf(stderr, "operator '%s' does not support operand of type %s", args);
        break;
    case INVALID_OPERAND_TYPES:
        vfprintf(stderr, "operator '%s' does not support operands of type %s and %s", args);
        break;
    case NONBOOL_CONDITION:
        fprintf(stderr, "conditional expression must have type BOOL");
        break;
    case OUT_OF_MEMORY:
        fprintf(stderr, "ran out of memory during compilation");
        break;
    }
	printf("\n");
}