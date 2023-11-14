/*
 * Copyright (C) 2019 Intel Corporation. All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

#include "aot_emit_stringref.h"
#include "aot.h"
#include "aot_compiler.h"
#include "aot_emit_memory.h"
#include "gc_object.h"
#include "string_object.h"
#include <stdbool.h>

static bool
aot_call_wasm_stringref_obj_new(AOTCompContext *comp_ctx,
                                AOTFuncContext *func_ctx, LLVMValueRef str_obj,
                                uint32 stringref_type, uint32 pos,
                                LLVMValueRef *stringref_obj)
{
    LLVMValueRef param_values[3], func, value, res;
    LLVMTypeRef param_types[3], ret_type, func_type, func_ptr_type;
    uint32 argc = 2;

    param_types[0] = INT8_PTR_TYPE;
    param_types[1] = INT8_PTR_TYPE;
    param_types[2] = I32_TYPE;
    ret_type = INT8_PTR_TYPE;

    if (stringref_type == WASM_TYPE_STRINGREF) {
        GET_AOT_FUNCTION(wasm_stringref_obj_new, argc);
    }
    else if (stringref_type == WASM_TYPE_STRINGVIEWWTF8) {
        GET_AOT_FUNCTION(wasm_stringview_wtf8_obj_new, argc);
    }
    else if (stringref_type == WASM_TYPE_STRINGVIEWWTF16) {
        GET_AOT_FUNCTION(wasm_stringview_wtf16_obj_new, argc);
    }
    else {
        argc = 3;
        GET_AOT_FUNCTION(wasm_stringview_iter_obj_new, argc);
    }

    param_values[0] = func_ctx->exec_env;
    param_values[1] = str_obj;
    if (stringref_type == WASM_TYPE_STRINGVIEWITER) {
        param_values[2] = I32_CONST(pos);
    }

    if (!(res = LLVMBuildCall2(comp_ctx->builder, func_type, func, param_values,
                               argc, "create_stringref"))) {
        aot_set_last_error("llvm build call failed.");
        goto fail;
    }

    *stringref_obj = res;

    return true;
fail:
    return false;
}

static LLVMValueRef
aot_stringref_obj_get_value(AOTCompContext *comp_ctx, AOTFuncContext *func_ctx,
                            LLVMValueRef stringref_obj)
{
    LLVMValueRef str_obj_ptr, str_obj, host_ptr_offset;

    host_ptr_offset = I32_CONST(offsetof(WASMStringrefObject, str_obj));

    if (!(stringref_obj =
              LLVMBuildBitCast(comp_ctx->builder, stringref_obj, INT8_PTR_TYPE,
                               "stringref_obj_i8p"))) {
        aot_set_last_error("llvm build bitcast failed.");
        goto fail;
    }

    if (!(str_obj_ptr =
              LLVMBuildInBoundsGEP2(comp_ctx->builder, INT8_TYPE, stringref_obj,
                                    &host_ptr_offset, 1, "str_obj_i8p"))) {
        aot_set_last_error("llvm build gep failed.");
        goto fail;
    }

    if (!(str_obj_ptr = LLVMBuildBitCast(comp_ctx->builder, str_obj_ptr,
                                         GC_REF_PTR_TYPE, "str_obj_gcref_p"))) {
        aot_set_last_error("llvm build bitcast failed.");
        goto fail;
    }

    if (!(str_obj = LLVMBuildLoad2(comp_ctx->builder, GC_REF_TYPE, str_obj_ptr,
                                   "str_obj"))) {
        aot_set_last_error("llvm build load failed.");
        goto fail;
    }

    return str_obj;

fail:
    return NULL;
}

static LLVMValueRef
aot_call_wasm_string_measure(AOTCompContext *comp_ctx, AOTFuncContext *func_ctx,
                             LLVMValueRef stringref_obj, uint32 encoding)
{
    LLVMValueRef param_values[3], func, value, str_obj;
    LLVMTypeRef param_types[3], ret_type, func_type, func_ptr_type;

    if (!(str_obj =
              aot_stringref_obj_get_value(comp_ctx, func_ctx, stringref_obj))) {
        goto fail;
    }

    param_types[0] = INT8_PTR_TYPE;
    param_types[1] = I32_TYPE;
    ret_type = I32_TYPE;

    GET_AOT_FUNCTION(wasm_string_measure, 2);

    /* Call function wasm_string_measure() */
    param_values[0] = str_obj;
    param_values[1] = I32_CONST(encoding);

    if (!(value = LLVMBuildCall2(comp_ctx->builder, func_type, func,
                                 param_values, 2, "string_measure"))) {
        aot_set_last_error("llvm build call failed.");
        goto fail;
    }

    return value;
