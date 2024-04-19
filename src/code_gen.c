#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <llvm-c/Analysis.h>

#include "compiler/code_gen.h"

#define ASSERT_CONTEXT if (!context) {log_code_gen_error("no context"); return;}
#define ASSERT_CONTEXT_RET_NULL if (!context) {log_code_gen_error("no context"); return NULL;}
#define ASSERT_CONTEXT_RET_EMPTY_DATA if (!context) {log_code_gen_error("no context"); return (if_stmt_data){NULL};}
#define ASSERT_BUILDER if (!builder) {log_code_gen_error("no builder"); return;}
#define ASSERT_BUILDER_RET_NULL if (!builder) {log_code_gen_error("no builder"); return NULL;}
#define ASSERT_BUILDER_RET_EMPTY_DATA if (!builder) {log_code_gen_error("no builder"); return (if_stmt_data){NULL};}
#define ASSERT_MODULE if (!module) {log_code_gen_error("no module"); return;}
#define ASSERT_MODULE_RET_NULL if (!module) {log_code_gen_error("no module"); return NULL;}
#define ASSERT_MODULE_RET_EMPTY_DATA if (!module) {log_code_gen_error("no module"); return (if_stmt_data){NULL};}

static LLVMContextRef context = NULL;
static LLVMBuilderRef builder = NULL;
static LLVMModuleRef module = NULL;

static void log_code_gen_error(char *str) {
    fprintf(stderr, "code_gen error: %s\n", str);
}

