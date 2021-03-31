/**
    @file disassembler.c

    @brief

**/
#include "common.h"

extern VMachine* vm;

static size_t simple_instruction(const char* name, size_t offset) {

    printf("%s\n", name);
    return offset + 1;
}

static size_t constant_instruction(const char* name, codeBlock* cb, size_t offset) {

    uint16_t* code = raw_code_list(cb);
    uint16_t constant = code[offset + 1];
    printf("%-16s %4d ", name, constant);
    Value** vals = raw_value_list(cb);
    print_value(vals[constant]);
    printf("\n");

    return offset + 2;
}

void disassemble_codeblock(const char* name) {

    printf("\ndisassemble block\n\n== %s ==\n", name);

    codeBlock* code_block = vm->block;
    size_t length = code_list_size(code_block);
    for(size_t offset = 0; offset < length; /* empty */) {
        offset = disassemble_instruction(code_block, offset);
    }
}

int disassemble_instruction(codeBlock* code_block, size_t offset) {

    printf("%04lu ", offset);

    uint16_t* code = raw_code_list(code_block);
    uint16_t instruction = code[offset];
    switch(instruction) {
        case OP_CONSTANT:
            return constant_instruction("OP_CONSTANT", code_block, offset);
        case OP_ADD:    return simple_instruction("OP_ADD", offset);
        case OP_SUB:    return simple_instruction("OP_SUB", offset);
        case OP_MUL:    return simple_instruction("OP_MUL", offset);
        case OP_DIV:    return simple_instruction("OP_DIV", offset);
        case OP_MOD:    return simple_instruction("OP_MOD", offset);
        case OP_EQUALITY: return simple_instruction("OP_EQUALITY", offset);
        case OP_NEQ:    return simple_instruction("OP_NEQ", offset);
        case OP_LT:     return simple_instruction("OP_LT", offset);
        case OP_GT:     return simple_instruction("OP_GT", offset);
        case OP_LTE:    return simple_instruction("OP_LTE", offset);
        case OP_GTE:    return simple_instruction("OP_GTE", offset);
        case OP_NEG:    return simple_instruction("OP_NEG", offset);
        case OP_NOTHING: return simple_instruction("OP_NOTHING", offset);
        case OP_TRUE:   return simple_instruction("OP_TRUE", offset);
        case OP_FALSE:  return simple_instruction("OP_FALSE", offset);
        case OP_NOT:    return simple_instruction("OP_NOT", offset);
        case OP_RETURN: return simple_instruction("OP_RETURN", offset);
        default:
            printf("OPCODE ERROR: Unknown opcode %d\n", instruction);
            return offset + 1;
    }
}