fail:
    return NULL;
}

static LLVMValueRef
aot_call_wasm_string_create_view(AOTCompContext *comp_ctx,
                                 AOTFuncContext *func_ctx,
                                 LLVMValueRef stringref_obj, uint32 encoding)
{
    LLVMValueRef param_values[3], func, value, str_obj;
    LLVMTypeRef param_types[3], ret_type, func_type, func_ptr_type;

    if (!(str_obj =
              aot_stringref_obj_get_value(comp_ctx, func_ctx, stringref_obj))) {
        goto fail;
    }

    param_types[0] = INT8_PTR_TYPE;
    param_types[1] = I32_TYPE;
    ret_type = INT8_PTR_TYPE;

    GET_AOT_FUNCTION(wasm_string_create_view, 2);

    /* Call function wasm_string_create_view() */
    param_values[0] = str_obj;
    param_values[1] = I32_CONST(encoding);

    if (!(value = LLVMBuildCall2(comp_ctx->builder, func_type, func,
                                 param_values, 2, "string_create_view"))) {
        aot_set_last_error("llvm build call failed.");
        goto fail;
    }

    return value;
fail:
    return NULL;
}

static LLVMValueRef
aot_call_wasm_string_advance(AOTCompContext *comp_ctx, AOTFuncContext *func_ctx,
                             LLVMValueRef stringref_obj, LLVMValueRef bytes,
                             LLVMValueRef pos)
{
    LLVMValueRef param_values[4], func, value, str_obj;
    LLVMTypeRef param_types[4], ret_type, func_type, func_ptr_type;

    if (!(str_obj =
              aot_stringref_obj_get_value(comp_ctx, func_ctx, stringref_obj))) {
        goto fail;
    }

    param_types[0] = INT8_PTR_TYPE;
    param_types[1] = I32_TYPE;
    param_types[2] = I32_TYPE;
    param_types[3] = INT32_PTR_TYPE;
    ret_type = INT8_PTR_TYPE;

    GET_AOT_FUNCTION(wasm_string_advance, 4);

    /* Call function wasm_string_advance() */
    param_values[0] = str_obj;
    param_values[1] = pos;
    param_values[2] = bytes;
    param_values[3] = I8_PTR_NULL;

    if (!(value = LLVMBuildCall2(comp_ctx->builder, func_type, func,
                                 param_values, 4, "string_advance"))) {
        aot_set_last_error("llvm build call failed.");
        goto fail;
    }

    return value;
fail:
    return NULL;
}

static LLVMValueRef
aot_call_wasm_string_slice(AOTCompContext *comp_ctx, AOTFuncContext *func_ctx,
                           LLVMValueRef stringref_obj, LLVMValueRef start,
                           LLVMValueRef end, StringViewType stringview_type)
{
    LLVMValueRef param_values[4], func, value, str_obj;
    LLVMTypeRef param_types[4], ret_type, func_type, func_ptr_type;

    if (!(str_obj =
              aot_stringref_obj_get_value(comp_ctx, func_ctx, stringref_obj))) {
        goto fail;
    }

    param_types[0] = INT8_PTR_TYPE;
    param_types[1] = I32_TYPE;
    param_types[2] = I32_TYPE;
    param_types[3] = I32_TYPE;
    ret_type = INT8_PTR_TYPE;

    GET_AOT_FUNCTION(wasm_string_slice, 4);

    /* Call function wasm_string_slice() */
    param_values[0] = str_obj;
    param_values[1] = start;
    param_values[2] = end;
    param_values[3] = I32_CONST(stringview_type);

    if (!(value = LLVMBuildCall2(comp_ctx->builder, func_type, func,
                                 param_values, 4, "string_slice"))) {
        aot_set_last_error("llvm build call failed.");
        goto fail;
    }

    return value;
fail:
    return NULL;
}

