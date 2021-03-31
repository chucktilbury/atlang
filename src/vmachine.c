/**
    @file vmachine.c

    @brief

**/
#include <math.h>

#include "common.h"

VMachine* vm;

static inline void create_value_stack() {
    vm->vstack = (valueStack*)create_ptr_list();
}

static inline void free_value_stack() {
    destroy_ptr_list(vm->vstack);
}

static inline void push_value_stack(Value* val) {
    push_ptr_list(vm->vstack, val);
}

static inline Value* pop_value_stack() {
    return (Value*)pop_ptr_list(vm->vstack);
}

static inline Value** raw_value_stack() {
    return (Value**)(vm->vstack->buffer);
}

static inline size_t value_stack_size() {
    return (vm->vstack->nitems);
}

Value* peek_value_stack() {
    return (Value*)peek_ptr_list(vm->vstack);
}

void destroy_vmachine() {

    log_debug("enter");
    if(vm != NULL) {
        if(vm->block != NULL) {
            log_debug("codeblock size = %d", vm->block->code->nitems);
            free_codeblock(vm->block);
        }

        if(vm->vstack != NULL) {
            log_debug("vstack items = %d", vm->vstack->nitems);
            for(int i = 0; i < vm->vstack->nitems; i++)
                FREE(vm->vstack->buffer[i]);
            free_value_stack();
        }

        FREE(vm);
    }
    log_debug("leave");
}

void init_vmachine() {

    vm = ALLOC_DS(VMachine);
    vm->block = create_codeblock();
    vm->lastIp = 0;
    create_value_stack();

    //atexit(free_vmachine);
}

/**
    @brief Clear the value stack, but leave the rest of the machine intact.

**/
void reset_vmachine() {

    log_debug("enter");
    Value* val;
    if(vm->vstack != NULL) {
        while(true) {
            val = pop_value_stack();
            if(val == NULL)
                return;
            else {
                free_value(val);
            }
        }
    }
    log_debug("leave");
}

static inline ValueType
            __attribute__((always_inline))
            conv_value(Value* val, ValueType type) {

    ValueType result = VAL_INVALID;
    ValueType from = val->type;

    switch(type) {
        case VAL_INUM:
            result = VAL_INUM;
            val->type = VAL_INUM;
            switch(from) {
                case VAL_UNUM: val->as.inum = (int64_t)val->as.unum; break;
                case VAL_FNUM: val->as.inum = (int64_t)val->as.fnum; break;
                case VAL_OBJ:  conv_obj_to_val(val, VAL_INUM); break;
                case VAL_BOOL:
                    runtime_error("cannot convert signed int to boolean");
                    result = VAL_INVALID;
                    break;
                case VAL_NOTHING:
                    runtime_error("cannot convert signed int to nothing");
                    result = VAL_INVALID;
                    break;
                default: {} /* do nothing */
            }
            break;
        case VAL_UNUM:
            result = VAL_UNUM;
            val->type = VAL_UNUM;
            switch(from) {
                case VAL_INUM: val->as.unum = (uint64_t)val->as.inum; break;
                case VAL_FNUM: val->as.unum = (uint64_t)val->as.fnum; break;
                case VAL_OBJ:  conv_obj_to_val(val, VAL_UNUM); break;
                case VAL_BOOL:
                    runtime_error("cannot convert unsigned int to boolean");
                    result = VAL_INVALID;
                    break;
                case VAL_NOTHING:
                    runtime_error("cannot convert unsigned int to nothing");
                    result = VAL_INVALID;
                    break;
                default: {} /* do nothing */
            }
            break;
        case VAL_FNUM:
            result = VAL_FNUM;
            switch(from) {
                case VAL_INUM: val->as.fnum = (double)val->as.inum; break;
                case VAL_UNUM: val->as.fnum = (double)val->as.unum; break;
                case VAL_OBJ:  conv_obj_to_val(val, VAL_FNUM); break;
                case VAL_BOOL:
                    runtime_error("cannot convert float to boolean");
                    result = VAL_INVALID;
                    break;
                case VAL_NOTHING:                    break;
                    runtime_error("cannot convert float to nothing");
                    result = VAL_INVALID;
                    break;
                default: {} /* do nothing */
            }
            break;
        case VAL_BOOL:
            result = VAL_BOOL;
            switch(from) {
                case VAL_INUM: val->as.bval = (val->as.inum == 0); break;
                case VAL_UNUM: val->as.bval = (val->as.unum == 0); break;
                case VAL_FNUM: val->as.bval = (val->as.fnum == 0.0); break;
                case VAL_OBJ:  conv_obj_to_val(val, VAL_BOOL); break;
                case VAL_NOTHING: val->as.bval = true; break;
                default: {} /* do nothing */
            }
            break;
        // THis is broken. How do we know the object type to convert to?
        case VAL_OBJ:
            result = VAL_OBJ;
            switch(from) {
                case VAL_INUM: conv_val_to_obj(val, OBJ_STRING); break;
                case VAL_UNUM: conv_val_to_obj(val, OBJ_STRING); break;
                case VAL_FNUM: conv_val_to_obj(val, OBJ_STRING); break;
                case VAL_BOOL: conv_val_to_obj(val, OBJ_STRING); break;
                case VAL_NOTHING:
                    runtime_error("nothing value in expression");
                    result = VAL_INVALID;
                default: {} /* do nothing */
            }
            break;
        case VAL_NOTHING:
            runtime_error("cannot convert 'nothing' to another type");
            break;
        default:
            runtime_error("invalid value type in conv_value() (unreachable)");
    }
    return result;
}

