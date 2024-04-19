#include <llvm-c/Core.h>

#include "compiler/token.h"

typedef struct if_stmt_data {
    LLVMValueRef func;
    LLVMBasicBlockRef then_block;
    LLVMBasicBlockRef else_block;
    LLVMBasicBlockRef merge_block;
} if_stmt_data;

void code_gen_init();
void code_gen_shutdown();
void code_gen_module(char *name);
void code_gen_del_module();
void log_ir();
LLVMValueRef code_gen_alloca(token *var);
LLVMValueRef code_gen_store(LLVMValueRef value, LLVMValueRef alloca_ptr);
LLVMValueRef code_gen_load(token *var);
LLVMValueRef code_gen_func(token *proc);
LLVMValueRef code_gen_verify_func(LLVMValueRef func);
LLVMValueRef code_gen_ret_stmt(LLVMValueRef value);
if_stmt_data code_gen_if_stmt_if(LLVMValueRef if_value);
void code_gen_if_stmt_then(if_stmt_data *data);
LLVMValueRef code_gen_if_stmt_merge(LLVMValueRef then_value, LLVMValueRef else_value, if_stmt_data *data);
LLVMValueRef code_gen_convert_type(LLVMValueRef value, symbol_value_type svt);
LLVMValueRef code_gen_literal(token *literal, int sign);
LLVMValueRef code_gen_un_op(token_subtype op_st, LLVMValueRef value, symbol_value_type svt);
LLVMValueRef code_gen_bin_op(token_subtype op_st, LLVMValueRef lhs, LLVMValueRef rhs, symbol_value_type svt);