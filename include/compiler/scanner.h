#ifndef SCANNER_H
#define SCANNER_H

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>

#include "compiler/symbol_table_chain.h"
#include "compiler/error.h"

extern char *file_name;
extern int line_num;
extern token *tok;
extern stc *symbol_tables;

void init_res_words();
void unscan(token *t);
void scan(FILE *file);

#endif