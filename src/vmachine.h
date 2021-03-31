/**
    @file vmachine.h

    @brief

**/
#ifndef __VMACHINE_H__
#define __VMACHINE_H__

#include "common.h"

typedef enum {
    INTERPRET_OK,
    INTERPRET_COMPILE_ERROR,
    INTERPRET_RUNTIME_ERROR,
} InterpretResult;

typedef ptr_list_t valueStack;

typedef struct {
    codeBlock* block;
    valueStack* vstack;
    size_t lastIp;
    //uint16_t* ip;   // instruction pointer
} VMachine;

void init_vmachine();
void destroy_vmachine();
void reset_vmachine();
//void free_vmachine(VMachine*);
//void set_codeblock(VMachine*, codeBlock*);
InterpretResult run_vmachine(VMachine*);
Value* peek_value_stack();
#endif
