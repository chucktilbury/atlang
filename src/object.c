/**
    @file object.c

    @brief

**/
#include <regex.h>
#include "common.h"


Obj* create_string_object(const char* str) {

    ObjString* sobj = ALLOC_DS(ObjString);
    sobj->obj.type = OBJ_STRING;
    sobj->str = create_char_buffer();
    add_char_buffer_str(sobj->str, str);
    sobj->len = strlen(str);

    return (Obj*)sobj;
}

/**
    @brief Free an object that was allocated in objects.c This method is not to
    be called for values that are not objects.

    @param obj
**/
void free_object(Obj* obj) {

    switch(obj->type) {
        case OBJ_STRING:
            destroy_char_buffer(((ObjString*)obj)->str);
            FREE(obj);
            break;
        default:
            fatal_error("unknown object type in free_object()");
    }
}

/**
    @brief Compare two objects. This will only be called for values of type
    VAL_OBJ. Other value types will likely cause a segfault.

    @param op1
    @param op2
    @param op
    @return true
    @return false
**/
bool compare_objects(Value* op1, Value* op2, OpCode op) {

    if(op1->type != op2->type)
        return false;

    // both objects are the same type
    switch(op) {
        case OP_EQUALITY:
            switch(op1->as.obj->type) {
                case OBJ_STRING: {
                    size_t len1 = ((ObjString*)(op1->as.obj))->len;
                    size_t len2 = ((ObjString*)(op2->as.obj))->len;
                    if(len1 != len2)
                        return false;
                    const char* str1 = get_char_buffer(((ObjString*)(op1->as.obj))->str);
                    const char* str2 = get_char_buffer(((ObjString*)(op2->as.obj))->str);
                    return memcmp(str1, str2, MIN(len1, len2)) == 0;
                }
                               break;
                default:
                    fatal_error("unknown object type in compare_object()");
            }
            break;
        case OP_NEQ:
            fatal_error("opcode not-equal is not supported for objects" );
            break;
        case OP_LT:
            fatal_error("opcode less-than is not supported for objects");
            break;
        case OP_GT:
            fatal_error("opcode greater-than is not supported for objects");
            break;
        case OP_LTE:
            fatal_error("opcode less-or-equal is not supported for objects");
            break;
        case OP_GTE:
            fatal_error("opcode greater-or-equal is not supported for objects");
            break;
        default:
            fatal_error("unknown opcode in compare_objects()");


    }

    return false;
}

/**
    @brief Stub for future use.

    @param op1
    @param op2
    @param op
**/
static Obj* arith_op_for_class(Value* op1, Value* op2, OpCode op) {

    (void)op1;
    (void)op2;
    (void)op;
    return NULL;
}

/**
    @brief Perform an arithmetic operation on two objects. Returns a new object
    or NULL if the operation was invalid. This assumes that both values are
    objects, but does not assume the object type.

    @param op1
    @param op2
    @param op
    @return Obj*
**/
Obj* arithmetic_objects(Value* op1, Value* op2, OpCode op) {

    Obj* nobj = NULL;
    ObjectType otype1 = op1->as.obj->type;
    ObjectType otype2 = op2->as.obj->type;

    switch(op) {
        case OP_ADD:
            switch(otype1) {
                case OBJ_STRING:
                    switch(otype2) {
                        case OBJ_STRING: {
                                const char* str1 = value_as_cstring(op1);
                                const char* str2 = value_as_cstring(op2);
                                nobj = create_string_object(str1);
                                add_char_buffer_str(((ObjString*)nobj)->str, str2);
                            }
                            break;
                        default:
                            return arith_op_for_class(op1, op2, op);
                    }
                    break;
                default:
                    return arith_op_for_class(op1, op2, op);
            }
            break;
        // stubs for later
        case OP_SUB: return arith_op_for_class(op1, op2, op);
        case OP_MUL: return arith_op_for_class(op1, op2, op);
        case OP_DIV: return arith_op_for_class(op1, op2, op);
        case OP_MOD: return arith_op_for_class(op1, op2, op);
        case OP_NEG: return arith_op_for_class(op1, op2, op);
        default:
            fatal_error("unknown operator in arithmetic_objects()");
    }

    return nobj;
}


static bool re_match(const char* regex, const char* str) {

    regex_t re;

    if(regcomp(&re, regex, REG_EXTENDED | REG_ICASE | REG_NOSUB) != 0)
        fatal_error("failed to compile regex in validate_float()");

    int status = regexec(&re, str, 0, NULL, 0);
    regfree(&re);

    return status == 0;
}

/**
    @brief Validate that the string can represent a floating point number.

    @param str
    @return true
    @return false
**/
static bool validate_float(const char* str) {

    const char* regex = "^[+-]?([0-9]*\\.)?[0-9]+(e[-+]?[0-9]+)?$";
    return re_match(regex, str);
}

/**
    @brief Validate an unsigend number, which is always represented as hex.

    @param str
    @return true
    @return false
**/
static bool validate_unsigned(const char* str) {

    const char* regex = "^0x[0-9a-f]+$";
    return re_match(regex, str);
}