bool
aot_compile_op_string_new(AOTCompContext *comp_ctx, AOTFuncContext *func_ctx,
                          uint32 encoding)
{
    LLVMValueRef maddr, byte_length, str_obj, stringref_obj;
    bool enable_segue = comp_ctx->enable_segue_i32_store;
    LLVMValueRef param_values[5], func, value;
    LLVMTypeRef param_types[5], ret_type, func_type, func_ptr_type;

    POP_I32(byte_length);

    /* TODO: check memory overflow based on byte_length, maybe reuse check bulk
     * memory overflow */
    if (!(maddr = aot_check_memory_overflow(comp_ctx, func_ctx, 0, 8,
                                            enable_segue)))
        return false;

    param_types[0] = INT8_PTR_TYPE;
    param_types[1] = I32_TYPE;
    param_types[2] = I32_TYPE;
    ret_type = INT8_PTR_TYPE;

    GET_AOT_FUNCTION(wasm_string_new_with_encoding, 3);

    /* Call function wasm_struct_obj_new() */
    param_values[0] = maddr;
    param_values[1] = byte_length;
    param_values[2] = I32_CONST(encoding);

    if (!(str_obj = LLVMBuildCall2(comp_ctx->builder, func_type, func,
                                   param_values, 3, "wasm_string_new"))) {
        aot_set_last_error("llvm build call failed.");
        goto fail;
    }
    /* TODO: check results */

    if (!aot_call_wasm_stringref_obj_new(comp_ctx, func_ctx, str_obj,
                                         WASM_TYPE_STRINGREF, 0,
                                         &stringref_obj)) {
        goto fail;
    }

    PUSH_GC_REF(stringref_obj);

    return true;
fail:
    return false;
}

bool
aot_compile_op_string_const(AOTCompContext *comp_ctx, AOTFuncContext *func_ctx,
                            uint32 contents)
{
    LLVMValueRef param_values[2], func, value, str_obj, stringref_obj;
    LLVMTypeRef param_types[2], ret_type, func_type, func_ptr_type;

    param_types[0] = INT8_PTR_TYPE;
    ret_type = INT8_PTR_TYPE;

    GET_AOT_FUNCTION(wasm_string_new_const, 1);

    /* TODO: should get string content from module */
    param_values[0] = I32_CONST(contents);

    if (!(str_obj = LLVMBuildCall2(comp_ctx->builder, func_type, func,
                                   param_values, 1, "create_stringref"))) {
        aot_set_last_error("llvm build call failed.");
        goto fail;
    }
    /* TODO: check results */

    if (!aot_call_wasm_stringref_obj_new(comp_ctx, func_ctx, str_obj,
                                         WASM_TYPE_STRINGREF, 0,
                                         &stringref_obj)) {
        goto fail;
    }

    PUSH_GC_REF(stringref_obj);

    return true;
fail:
    return false;
}

bool
aot_compile_op_string_measure(AOTCompContext *comp_ctx,
                              AOTFuncContext *func_ctx, uint32 encoding)
{
    LLVMValueRef stringref_obj, value;

    POP_GC_REF(stringref_obj);

    if (!(value = aot_call_wasm_string_measure(comp_ctx, func_ctx,
                                               stringref_obj, encoding))) {
        goto fail;
    }

    PUSH_I32(value);

    return true;
fail:
    return false;
}

