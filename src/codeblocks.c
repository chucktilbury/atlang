/**
    @file codeblocks.c

    @brief Manage blocks of code for the VM.

**/
#include "common.h"

extern VMachine* vm;

codeBlock* create_codeblock() {

    codeBlock* cb = ALLOC_DS(codeBlock);
    cb->code = create_code_list();
    cb->constants = create_value_list();
    return cb;
}

void emit_opcode(uint16_t word) {

    write_code_list(vm->block, word);
}

void free_codeblock(codeBlock* block) {

    log_debug("enter");
    ASSERT(block != NULL, "Null pointer cannot be free()d");

    Value** vlist = raw_value_list(block);
    int vsize = (int)value_list_size(block);
    log_debug("value stack size = %d", vsize);
    for(int i = 0; i < vsize; i++)
        FREE(vlist[i]);

    free_value_list(block);
    //printf("code size = %d\n", (int)code_list_size(block->code));
    free_code_list(block);

    FREE(block);
    log_debug("leave");
}

Value* create_value(ValueType type) {

    Value* val = ALLOC_DS(Value);
    val->type = type;
    return val;
}

void free_value(Value* val) {

    FREE(val);
}

size_t emit_fnum_value(double num) {

    emit_opcode(OP_CONSTANT);
    Value* val = create_value(VAL_FNUM);
    val->as.fnum = num;
    return add_constant(val);
}

size_t emit_unum_value(uint64_t num) {

    emit_opcode(OP_CONSTANT);
    Value* val = create_value(VAL_UNUM);
    val->as.unum = num;
    return add_constant(val);
}

size_t emit_inum_value(int64_t num) {

    emit_opcode(OP_CONSTANT);
    Value* val = create_value(VAL_INUM);
    val->as.inum = num;
    return add_constant(val);
}

size_t emit_obj_value(Obj* obj) {

    emit_opcode(OP_CONSTANT);
    Value* val = create_value(VAL_OBJ);
    val->as.obj = obj;
    return add_constant(val);
}

size_t add_constant(Value* value) {

    write_value_list(vm->block, value);
    write_code_list(vm->block, value_list_size(vm->block) - 1);
    return value_list_size(vm->block) - 1;
}

static void print_object(const Value* val) {

    switch(val->as.obj->type) {
        case OBJ_STRING:
            printf("%s", value_as_cstring((Value*)val));
            break;
    }
}

void print_value(const Value* value) {

    switch(value->type) {
        case VAL_FNUM:
            printf("%0.3f", value->as.fnum);
            break;
        case VAL_UNUM:
            printf("0x%lX", value->as.unum);
            break;
        case VAL_INUM:
            printf("%ld", value->as.inum);
            break;
        case VAL_NOTHING:
            printf("nothing");
            break;
        case VAL_BOOL:
            printf("%s", value->as.bval? "true": "false");
            break;
        case VAL_OBJ:
            print_object(value);
            break;
        default: printf("unknown type");
    }
}