/**
    @brief Validate a signed number, which cannot have a leading zero.

    @param str
    @return true
    @return false
**/
static bool validate_signed(const char* str) {

    const char* regex = "^[^0][0-9]+$";
    return re_match(regex, str);
}

/**
    @brief Validate a boolean value which can have the values of "true" or "false".
    This does not evaluate as case-sensitive.

    @param str
    @return true
    @return false
**/
static bool validate_bool(const char* str) {

    const char* regex = "^true|false$";
    return re_match(regex, str);
}

#if 0
/**
    @brief Validate a "nothing" keyword. This is overkill for a single word, but I do
    it anyway. This is evaluated as not case-sensitive.

    @param str
    @return true
    @return false
**/
static bool validate_nothing(const char* str) {

    const char* regex = "^nothing$";
    return re_match(regex, str);
}
#endif

/**
    @brief Convert the object value to another type. This will be used mainly
    to implement casts, but it is also be used to implement formatting strings.

    The value is converted in place, and the type of the value is returned. If
    the object could not be converted then VAL_INVALID is returned, so the
    caller should always check that. When the conversion is successful, the
    object is free()d.

    The validate routines are necessary to ensure a correct conversion or else
    correctly failing if appropriate.

    @param val
    @param type
    @return ValueType
**/
ValueType conv_obj_to_val(Value* val, ValueType type) {

    const char* buf;

    if(val->type != VAL_OBJ)
        return VAL_INVALID; // not an object

    switch(type) {
        case VAL_INUM:
            val->type = VAL_INUM;
            switch(val->as.obj->type) {
                case OBJ_STRING:
                    buf = get_char_buffer(((ObjString*)val->as.obj)->str);
                    if(validate_signed(buf)) {
                        val->as.inum = (int64_t)strtol(buf, NULL, 10);
                        free_object(val->as.obj);
                    }
                    else
                        return VAL_INVALID;
                    break;
                default:
                    fatal_error("connot convert object to value");
            }
            break;
        case VAL_UNUM:
            val->type = VAL_UNUM;
            switch(val->as.obj->type) {
                case OBJ_STRING:
                    // UNUMs are always hex.
                    buf = get_char_buffer(((ObjString*)val->as.obj)->str);
                    if(validate_unsigned(buf)) {
                        val->as.unum = (uint64_t)strtol(buf, NULL, 16);
                        free_object(val->as.obj);
                    }
                    else
                        return VAL_INVALID;
                    break;
                default:
                    fatal_error("connot convert object to value");
            }
            break;
        case VAL_FNUM:
            val->type = VAL_FNUM;
            switch(val->as.obj->type) {
                case OBJ_STRING:
                    buf = get_char_buffer(((ObjString*)val->as.obj)->str);
                    if(validate_float(buf)) {
                        val->as.fnum = (int64_t)strtod(buf, NULL);
                        free_object(val->as.obj);
                    }
                    else
                        return VAL_INVALID;
                    break;
                default:
                    fatal_error("connot convert object to value");
            }
            break;
        case VAL_BOOL:
            val->type = VAL_BOOL;
            switch(val->as.obj->type) {
                case OBJ_STRING:
                    buf = get_char_buffer(((ObjString*)val->as.obj)->str);
                    if(validate_bool(buf)) {
                        val->as.bval = (strcmp(buf, "true") == 0);
                        free_object(val->as.obj);
                    }
                    else
                        return VAL_INVALID;
                    break;
                default:
                    fatal_error("connot convert object to value");
            }
            break;
        case VAL_NOTHING:
            val->type = VAL_NOTHING;
            free_object(val->as.obj);
            break;
        case VAL_OBJ:
            fatal_error("connot convert object to another value type in conv_obj_to_val()");
            break;
        default:
            fatal_error("unknown value type in conv_obj_to_val()");
    }

    return val->type;
}

/**
    @brief Convert a value to an object type. This will be invalid in most
    cases, but is required for robust formatting of strings. It can also be
    used for casting variables.

    The value is converted in place and the type returned is VAL_OBJ with the
    object type specified in the obj member. If the value could not be
    converted, then the value will have the type VAL_INVALID and the contents
    of the value are undefined. If the value is already an object, then only
    the obj member is modified.

    @param val
    @return ValueType
**/
ValueType conv_val_to_obj(Value* val, ObjectType type) {

    char buf[1024];

    switch(type) {
        case OBJ_STRING:
            switch(val->type) {
                case VAL_INUM: snprintf(buf, sizeof(buf), "%ld", val->as.inum); break;
                case VAL_UNUM: snprintf(buf, sizeof(buf), "0x%lX", val->as.unum); break;
                case VAL_FNUM: snprintf(buf, sizeof(buf), "%0.f", val->as.fnum); break;
                case VAL_BOOL: strcpy(buf, (val->as.bval)? "true": "false"); break;
                case VAL_NOTHING: strcpy(buf, "nothing"); break;
                case VAL_OBJ:
                    // fatal error does not return
                    fatal_error("cannot convert object value to a string in conv_val_to_obj()");
                    break;
                default:
                    fatal_error("unknown value type in conv_val_to_obj()");
            }
            val->type = VAL_OBJ;
            val->as.obj = create_string_object(buf);
            break;
        default:
            fatal_error("unknown object type in conv_val_to_obj()");
    }
    return val->type;
}