bool
aot_compile_op_string_encode(AOTCompContext *comp_ctx, AOTFuncContext *func_ctx,
                             uint32 mem_idx, uint32 encoding)
{
    LLVMValueRef param_values[6], func, value, length, maddr, str_obj,
        stringref_obj;
    LLVMTypeRef param_types[6], ret_type, func_type, func_ptr_type;
    bool enable_segue = comp_ctx->enable_segue_i32_store;

    /* TODO: check memory overflow */
    if (!(maddr = aot_check_memory_overflow(comp_ctx, func_ctx, 0, 8,
                                            enable_segue)))
        return false;

    POP_GC_REF(stringref_obj);

    if (!(str_obj =
              aot_stringref_obj_get_value(comp_ctx, func_ctx, stringref_obj))) {
        goto fail;
    }

    if (!(length = aot_call_wasm_string_measure(comp_ctx, func_ctx,
                                                stringref_obj, encoding))) {
        goto fail;
    }

    param_types[0] = INT8_PTR_TYPE;
    param_types[1] = I32_TYPE;
    param_types[2] = I32_TYPE;
    param_types[3] = INT8_PTR_TYPE;
    param_types[4] = INT8_PTR_TYPE;
    param_types[5] = I32_TYPE;
    ret_type = I32_TYPE;

    GET_AOT_FUNCTION(wasm_string_encode, 6);

    /* Call function wasm_string_measure() */
    param_values[0] = str_obj;
    param_values[1] = I32_ZERO;
    param_values[2] = length;
    param_values[3] = maddr;
    param_values[4] = I8_PTR_NULL;
    param_values[5] = I32_CONST(encoding);

    if (!(value = LLVMBuildCall2(comp_ctx->builder, func_type, func,
                                 param_values, 6, "string_encode"))) {
        aot_set_last_error("llvm build call failed.");
        goto fail;
    }

    /* TODO: check result and raise exception */
    PUSH_I32(value);

    return true;
fail:
    return false;
}

bool
aot_compile_op_string_concat(AOTCompContext *comp_ctx, AOTFuncContext *func_ctx)
{
    LLVMValueRef param_values[2], func, value, str_obj_lhs, str_obj_rhs,
        stringref_obj_lhs, stringref_obj_rhs, stringref_obj_new;
    LLVMTypeRef param_types[2], ret_type, func_type, func_ptr_type;

    POP_GC_REF(stringref_obj_rhs);
    POP_GC_REF(stringref_obj_lhs);

    if (!(str_obj_lhs = aot_stringref_obj_get_value(comp_ctx, func_ctx,
                                                    stringref_obj_lhs))) {
        goto fail;
    }

    if (!(str_obj_rhs = aot_stringref_obj_get_value(comp_ctx, func_ctx,
                                                    stringref_obj_rhs))) {
        goto fail;
    }

    param_types[0] = INT8_PTR_TYPE;
    param_types[1] = INT8_PTR_TYPE;
    ret_type = INT8_PTR_TYPE;

    GET_AOT_FUNCTION(wasm_string_concat, 2);

    /* Call function wasm_string_concat() */
    param_values[0] = str_obj_lhs;
    param_values[1] = str_obj_rhs;

    if (!(str_obj_lhs = LLVMBuildCall2(comp_ctx->builder, func_type, func,
                                       param_values, 2, "string_concat"))) {
        aot_set_last_error("llvm build call failed.");
        goto fail;
    }

    if (!aot_call_wasm_stringref_obj_new(comp_ctx, func_ctx, str_obj_lhs,
                                         WASM_TYPE_STRINGREF, 0,
                                         &stringref_obj_new)) {
        goto fail;
    }

    PUSH_GC_REF(stringref_obj_new);

    return true;
fail:
    return false;
}

bool
aot_compile_op_string_eq(AOTCompContext *comp_ctx, AOTFuncContext *func_ctx)
{
    LLVMValueRef param_values[2], func, value, str_obj_lhs, str_obj_rhs,
        stringref_obj_lhs, stringref_obj_rhs;
    LLVMTypeRef param_types[2], ret_type, func_type, func_ptr_type;

    POP_GC_REF(stringref_obj_lhs);
    POP_GC_REF(stringref_obj_rhs);

    if (!(str_obj_lhs = aot_stringref_obj_get_value(comp_ctx, func_ctx,
                                                    stringref_obj_lhs))) {
        goto fail;
    }

    if (!(str_obj_rhs = aot_stringref_obj_get_value(comp_ctx, func_ctx,
                                                    stringref_obj_rhs))) {
        goto fail;
    }

    param_types[0] = INT8_PTR_TYPE;
    param_types[1] = INT8_PTR_TYPE;
    ret_type = I32_TYPE;

    GET_AOT_FUNCTION(wasm_string_eq, 2);

    /* Call function wasm_string_eq() */
    param_values[0] = str_obj_lhs;
    param_values[1] = str_obj_rhs;

    if (!(value = LLVMBuildCall2(comp_ctx->builder, func_type, func,
                                 param_values, 2, "string_eq"))) {
        aot_set_last_error("llvm build call failed.");
        goto fail;
    }

    PUSH_I32(value);

    return true;
fail:
    return false;
}

