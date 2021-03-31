/**
    @file disassembler.h

    @brief

**/
#ifndef __DISASSEMBLER_H__
#define __DISASSEMBLER_H__

#include "common.h"

void disassemble_codeblock(const char*);
int disassemble_instruction(codeBlock*, size_t);

#endif
