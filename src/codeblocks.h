/**
    @file codeblocks.h

    @brief

**/
#ifndef __CODEBLOCKS_H__
#define __CODEBLOCKS_H__

#include "common.h"

typedef enum {
    VAL_INVALID,
    VAL_INUM,
    VAL_UNUM,
    VAL_FNUM,
    VAL_BOOL,
    VAL_NOTHING,
    VAL_OBJ,
} ValueType;

#define create_value_list      (ValueArray*)create_ptr_list
#define free_value_list(b)     destroy_ptr_list((b)->constants)
#define write_value_list(b, v) append_ptr_list((b)->constants, (v))
#define value_list_size(b)     ((b)->constants->nitems)
#define raw_value_list(b)      ((Value**)((b)->constants->buffer))
typedef ptr_list_t ValueArray;

typedef struct Obj Obj;
typedef struct ObjString ObjString;

typedef struct {
    ValueType type;
    union {
        uint64_t unum;
        int64_t inum;
        double fnum;
        bool bval;
        Obj* obj;
    } as;
} Value;

#define create_code_list        (codeArray*)create_u16_list
#define free_code_list(b)       destroy_u16_list((b)->code)
#define write_code_list(b, v)   append_u16_list((b)->code, v)
#define code_list_size(b)       ((b)->code->nitems)
#define raw_code_list(b)        ((b)->code->buffer)
typedef u16_list_t codeArray;

typedef enum {
    OP_CONSTANT,
    OP_ADD,
    OP_SUB,
    OP_MUL,
    OP_DIV,
    OP_MOD,
    OP_NEG,
    OP_NOTHING,
    OP_TRUE,
    OP_FALSE,
    OP_NOT,
    OP_EQUALITY,
    OP_NEQ,
    OP_LT,
    OP_GT,
    OP_LTE,
    OP_GTE,
    OP_RETURN,
} OpCode;

typedef struct {
    codeArray* code;
    ValueArray* constants;
} codeBlock;

codeBlock* create_codeblock();
void free_codeblock(codeBlock*);

void emit_opcode(uint16_t);
size_t emit_fnum_value(double);
size_t emit_unum_value(uint64_t);
size_t emit_inum_value(int64_t);
size_t emit_obj_value(Obj*);
size_t add_constant(Value*);

Value* create_value(ValueType);
void free_value(Value*);
void print_value(const Value*);
// bool value_is_number(Value*);
// bool value_is_bool(Value*);
// bool value_is_void(Value*);

static inline bool __attribute__((always_inline)) value_is_number(Value* val) {
    return (val->type == VAL_FNUM ||
        val->type == VAL_INUM ||
        val->type == VAL_UNUM);
}

static inline bool __attribute__((always_inline)) value_is_bool(Value* val) {
    return val->type == VAL_BOOL;
}

static inline bool __attribute__((always_inline)) value_is_nothing(Value* val) {
    return val->type == VAL_NOTHING;
}

static inline bool __attribute__((always_inline)) value_is_object(Value* val) {
    return val->type == VAL_OBJ;
}

#endif