bool
aot_compile_op_string_is_usv_sequence(AOTCompContext *comp_ctx,
                                      AOTFuncContext *func_ctx)
{
    LLVMValueRef param_values[1], func, value, str_obj, stringref_obj;
    LLVMTypeRef param_types[1], ret_type, func_type, func_ptr_type;

    POP_GC_REF(stringref_obj);

    if (!(str_obj =
              aot_stringref_obj_get_value(comp_ctx, func_ctx, stringref_obj))) {
        goto fail;
    }

    param_types[0] = INT8_PTR_TYPE;
    ret_type = I32_TYPE;

    GET_AOT_FUNCTION(wasm_string_is_usv_sequence, 1);

    /* Call function wasm_string_is_usv_sequence() */
    param_values[0] = str_obj;

    if (!(value = LLVMBuildCall2(comp_ctx->builder, func_type, func,
                                 param_values, 1, "string_is_usv_sequence"))) {
        aot_set_last_error("llvm build call failed.");
        goto fail;
    }

    PUSH_I32(value);

    return true;
fail:
    return false;
}

bool
aot_compile_op_string_as_wtf8(AOTCompContext *comp_ctx,
                              AOTFuncContext *func_ctx)
{
    LLVMValueRef str_obj, stringref_obj, stringview_wtf8_obj;

    POP_GC_REF(stringref_obj);

    if (!(str_obj = aot_call_wasm_string_create_view(
              comp_ctx, func_ctx, stringref_obj, STRING_VIEW_WTF8))) {
        goto fail;
    }

    if (!aot_call_wasm_stringref_obj_new(comp_ctx, func_ctx, str_obj,
                                         WASM_TYPE_STRINGVIEWWTF8, 0,
                                         &stringview_wtf8_obj)) {
        goto fail;
    }

    PUSH_GC_REF(stringview_wtf8_obj);

    return true;
fail:
    return false;
}

bool
aot_compile_op_stringview_wtf8_advance(AOTCompContext *comp_ctx,
                                       AOTFuncContext *func_ctx)
{
    LLVMValueRef stringref_obj, bytes, pos, value;

    POP_I32(bytes);
    POP_I32(pos);
    POP_GC_REF(stringref_obj);

    if (!(value = aot_call_wasm_string_advance(comp_ctx, func_ctx,
                                               stringref_obj, bytes, pos))) {
        goto fail;
    }

    PUSH_I32(value);

    return true;
fail:
    return false;
}

bool
aot_compile_op_stringview_wtf8_encode(AOTCompContext *comp_ctx,
                                      AOTFuncContext *func_ctx, uint32 mem_idx,
                                      uint32 encoding)
{
    LLVMValueRef param_values[6], func, value, maddr, str_obj, stringref_obj;
    LLVMValueRef bytes, pos, next_pos;
    LLVMTypeRef param_types[6], ret_type, func_type, func_ptr_type;
    bool enable_segue = comp_ctx->enable_segue_i32_store;

    POP_I32(bytes);
    POP_I32(pos);

    next_pos = LLVMBuildAlloca(comp_ctx->builder, I32_TYPE, "next_pos");
    if (!next_pos) {
        aot_set_last_error("failed to build alloca");
        goto fail;
    }

    /* TODO: check memory overflow */
    if (!(maddr = aot_check_memory_overflow(comp_ctx, func_ctx, 0, 8,
                                            enable_segue)))
        goto fail;

    POP_GC_REF(stringref_obj);

    if (!(str_obj =
              aot_stringref_obj_get_value(comp_ctx, func_ctx, stringref_obj))) {
        goto fail;
    }

    param_types[0] = INT8_PTR_TYPE;
    param_types[1] = I32_TYPE;
    param_types[2] = I32_TYPE;
    param_types[3] = INT8_PTR_TYPE;
    param_types[4] = INT8_PTR_TYPE;
    param_types[5] = I32_TYPE;
    ret_type = I32_TYPE;

    GET_AOT_FUNCTION(wasm_string_encode, 6);

    /* Call function wasm_string_measure() */
    param_values[0] = str_obj;
    param_values[1] = pos;
    param_values[2] = bytes;
    param_values[3] = maddr;
    param_values[4] = next_pos;
    param_values[5] = I32_CONST(encoding);

    if (!(value = LLVMBuildCall2(comp_ctx->builder, func_type, func,
                                 param_values, 6, "string_encode"))) {
        aot_set_last_error("llvm build call failed.");
        goto fail;
    }
    /* TODO: check result and raise exception */

    next_pos =
        LLVMBuildLoad2(comp_ctx->builder, I32_TYPE, next_pos, "next_pos");
    if (!next_pos) {
        aot_set_last_error("llvm build load failed.");
        goto fail;
    }

    PUSH_I32(next_pos);
    PUSH_I32(value);

    return true;
fail:
    return false;
}

