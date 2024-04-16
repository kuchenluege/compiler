#include <stddef.h>
#include <stdio.h>

#include <llvm-c/Analysis.h>

#include "compiler/code_gen.h"

#define ASSERT_CONTEXT if (!context) {log_code_gen_error("no context"); return;}
#define ASSERT_CONTEXT_RET_NULL if (!context) {log_code_gen_error("no context"); return NULL;}
#define ASSERT_BUILDER if (!builder) {log_code_gen_error("no builder"); return;}
#define ASSERT_BUILDER_RET_NULL if (!builder) {log_code_gen_error("no builder"); return NULL;}
#define ASSERT_MODULE if (!module) {log_code_gen_error("no module"); return;}
#define ASSERT_MODULE_RET_NULL if (!module) {log_code_gen_error("no module"); return NULL;}

static LLVMContextRef context = NULL;
static LLVMBuilderRef builder = NULL;
static LLVMModuleRef module = NULL;

static void log_code_gen_error(char *str) {
    fprintf(stderr, "code_gen error: %s\n", str);
}

static LLVMTypeRef llvm_type(symbol_value_type svt) {
    ASSERT_CONTEXT_RET_NULL
    switch (svt)
    {
    case SVT_INT:
        return LLVMInt32TypeInContext(context);
    case SVT_BOOL:
        return LLVMInt1TypeInContext(context);
    case SVT_FLT:
        return LLVMFloatTypeInContext(context);
    default:
        log_code_gen_error("invalid type");
        return NULL;
    }
}

void code_gen_init() {
    context = LLVMContextCreate();
    builder = LLVMCreateBuilderInContext(context);
}

void code_gen_shutdown() {
    ASSERT_CONTEXT
    ASSERT_BUILDER
    LLVMDisposeBuilder(builder);
    LLVMContextDispose(context);
    LLVMShutdown();
}

void code_gen_module(char *name) {
    ASSERT_CONTEXT
    module = LLVMModuleCreateWithNameInContext(name, context);
}

void code_gen_del_module() {
    ASSERT_MODULE
    LLVMDisposeModule(module);
}

void log_ir() {
    ASSERT_MODULE
    char *ir = LLVMPrintModuleToString(module);
    printf("%s\n", ir);
    LLVMDisposeMessage(ir);
}

LLVMValueRef code_gen_func_proto(token *proc) {
    ASSERT_MODULE_RET_NULL
    LLVMTypeRef func_type = LLVMFunctionType(llvm_type(proc->sym_val_type), NULL, 0, (LLVMBool)0);
    LLVMValueRef func = LLVMAddFunction(module, proc->display_name, func_type);
    if (!func) {
        log_code_gen_error("could not create function prototype");
        return NULL;
    }
    return func;
}

LLVMValueRef code_gen_func(LLVMValueRef func, LLVMValueRef return_value) {
    ASSERT_CONTEXT_RET_NULL

    if (LLVMCountBasicBlocks(func) != 0) {
        log_code_gen_error("function already defined");
        return NULL;
    }

    LLVMBasicBlockRef block = LLVMAppendBasicBlockInContext(context, func, "entry");
    LLVMPositionBuilderAtEnd(builder, block);

    LLVMBuildRet(builder, return_value);

    if (LLVMVerifyFunction(func, LLVMReturnStatusAction) == 1) {
        LLVMDeleteFunction(func);
        log_code_gen_error("invalid function");
        return NULL;
    }
    return func;
}

LLVMValueRef code_gen_convert_type(LLVMValueRef value, symbol_value_type svt) {
    ASSERT_CONTEXT_RET_NULL
    ASSERT_BUILDER_RET_NULL
    LLVMTypeRef type = llvm_type(svt);
    if (LLVMTypeOf(value) == type) {
        return value;
    }
    if (LLVMTypeOf(value) == LLVMInt32TypeInContext(context) && type == LLVMFloatTypeInContext(context)) {
        return LLVMBuildSIToFP(builder, value, type, "sitofptmp");
    }
    if (LLVMTypeOf(value) == LLVMInt1TypeInContext(context) && type == LLVMInt32TypeInContext(context)) {
        return LLVMBuildSExt(builder, value, type, "sexttmp");
    }
    if (LLVMTypeOf(value) == LLVMInt32TypeInContext(context) && type == LLVMInt1TypeInContext(context)) {
        return LLVMBuildTrunc(builder, value, type, "trunctmp");
    }
    log_code_gen_error("invalid type conversion");
    return NULL;
}

LLVMValueRef code_gen_literal(token *literal, int sign) {
    ASSERT_CONTEXT_RET_NULL
    switch (literal->subtype) {
    case T_ST_INT_LIT:
        return LLVMConstInt(LLVMInt32TypeInContext(context), literal->lit_val.int_val * sign, (LLVMBool)1);
    case T_ST_TRUE:
        return LLVMConstInt(LLVMInt1TypeInContext(context), 1, (LLVMBool)0);
    case T_ST_FALSE:
        return LLVMConstInt(LLVMInt1TypeInContext(context), 0, (LLVMBool)0);
    case T_ST_FLOAT_LIT:
        return LLVMConstReal(LLVMFloatTypeInContext(context), (double)literal->lit_val.flt_val * sign);
    case T_ST_STR_LIT:
        return LLVMConstString(literal->lit_val.str_val, MAX_TOKEN_LEN, (LLVMBool)0);
    default:
        return NULL;
    }
}