/**
    @brief Convert two operands to the same type for a comparison or arithmetic
    operation. The precedence of the conversion, from high to low is BOOL,
    FLOAT, INT, UINT. When an object is on the left, the right is converted to
    an object. When an object is on the right, then the object is converted to
    the type of the left operand.

    @param op1
    @param op2
    @return ValueType
**/
static ValueType normalize_operands(Value* op1, Value* op2) {

    // note that we can add strings....
    // if((!value_is_number(op1) && !value_is_bool(op1) && !value_is_object(op1)) ||
    //     (!value_is_number(op2) && !value_is_bool(op2) && !value_is_object(op2)))
    //     return VAL_INVALID;

    ValueType type1 = op1->type;
    ValueType type2 = op2->type;
    ValueType result;

    switch(type1) {
        case VAL_OBJ:
            switch(type2) {
                //case VAL_OBJ:
                case VAL_INUM:
                case VAL_UNUM:
                case VAL_FNUM:
                case VAL_BOOL:
                case VAL_NOTHING:
                    result = conv_value(op2, VAL_OBJ);
                    break;
                default:
                    result = VAL_OBJ;
                    break;
            }
            break;

        case VAL_BOOL:
            switch(type2) {
                //case VAL_OBJ:
                case VAL_INUM:
                case VAL_UNUM:
                case VAL_BOOL:
                case VAL_NOTHING:
                    result = conv_value(op2, VAL_BOOL);
                    break;
                case VAL_FNUM:
                    runtime_warning("converting float to bool can produce unexpected results");
                    result = conv_value(op2, VAL_BOOL);
                    break;
                default:
                    result = VAL_BOOL;
                    break;
            }
            break;
        case VAL_INUM:
            switch(type2) {
                case VAL_INUM:
                //case VAL_OBJ:
                case VAL_UNUM:
                case VAL_NOTHING:
                    result = conv_value(op2, VAL_INUM);
                    break;
                case VAL_BOOL: result = conv_value(op1, VAL_BOOL); break;
                case VAL_FNUM: result = conv_value(op1, VAL_FNUM); break;
                default:
                    result = VAL_INUM;
                    break;
            }
            break;
        case VAL_UNUM:
            switch(type2) {
                case VAL_INUM:
                case VAL_UNUM:
                case VAL_NOTHING:
                case VAL_BOOL:
                    result = conv_value(op1, VAL_UNUM);
                    break;
                case VAL_FNUM:
                    result = conv_value(op1, VAL_UNUM);
                    runtime_warning("converting unsigned to float can produce unexpected results");
                    break;
                case VAL_OBJ:
                    result = conv_value(op2, VAL_UNUM);
                    break;
                default:
                    result = VAL_UNUM;
                    break;
            }
            break;
        case VAL_FNUM:
            switch(type2) {
                case VAL_INUM:
                case VAL_UNUM:
                case VAL_FNUM:
                case VAL_NOTHING:
                    result = conv_value(op1, VAL_FNUM);
                    break;
                case VAL_BOOL:
                    result = conv_value(op1, VAL_FNUM);
                    runtime_warning("converting float to bool can produce unexpected results");
                    break;
                case VAL_OBJ:
                    result = conv_value(op2, VAL_FNUM);
                    break;
                default:
                    result = VAL_FNUM;
                    break;
            }
            break;

        case VAL_NOTHING:
            switch(type2) {
                case VAL_INUM:
                case VAL_UNUM:
                case VAL_FNUM:
                case VAL_NOTHING:
                case VAL_BOOL:
                case VAL_OBJ:
                default:
                    result = VAL_INVALID;
                    runtime_error("nothing value in expression");
                    break;
            }
            break;

        default:
            runtime_error("unknown value type: %d", type1);
    }
    return result;
}

