#include <stddef.h>
#include <stdio.h>

#include <llvm-c/Analysis.h>

#include "compiler/code_gen.h"

LLVMContextRef context = NULL;
LLVMBuilderRef builder = NULL;
LLVMModuleRef module = NULL;

static LLVMValueRef log_code_gen_error(char *str) {
    fprintf(stderr, "code_gen error: %s\n", str);
    return NULL;
}

LLVMModuleRef code_gen_module(char *name, LLVMValueRef expr_value) {
    LLVMModuleRef module = LLVMModuleCreateWithName(name);

    LLVMTypeRef ret_type = LLVMFunctionType(LLVMTypeOf(expr_value), NULL, 0, 0);
    LLVMValueRef function = LLVMAddFunction(module, "function", ret_type);

    LLVMBasicBlockRef entry = LLVMAppendBasicBlock(function, "entry");
    LLVMPositionBuilderAtEnd(builder, entry);
    LLVMBuildRet(builder, expr_value);

    char *error = NULL;
    LLVMVerifyModule(module, LLVMAbortProcessAction, &error);
    LLVMDisposeMessage(error);

    return module;
}

LLVMValueRef code_gen_literal(token *literal) {
    switch (literal->subtype) {
    case T_ST_INT_LIT:
        return LLVMConstInt(LLVMInt32Type(), literal->lit_val.int_val, (LLVMBool)1);
    case T_ST_TRUE:
        return LLVMConstInt(LLVMInt1Type(), 1, (LLVMBool)0);
    case T_ST_FALSE:
        return LLVMConstInt(LLVMInt1Type(), 0, (LLVMBool)0);
    case T_ST_FLOAT_LIT:
        return LLVMConstReal(LLVMFloatType(), (double)literal->lit_val.flt_val);
    case T_ST_STR_LIT:
        return LLVMConstStringInContext(context, literal->lit_val.str_val, MAX_TOKEN_LEN, (LLVMBool)1);
    default:
        return NULL;
    }
}

LLVMValueRef code_gen_bin_op(token_subtype op_st, LLVMValueRef lhs, LLVMValueRef rhs, symbol_value_type type) {
    if (!lhs || !rhs) {
        return NULL;
    }

    switch (op_st) {
    case '*':
        switch (type)
        {
        case SVT_INT:
            return LLVMBuildMul(builder, lhs, rhs, "multmp");
        case SVT_FLT:
            return LLVMBuildFMul(builder, lhs, rhs, "multmp");
        default:
            return log_code_gen_error("invalid type");
        }
    case '/':
        switch (type)
        {
        case SVT_INT:
            return LLVMBuildSDiv(builder, lhs, rhs, "divtmp");
        case SVT_FLT:
            return LLVMBuildFDiv(builder, lhs, rhs, "divtmp");
        default:
            return log_code_gen_error("invalid type");
        }
    case '+':
        switch (type)
        {
        case SVT_INT:
            return LLVMBuildAdd(builder, lhs, rhs, "addtmp");
        case SVT_FLT:
            return LLVMBuildFAdd(builder, lhs, rhs, "addtmp");
        default:
            return log_code_gen_error("invalid type");
        }
    case '-':
        switch (type)
        {
        case SVT_INT:
            return LLVMBuildSub(builder, lhs, rhs, "subtmp");
        case SVT_FLT:
            return LLVMBuildFSub(builder, lhs, rhs, "subtmp");
        default:
            return log_code_gen_error("invalid type");
        }
    default:
        return log_code_gen_error("invalid binary operator");
    }
}