bool
aot_compile_op_stringview_wtf8_slice(AOTCompContext *comp_ctx,
                                     AOTFuncContext *func_ctx)
{
    LLVMValueRef stringref_obj, start, end, stringref_obj_new, value;

    POP_I32(start);
    POP_I32(end);
    POP_GC_REF(stringref_obj);

    if (!(value = aot_call_wasm_string_slice(comp_ctx, func_ctx, stringref_obj,
                                             start, end, STRING_VIEW_WTF8))) {
        goto fail;
    }

    if (!aot_call_wasm_stringref_obj_new(comp_ctx, func_ctx, value,
                                         WASM_TYPE_STRINGVIEWWTF8, 0,
                                         &stringref_obj_new)) {
        goto fail;
    }

    PUSH_GC_REF(stringref_obj_new);

    return true;
fail:
    return false;
}

bool
aot_compile_op_string_as_wtf16(AOTCompContext *comp_ctx,
                               AOTFuncContext *func_ctx)
{
    LLVMValueRef str_obj, stringref_obj, stringview_wtf16_obj;

    POP_GC_REF(stringref_obj);

    if (!(str_obj = aot_call_wasm_string_create_view(
              comp_ctx, func_ctx, stringref_obj, STRING_VIEW_WTF16))) {
        goto fail;
    }

    if (!aot_call_wasm_stringref_obj_new(comp_ctx, func_ctx, str_obj,
                                         WASM_TYPE_STRINGVIEWWTF16, 0,
                                         &stringview_wtf16_obj)) {
        goto fail;
    }

    PUSH_GC_REF(stringview_wtf16_obj);

    return true;
fail:
    return false;
}

bool
aot_compile_op_stringview_wtf16_length(AOTCompContext *comp_ctx,
                                       AOTFuncContext *func_ctx)
{
    LLVMValueRef param_values[2], func, value, str_obj, stringview_wtf16_obj;
    LLVMTypeRef param_types[2], ret_type, func_type, func_ptr_type;

    POP_GC_REF(stringview_wtf16_obj);

    if (!(str_obj = aot_stringref_obj_get_value(comp_ctx, func_ctx,
                                                stringview_wtf16_obj))) {
        goto fail;
    }

    param_types[0] = INT8_PTR_TYPE;
    ret_type = I32_TYPE;

    GET_AOT_FUNCTION(wasm_string_wtf16_get_length, 6);

    /* Call function wasm_string_wtf16_get_length() */
    param_values[0] = str_obj;

    if (!(value = LLVMBuildCall2(comp_ctx->builder, func_type, func,
                                 param_values, 1, "stringview_wtf16_length"))) {
        aot_set_last_error("llvm build call failed.");
        goto fail;
    }

    PUSH_I32(value);

    return true;
fail:
    return false;
}