static inline InterpretResult __attribute__((always_inline)) compare_op(uint16_t op, size_t ip) {

    log_debug("binary comparison operation start");

    InterpretResult result = INTERPRET_OK;
    Value* op2 = pop_value_stack();
    Value* op1 = pop_value_stack();
    ValueType vt = normalize_operands(op1, op2);
    if(vt != VAL_INVALID) {
        log_debug("vt = %d", vt);
        Value* val = create_value(VAL_BOOL);
        switch(op) {
            case OP_EQUALITY: // strings
                switch(vt) {
                    case VAL_OBJ:  val->as.bval = compare_objects(op1, op2, op); break;
                    case VAL_INUM: val->as.bval = op1->as.inum == op2->as.inum; break;
                    case VAL_UNUM: val->as.bval = op1->as.unum == op2->as.unum; break;
                    case VAL_BOOL: val->as.bval = op1->as.bval == op2->as.bval; break;
                    case VAL_FNUM:
                        val->as.bval = op1->as.fnum == op2->as.fnum;
                        runtime_warning("comparing floats for equality can produce unexpected results");
                        break;
                    default:
                        result = INTERPRET_RUNTIME_ERROR;
                        runtime_error("unknown value type: %d at %d", vt, ip);
                }
                break;
            case OP_NEQ:
                switch(vt) {
                    case VAL_OBJ:  val->as.bval = compare_objects(op1, op2, op); break;
                    case VAL_INUM: val->as.bval = op1->as.inum != op2->as.inum; break;
                    case VAL_UNUM: val->as.bval = op1->as.unum != op2->as.unum; break;
                    case VAL_BOOL: val->as.bval = op1->as.bval != op2->as.bval; break;
                    case VAL_FNUM:
                        val->as.bval = op1->as.fnum != op2->as.fnum;
                        runtime_warning("comparing floats for equality can produce unexpected results");
                        break;
                    default:
                        result = INTERPRET_RUNTIME_ERROR;
                        runtime_error("unknown value type: %d at %d", vt, ip);
                }
                break;
            case OP_LT:
                switch(vt) {
                    case VAL_OBJ:  val->as.bval = compare_objects(op1, op2, op); break;
                    case VAL_INUM: val->as.bval = op1->as.inum < op2->as.inum; break;
                    case VAL_UNUM: val->as.bval = op1->as.unum < op2->as.unum; break;
                    case VAL_BOOL: val->as.bval = op1->as.bval < op2->as.bval; break;
                    case VAL_FNUM: val->as.bval = op1->as.fnum < op2->as.fnum; break;
                    default:
                        result = INTERPRET_RUNTIME_ERROR;
                        runtime_error("unknown value type: %d at %d", vt, ip);
                }
                break;
            case OP_GT:
                switch(vt) {
                    case VAL_OBJ:  val->as.bval = compare_objects(op1, op2, op); break;
                    case VAL_INUM: val->as.bval = op1->as.inum > op2->as.inum; break;
                    case VAL_UNUM: val->as.bval = op1->as.unum > op2->as.unum; break;
                    case VAL_BOOL: val->as.bval = op1->as.bval > op2->as.bval; break;
                    case VAL_FNUM: val->as.bval = op1->as.fnum > op2->as.fnum; break;
                    default:
                        result = INTERPRET_RUNTIME_ERROR;
                        runtime_error("unknown value type: %d at %d", vt, ip);
                }
                break;
            case OP_LTE:
                switch(vt) {
                    case VAL_OBJ:  val->as.bval = compare_objects(op1, op2, op); break;
                    case VAL_INUM: val->as.bval = op1->as.inum <= op2->as.inum; break;
                    case VAL_UNUM: val->as.bval = op1->as.unum <= op2->as.unum; break;
                    case VAL_BOOL: val->as.bval = op1->as.bval <= op2->as.bval; break;
                    case VAL_FNUM: val->as.bval = op1->as.fnum <= op2->as.fnum; break;
                    default:
                        result = INTERPRET_RUNTIME_ERROR;
                        runtime_error("unknown value type: %d at %d", vt, ip);
                }
                break;
            case OP_GTE:
                switch(vt) {
                    case VAL_OBJ:  val->as.bval = compare_objects(op1, op2, op); break;
                    case VAL_INUM: val->as.bval = op1->as.inum >= op2->as.inum; break;
                    case VAL_UNUM: val->as.bval = op1->as.unum >= op2->as.unum; break;
                    case VAL_BOOL: val->as.bval = op1->as.bval >= op2->as.bval; break;
                    case VAL_FNUM: val->as.bval = op1->as.fnum >= op2->as.fnum; break;
                    default:
                        result = INTERPRET_RUNTIME_ERROR;
                        runtime_error("unknown value type: %d at %d", vt, ip);
                }
                break;
            default:
                result = INTERPRET_RUNTIME_ERROR;
                runtime_error("invalid opcode in compare_op()");
        }
        push_value_stack(val);
    }
    else {
        result = INTERPRET_RUNTIME_ERROR;
        runtime_error("invalid type for binary comparison operation: %d at %d", vt, ip);
    }

    log_debug("binary comparison operation finished");
    return result;
}

