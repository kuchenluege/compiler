#include <llvm-c/Core.h>

#include "compiler/token.h"

extern LLVMContextRef context;
extern LLVMBuilderRef builder;
extern LLVMModuleRef module;

LLVMModuleRef code_gen_module(char *name, LLVMValueRef expr_value);
LLVMValueRef code_gen_literal(token *literal);
LLVMValueRef code_gen_bin_op(token_subtype op_st, LLVMValueRef lhs, LLVMValueRef rhs, symbol_value_type type);