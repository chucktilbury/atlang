/**
    @file object.h

    @brief

**/
#ifndef __OBJECT_H__
#define __OBJECT_H__

#include "common.h"

typedef enum {
    OBJ_STRING,
} ObjectType;

struct Obj {
    ObjectType type;
};

typedef char_buffer_t String;

struct ObjString {
    Obj obj;
    int len;
    String str;
};

static inline bool __attribute__((always_inline)) value_is_string(Value* val) {
    if(value_is_object(val)) {
        if(val->as.obj->type == OBJ_STRING)
            return true;
    }
    return false;
}

static inline ObjString* __attribute__((always_inline)) value_as_string(Value* val) {
    if(value_is_object(val)) {
        if(val->as.obj->type == OBJ_STRING)
            return (ObjString*)val->as.obj;
    }
    return NULL;
}

static inline char* __attribute__((always_inline)) value_as_cstring(Value* val) {
    if(value_is_object(val)) {
        if(val->as.obj->type == OBJ_STRING)
            return (char*)get_char_buffer(((ObjString*)val->as.obj)->str);
    }
    return NULL;
}

Obj* create_string_object(const char* str);
void free_object(Obj*);
bool compare_objects(Value* op1, Value* op2, OpCode op);
Obj* arithmetic_objects(Value* op1, Value* op2, OpCode op);
ValueType conv_obj_to_val(Value*, ValueType);
ValueType conv_val_to_obj(Value*, ObjectType);

#endif