static inline InterpretResult __attribute__((always_inline)) arithmetic_op(uint16_t op, size_t ip) {

    log_debug("binary arithmetic operation start");

    InterpretResult result = INTERPRET_OK;
    Value* op2 = pop_value_stack();
    Value* op1 = pop_value_stack();
    ValueType vt = normalize_operands(op1, op2);
    if(vt != VAL_INVALID) {
        Value* val = create_value(vt);
        switch(op) {
            case OP_ADD:
                switch(vt) {
                    case VAL_OBJ:  val->as.obj = arithmetic_objects(op1, op2, op); break;
                    case VAL_INUM: val->as.inum = op1->as.inum + op2->as.inum; break;
                    case VAL_UNUM: val->as.unum = op1->as.unum + op2->as.unum; break;
                    case VAL_FNUM: val->as.fnum = op1->as.fnum + op2->as.fnum; break;
                    case VAL_BOOL:
                        result = INTERPRET_RUNTIME_ERROR;
                        runtime_error("arithmetic operation on boolean type at %d", ip);
                        break;
                    default:
                        result = INTERPRET_RUNTIME_ERROR;
                        runtime_error("unknown value type: %d at %d", vt, ip);
                }
                break;
            case OP_SUB:
                switch(vt) {
                    case VAL_OBJ:  val->as.obj = arithmetic_objects(op1, op2, op); break;
                    case VAL_INUM: val->as.inum = op1->as.inum - op2->as.inum; break;
                    case VAL_UNUM: val->as.unum = op1->as.unum - op2->as.unum; break;
                    case VAL_FNUM: val->as.fnum = op1->as.fnum - op2->as.fnum; break;
                    case VAL_BOOL:
                        result = INTERPRET_RUNTIME_ERROR;
                        runtime_error("arithmetic operation on boolean type at %d", ip);
                        break;
                    default:
                        result = INTERPRET_RUNTIME_ERROR;
                        runtime_error("unknown value type: %d at %d", vt, ip);
                }
                break;
            case OP_MUL:
                switch(vt) {
                    case VAL_OBJ:  val->as.obj = arithmetic_objects(op1, op2, op); break;
                    case VAL_INUM: val->as.inum = op1->as.inum * op2->as.inum; break;
                    case VAL_UNUM: val->as.unum = op1->as.unum * op2->as.unum; break;
                    case VAL_FNUM: val->as.fnum = op1->as.fnum * op2->as.fnum; break;
                    case VAL_BOOL:
                        result = INTERPRET_RUNTIME_ERROR;
                        runtime_error("arithmetic operation on boolean type at %d", ip);
                        break;
                    default:
                        result = INTERPRET_RUNTIME_ERROR;
                        runtime_error("unknown value type: %d at %d", vt, ip);
                }
                break;
            case OP_DIV:
                switch(vt) {
                    case VAL_OBJ:  val->as.obj = arithmetic_objects(op1, op2, op); break;
                    case VAL_INUM: val->as.inum = op1->as.inum / op2->as.inum; break;
                    case VAL_UNUM: val->as.unum = op1->as.unum / op2->as.unum; break;
                    case VAL_FNUM: val->as.fnum = op1->as.fnum / op2->as.fnum; break;
                    case VAL_BOOL:
                        result = INTERPRET_RUNTIME_ERROR;
                        runtime_error("arithmetic operation on boolean type at %d", ip);
                        break;
                    default:
                        result = INTERPRET_RUNTIME_ERROR;
                        runtime_error("unknown value type: %d at %d", vt, ip);
                }
                break;
            case OP_MOD:
                switch(vt) {
                    case VAL_OBJ:  val->as.obj = arithmetic_objects(op1, op2, op); break;
                    case VAL_INUM: val->as.inum = op1->as.inum % op2->as.inum; break;
                    case VAL_UNUM: val->as.unum = op1->as.unum % op2->as.unum; break;
                    case VAL_FNUM: val->as.fnum = fmod(op1->as.fnum, op2->as.fnum); break;
                    case VAL_BOOL:
                        result = INTERPRET_RUNTIME_ERROR;
                        runtime_error("arithmetic operation on boolean type at %d", ip);
                        break;
                    default:
                        result = INTERPRET_RUNTIME_ERROR;
                        runtime_error("unknown value type: %d at %d", vt, ip);
                }
                break;

            default:
                result = INTERPRET_RUNTIME_ERROR;
                runtime_error("invalid opcode in arithmetic_op()");
        }
        push_value_stack(val);
    }
    else {
        result = INTERPRET_RUNTIME_ERROR;
        runtime_error("invalid type for binary arithmetic operation: %d at %d", vt, ip);
    }

    log_debug("binary arithmetic operation finished");
    return result;
}