static LLVMTypeRef llvm_type(symbol_value_type svt, int size) {
    ASSERT_CONTEXT_RET_NULL
    switch (svt)
    {
    case SVT_INT:
        return LLVMInt32TypeInContext(context);
    case SVT_INT_ARR:
        return LLVMArrayType(LLVMInt32TypeInContext(context), size);
    case SVT_BOOL:
        return LLVMInt1TypeInContext(context);
    case SVT_BOOL_ARR:
        return LLVMArrayType(LLVMInt1TypeInContext(context), size);
    case SVT_FLT:
        return LLVMFloatTypeInContext(context);
    case SVT_FLT_ARR:
        return LLVMArrayType(LLVMFloatTypeInContext(context), size);
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

LLVMValueRef code_gen_alloca(token *var) {
    ASSERT_BUILDER_RET_NULL
    if (is_array_type(var->sym_val_type)) {
        LLVMValueRef size = LLVMConstInt(LLVMInt32TypeInContext(context), var->sym_len, (LLVMBool)1);
        return LLVMBuildArrayAlloca(builder, llvm_type(var->sym_val_type, var->sym_len), size, var->display_name);
    }
    return LLVMBuildAlloca(builder, llvm_type(var->sym_val_type, var->sym_len), var->display_name);
}

LLVMValueRef code_gen_store(LLVMValueRef value, LLVMValueRef alloca_ptr) {
    ASSERT_BUILDER_RET_NULL
    if (!LLVMIsAAllocaInst(alloca_ptr)) {
        log_code_gen_error("invalid alloca for store");
    }
    return LLVMBuildStore(builder, value, alloca_ptr);
}

LLVMValueRef code_gen_load(token *var) {
    ASSERT_BUILDER_RET_NULL
    if (!LLVMIsAAllocaInst(var->alloca_ptr)) {
        log_code_gen_error("invalid alloca for load");
    }
    return LLVMBuildLoad2(builder, llvm_type(var->sym_val_type, var->sym_len), var->alloca_ptr, var->display_name);
}

LLVMValueRef code_gen_func(token *proc) {
    ASSERT_CONTEXT_RET_NULL
    ASSERT_BUILDER_RET_NULL
    ASSERT_MODULE_RET_NULL

    int param_count = proc->proc_param_count;
    LLVMTypeRef *param_types = NULL;
    if (param_count > 0) {
        param_types = malloc(param_count * sizeof(LLVMTypeRef));
        for (unsigned i = 0; i < param_count; i++) {
            param_types[i] = llvm_type(proc->proc_params[i]->sym_val_type, proc->sym_len);
        }
    }
    LLVMTypeRef func_type = LLVMFunctionType(llvm_type(proc->sym_val_type, proc->sym_len), param_types, param_count, (LLVMBool)0);
    LLVMValueRef func = LLVMAddFunction(module, proc->display_name, func_type);
    if (!func) {
        log_code_gen_error("could not create function prototype");
        return NULL;
    }
    if (LLVMCountBasicBlocks(func) != 0) {
        log_code_gen_error("function already defined");
        return NULL;
    }
    LLVMBasicBlockRef block = LLVMAppendBasicBlockInContext(context, func, "entry");
    LLVMPositionBuilderAtEnd(builder, block);

    LLVMValueRef func_param = LLVMGetFirstParam(func);
    for (unsigned i = 0; i < param_count; i++) {
        token *proc_param = proc->proc_params[i];
        LLVMSetValueName2(func_param, proc_param->display_name, strlen(proc_param->display_name));
        proc_param->alloca_ptr = code_gen_alloca(proc_param);
        code_gen_store(func_param, proc_param->alloca_ptr);
        func_param = LLVMGetNextParam(func_param);
    }

    return func;
}

LLVMValueRef code_gen_verify_func(LLVMValueRef func) {
    if (LLVMVerifyFunction(func, LLVMPrintMessageAction) == 1) {
        //LLVMDeleteFunction(func);
        return NULL;
    }
    return func;
}

LLVMValueRef code_gen_ret_stmt(LLVMValueRef value) {
    ASSERT_BUILDER_RET_NULL
    return LLVMBuildRet(builder, value);
}

if_stmt_data code_gen_if_stmt_if(LLVMValueRef if_value) {
    ASSERT_CONTEXT_RET_EMPTY_DATA
    ASSERT_BUILDER_RET_EMPTY_DATA
    if (LLVMTypeOf(if_value) != LLVMInt1TypeInContext(context)) {
        log_code_gen_error("conditional value must have type i1");
        return (if_stmt_data){};
    }
    LLVMValueRef func = LLVMGetBasicBlockParent(LLVMGetInsertBlock(builder));
    LLVMBasicBlockRef then_block = LLVMAppendBasicBlockInContext(context, func, "then");
    LLVMBasicBlockRef else_block = LLVMCreateBasicBlockInContext(context, "else");
    LLVMBasicBlockRef merge_block = LLVMCreateBasicBlockInContext(context, "merge");
    LLVMValueRef cond_branch = LLVMBuildCondBr(builder, if_value, then_block, else_block);

    LLVMPositionBuilderAtEnd(builder, then_block);
    return (if_stmt_data){func, then_block, else_block, merge_block};
}

void code_gen_if_stmt_then(if_stmt_data *data) {
    ASSERT_BUILDER
    LLVMBuildBr(builder, data->merge_block);
    data->then_block = LLVMGetInsertBlock(builder);
    LLVMAppendExistingBasicBlock(data->func, data->else_block);
    LLVMPositionBuilderAtEnd(builder, data->else_block);
}

LLVMValueRef code_gen_if_stmt_merge(LLVMValueRef then_value, LLVMValueRef else_value, if_stmt_data *data) {
    ASSERT_BUILDER_RET_NULL
    LLVMBuildBr(builder, data->merge_block);
    data->else_block = LLVMGetInsertBlock(builder);
    LLVMAppendExistingBasicBlock(data->func, data->merge_block);
    LLVMPositionBuilderAtEnd(builder, data->merge_block);

    // Fix phi crap
    LLVMValueRef phi_node = LLVMBuildPhi(builder, LLVMTypeOf(then_value), "iftmp");
    LLVMValueRef values[] = {then_value, else_value};
    LLVMBasicBlockRef blocks[] = {data->then_block, data->else_block};
    LLVMAddIncoming(phi_node, values, blocks, 2);
    return LLVMBuildRet(builder, phi_node);
}

LLVMValueRef code_gen_convert_type(LLVMValueRef value, symbol_value_type svt) {
    ASSERT_CONTEXT_RET_NULL
    ASSERT_BUILDER_RET_NULL
    LLVMTypeRef type = llvm_type(svt, 0);
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