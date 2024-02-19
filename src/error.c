#include <stdarg.h>
#include <stdio.h>
#include "compiler/error.h"

void print_error(char* file_name, error_type type, int line, ...) {
	va_list args;
    va_start(args, line);

	printf("%s:%d: ", file_name, line);
	switch (type)
	{
	case UNREC_TOKEN:
		printf("error: unrecognized token '%s'", args);
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
	case EXPECTED_TOKEN:
		printf("error: at token '%s', expecting token '%s'", args);
	}
	printf("\n");
}