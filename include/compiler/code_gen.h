#include <llvm-c/Core.h>

#include "compiler/token.h"

void code_gen_init();
void code_gen_shutdown();
void code_gen_module(char *name);
void code_gen_del_module();
void log_ir();
LLVMValueRef code_gen_func_proto(token *proc);
LLVMValueRef code_gen_func(LLVMValueRef func, LLVMValueRef return_value);
LLVMValueRef code_gen_convert_type(LLVMValueRef value, symbol_value_type svt);
LLVMValueRef code_gen_literal(token *literal, int sign);
LLVMValueRef code_gen_un_op(token_subtype op_st, LLVMValueRef value, symbol_value_type svt);
LLVMValueRef code_gen_bin_op(token_subtype op_st, LLVMValueRef lhs, LLVMValueRef rhs, symbol_value_type svt);