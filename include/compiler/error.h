#ifndef ERROR_H
#define ERROR_H

typedef enum {ILLEGAL_CHAR, UNCLOSED_COMMENT, UNCLOSED_STR, EXTRA_DEC_PT} error_type;

void print_error(char* file_name, error_type type, int line, int c, char* quote);

#endif