LLVMValueRef code_gen_un_op(token_subtype op_st, LLVMValueRef value, symbol_value_type svt) {
    ASSERT_BUILDER_RET_NULL
    if (!value) {
        log_code_gen_error("missing value");
        return NULL;
    }

    if (op_st == T_ST_NOT) {
        switch (svt)
        {
        case SVT_INT:
        case SVT_BOOL:
            return LLVMBuildNot(builder, value, "nottmp");
        default:
            log_code_gen_error("invalid type");
        }
    }
    log_code_gen_error("invalid unary operator");
    return NULL;
}

LLVMValueRef code_gen_bin_op(token_subtype op_st, LLVMValueRef lhs, LLVMValueRef rhs, symbol_value_type svt) {
    ASSERT_BUILDER_RET_NULL
    if (!lhs || !rhs) {
        log_code_gen_error("missing value(s)");
        return NULL;
    }

    switch (op_st) {
    case '*':
        switch (svt)
        {
        case SVT_INT:
            return LLVMBuildMul(builder, lhs, rhs, "multmp");
        case SVT_FLT:
            return LLVMBuildFMul(builder, lhs, rhs, "multmp");
        default:
            log_code_gen_error("invalid type");
            return NULL;
        }
    case '/':
        switch (svt)
        {
        case SVT_INT:
            return LLVMBuildSDiv(builder, lhs, rhs, "divtmp");
        case SVT_FLT:
            return LLVMBuildFDiv(builder, lhs, rhs, "divtmp");
        default:
            log_code_gen_error("invalid type");
            return NULL;
        }
    case '+':
        switch (svt)
        {
        case SVT_INT:
            return LLVMBuildAdd(builder, lhs, rhs, "addtmp");
        case SVT_FLT:
            return LLVMBuildFAdd(builder, lhs, rhs, "addtmp");
        default:
            log_code_gen_error("invalid type");
            return NULL;
        }
    case '-':
        switch (svt)
        {
        case SVT_INT:
            return LLVMBuildSub(builder, lhs, rhs, "subtmp");
        case SVT_FLT:
            return LLVMBuildFSub(builder, lhs, rhs, "subtmp");
        default:
            log_code_gen_error("invalid type");
            return NULL;
        }
    case T_ST_LTHAN:
        switch (svt)
        {
        case SVT_INT:
        case SVT_BOOL:
            return LLVMBuildICmp(builder, LLVMIntSLT, lhs, rhs, "slttmp");
        case SVT_FLT:
            return LLVMBuildFCmp(builder, LLVMRealOLT, lhs, rhs, "olttmp");
        default:
            log_code_gen_error("invalid type");
            return NULL;
        }
    case T_ST_LTEQL:
        switch (svt)
        {
        case SVT_INT:
        case SVT_BOOL:
            return LLVMBuildICmp(builder, LLVMIntSLE, lhs, rhs, "sletmp");
        case SVT_FLT:
            return LLVMBuildFCmp(builder, LLVMRealOLE, lhs, rhs, "oletmp");
        default:
            log_code_gen_error("invalid type");
            return NULL;
        }
    case T_ST_GTHAN:
        switch (svt)
        {
        case SVT_INT:
        case SVT_BOOL:
            return LLVMBuildICmp(builder, LLVMIntSGT, lhs, rhs, "sgttmp");
        case SVT_FLT:
            return LLVMBuildFCmp(builder, LLVMRealOGT, lhs, rhs, "ogttmp");
        default:
            log_code_gen_error("invalid type");
            return NULL;
        }
    case T_ST_GTEQL:
        switch (svt)
        {
        case SVT_INT:
        case SVT_BOOL:
            return LLVMBuildICmp(builder, LLVMIntSGE, lhs, rhs, "sgetmp");
        case SVT_FLT:
            return LLVMBuildFCmp(builder, LLVMRealOGE, lhs, rhs, "ogetmp");
        default:
            log_code_gen_error("invalid type");
            return NULL;
        }
    case T_ST_EQLTO:
        switch (svt)
        {
        case SVT_INT:
        case SVT_BOOL:
            return LLVMBuildICmp(builder, LLVMIntEQ, lhs, rhs, "eqtmp");
        case SVT_FLT:
            return LLVMBuildFCmp(builder, LLVMRealOEQ, lhs, rhs, "oeqtmp");
        default:
            log_code_gen_error("invalid type");
            return NULL;
        }
    case T_ST_NOTEQ:
        switch (svt)
        {
        case SVT_INT:
        case SVT_BOOL:
            return LLVMBuildICmp(builder, LLVMIntNE, lhs, rhs, "netmp");
        case SVT_FLT:
            return LLVMBuildFCmp(builder, LLVMRealONE, lhs, rhs, "onetmp");
        default:
            log_code_gen_error("invalid type");
            return NULL;
        }
    case '&':
        switch (svt)
        {
        case SVT_INT:
        case SVT_BOOL:
            return LLVMBuildAnd(builder, lhs, rhs, "andtmp");
        default:
            log_code_gen_error("invalid type");
            return NULL;
        }
    case '|':
        switch (svt)
        {
        case SVT_INT:
        case SVT_BOOL:
            return LLVMBuildOr(builder, lhs, rhs, "ortmp");
        default:
            log_code_gen_error("invalid type");
            return NULL;
        }
    default:
        log_code_gen_error("invalid binary operator");
        return NULL;
    }
}