#ifdef DEBUG_TRACE_EXECUTION
#define trace_instruction(ofst) \
    do {\
        printf("     stack: "); \
        Value** stack = raw_value_stack(); \
        size_t limit = value_stack_size(); \
        for(size_t idx = 0;  idx < limit; idx++) { \
            printf("[ "); \
            print_value(stack[idx]); \
            printf(" ]"); \
        } \
        printf("\n"); \
        disassemble_instruction((vm)->block, (ofst)); \
    } while(false)
#else
#define trace_instruction(ofst)
#endif

InterpretResult run_vmachine(VMachine* vm) {

    if(vm == NULL)
        return INTERPRET_RUNTIME_ERROR;
    if(vm->block == NULL)
        return INTERPRET_RUNTIME_ERROR;

    printf("\nrun vm\n");
    bool finished = false;
    InterpretResult result = INTERPRET_OK;
    Value** value_list = raw_value_list(vm->block);
    uint16_t* instruction_list = raw_code_list(vm->block);
    size_t ip = vm->lastIp;

    while(!finished) {
        uint16_t instruction = instruction_list[ip];
        trace_instruction(ip);
        switch(instruction) {
            case OP_CONSTANT: {
                ip++;
                Value* value = value_list[instruction_list[ip++]];
                push_value_stack(value);
            }
            break;

            case OP_EQUALITY:
            case OP_NEQ:
            case OP_LT:
            case OP_GT:
            case OP_LTE:
            case OP_GTE:
                result = compare_op(instruction, ip);
                if(result == INTERPRET_OK)
                    ip++;
                else
                    finished = true; // error already posted.
                break;

            case OP_ADD:
            case OP_SUB:
            case OP_MUL:
            case OP_DIV:
            case OP_MOD:
                result = arithmetic_op(instruction, ip);
                if(result == INTERPRET_OK)
                    ip++;
                else
                    finished = true; // error already posted.
                break;

            case OP_NEG: { // unary operation
                    Value* op = pop_value_stack();
                    ValueType vt = op->type;
                    Value* val = create_value(vt); // avoid double free();
                    if(value_is_number(op) || value_is_bool(op)) {
                        switch(vt) {
                            case VAL_INUM: val->as.inum = -op->as.inum; break;
                            case VAL_UNUM: val->as.unum = -op->as.unum; break;
                            case VAL_FNUM: val->as.fnum = -op->as.fnum; break;
                            case VAL_BOOL: val->as.bval = -op->as.bval; break;
                            default:
                                finished = true;
                                result = INTERPRET_RUNTIME_ERROR;
                                runtime_error("unknown value type: %d at %d", vt, ip);
                        }
                        push_value_stack(val);
                        ip++;
                    }
                    else {
                        finished = true;
                        result = INTERPRET_RUNTIME_ERROR;
                        runtime_error("expected number or bool, but got: %d at %d", vt, ip);
                    }
                }
                break;

            case OP_NOTHING:
                ip++;
                push_value_stack(create_value(VAL_NOTHING));
                break;

            case OP_TRUE: {
                    ip++;
                    Value* val = create_value(VAL_BOOL);
                    val->as.bval = true;
                    push_value_stack(val);
                }
                break;

            case OP_FALSE: {
                    ip++;
                    Value* val = create_value(VAL_BOOL);
                    val->as.bval = false;
                    push_value_stack(val);
                }
                break;

            case OP_RETURN:
                ip++;
                vm->lastIp = ip;
                finished = true;
                break;

            case OP_NOT: {
                    ip++;
                    Value* op = pop_value_stack();
                    Value* val = create_value(VAL_BOOL);
                    val->as.bval = (value_is_nothing(op) || (value_is_bool(op) && !op->as.bval));
                    push_value_stack(val);
                }
                break;

            default:
                finished = true;
                result = INTERPRET_RUNTIME_ERROR;
                runtime_error("unknown opcode: %d, %d", instruction, ip);   // does not return
        }
    }

    return result;
}




