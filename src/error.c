#include <stdio.h>
#include "compiler/error.h"

void print_error(char* file_name, error_type type, int line, int c, char* quote) {
	printf("%s:%d:%d: ", file_name, line, c);
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