bool
aot_compile_op_stringview_wtf16_get_codeunit(AOTCompContext *comp_ctx,
                                             AOTFuncContext *func_ctx)
{
    LLVMValueRef param_values[2], func, value, str_obj, stringview_wtf16_obj,
        pos;
    LLVMTypeRef param_types[2], ret_type, func_type, func_ptr_type;

    POP_I32(pos);
    POP_GC_REF(stringview_wtf16_obj);

    if (!(str_obj = aot_stringref_obj_get_value(comp_ctx, func_ctx,
                                                stringview_wtf16_obj))) {
        goto fail;
    }

    param_types[0] = INT8_PTR_TYPE;
    param_types[1] = I32_TYPE;
    ret_type = I32_TYPE;

    GET_AOT_FUNCTION(wasm_string_get_wtf16_codeunit, 2);

    /* Call function wasm_string_get_wtf16_codeunit() */
    param_values[0] = str_obj;
    param_values[1] = pos;

    if (!(value =
              LLVMBuildCall2(comp_ctx->builder, func_type, func, param_values,
                             2, "stringview_wtf16_get_codeunit"))) {
        aot_set_last_error("llvm build call failed.");
        goto fail;
    }

    PUSH_I32(value);

    return true;
fail:
    return false;
}

bool
aot_compile_op_stringview_wtf16_encode(AOTCompContext *comp_ctx,
                                       AOTFuncContext *func_ctx, uint32 mem_idx)
{
    LLVMValueRef param_values[6], func, value, maddr, str_obj, stringref_obj;
    LLVMValueRef len, pos;
    LLVMTypeRef param_types[6], ret_type, func_type, func_ptr_type;
    bool enable_segue = comp_ctx->enable_segue_i32_store;

    POP_I32(len);
    POP_I32(pos);

    /* TODO: check memory overflow */
    if (!(maddr = aot_check_memory_overflow(comp_ctx, func_ctx, 0, 8,
                                            enable_segue)))
        return false;

    POP_GC_REF(stringref_obj);

    /* TODO: check alignment */

    if (!(str_obj =
              aot_stringref_obj_get_value(comp_ctx, func_ctx, stringref_obj))) {
        goto fail;
    }

    param_types[0] = INT8_PTR_TYPE;
    param_types[1] = I32_TYPE;
    param_types[2] = I32_TYPE;
    param_types[3] = INT8_PTR_TYPE;
    param_types[4] = INT8_PTR_TYPE;
    param_types[5] = I32_TYPE;
    ret_type = I32_TYPE;

    GET_AOT_FUNCTION(wasm_string_encode, 6);

    /* Call function wasm_string_measure() */
    param_values[0] = str_obj;
    param_values[1] = pos;
    param_values[2] = len;
    param_values[3] = maddr;
    param_values[4] = I8_PTR_NULL;
    param_values[5] = I32_CONST(WTF16);

    if (!(value = LLVMBuildCall2(comp_ctx->builder, func_type, func,
                                 param_values, 6, "string_encode"))) {
        aot_set_last_error("llvm build call failed.");
        goto fail;
    }

    return true;
fail:
    return false;
}

bool
aot_compile_op_stringview_wtf16_slice(AOTCompContext *comp_ctx,
                                      AOTFuncContext *func_ctx)
{
    LLVMValueRef stringref_obj, start, end, stringref_obj_new, value;

    POP_I32(end);
    POP_I32(start);
    POP_GC_REF(stringref_obj);

    if (!(value = aot_call_wasm_string_slice(comp_ctx, func_ctx, stringref_obj,
                                             start, end, STRING_VIEW_WTF16))) {
        goto fail;
    }

    if (!aot_call_wasm_stringref_obj_new(comp_ctx, func_ctx, value,
                                         WASM_TYPE_STRINGVIEWWTF16, 0,
                                         &stringref_obj_new)) {
        goto fail;
    }

    PUSH_GC_REF(stringref_obj_new);

    return true;
fail:
    return false;
}

bool
aot_compile_op_string_as_iter(AOTCompContext *comp_ctx,
                              AOTFuncContext *func_ctx)
{
    LLVMValueRef stringref_obj, stringview_iter_obj;

    POP_GC_REF(stringref_obj);

    if (!aot_call_wasm_stringref_obj_new(comp_ctx, func_ctx, stringref_obj,
                                         WASM_TYPE_STRINGVIEWITER, 0,
                                         &stringview_iter_obj)) {
        goto fail;
    }

    PUSH_GC_REF(stringview_iter_obj);

    return true;
fail:
    return false;
}