#if 0
#define BINARY_COP(oper) \
    do { \
        log_debug("binary comparison operation start"); \
        Value* op2 = pop_value_stack(); \
        Value* op1 = pop_value_stack(); \
        ValueType vt = normalize_operands(op1, op2); \
        if(vt != VAL_INVALID) { \
            log_debug("vt = %d", vt); \
            Value* val = create_value(VAL_BOOL); \
            switch(vt) { \
                case VAL_INUM: val->as.bval = op1->as.inum oper op2->as.inum; break; \
                case VAL_UNUM: val->as.bval = op1->as.unum oper op2->as.unum; break; \
                case VAL_BOOL: val->as.bval = op1->as.bval oper op2->as.bval; break;\
                case VAL_FNUM: \
                    val->as.bval = op1->as.fnum oper op2->as.fnum; \
                    runtime_warning("comparing floats for equality can produce unexpected results"); \
                    break; \
                default:  \
                    finished = true; \
                    result = INTERPRET_RUNTIME_ERROR; \
                    runtime_error("unknown value type: %d at %d", vt, ip); \
            } \
            push_value_stack(val); \
            ip++; \
        } \
        else { \
            finished = true; \
            result = INTERPRET_RUNTIME_ERROR; \
            runtime_error("invalid type for binary comparison operation: %d at %d", vt, ip); \
        } \
        log_debug("binary comparison operation finished"); \
    } while(false)
#endif

#if 0
#define BINARY_AOP(oper) \
    do { \
        log_debug("binary arithmetic operation start"); \
        Value* op2 = pop_value_stack(); \
        Value* op1 = pop_value_stack(); \
        ValueType vt = normalize_operands(op1, op2); \
        if(vt != VAL_INVALID) { \
            Value* val = create_value(vt); \
            switch(vt) { \
                case VAL_INUM: val->as.inum = op1->as.inum oper op2->as.inum; break; \
                case VAL_UNUM: val->as.unum = op1->as.unum oper op2->as.unum; break; \
                case VAL_FNUM: val->as.fnum = op1->as.fnum oper op2->as.fnum; break; \
                case VAL_BOOL: \
                    finished = true; \
                    result = INTERPRET_RUNTIME_ERROR; \
                    runtime_error("arithmetic operation on boolean type at %d", ip); \
                    break; \
                default:  \
                    finished = true; \
                    result = INTERPRET_RUNTIME_ERROR; \
                    runtime_error("unknown value type: %d at %d", vt, ip); \
            } \
            push_value_stack(val); \
            ip++; \
        } \
        else { \
            finished = true; \
            result = INTERPRET_RUNTIME_ERROR; \
            runtime_error("invalid type for binary arithmetic operation: %d at %d", vt, ip); \
        } \
        log_debug("binary arithmetic operation finished"); \
    } while(false)
#endif

