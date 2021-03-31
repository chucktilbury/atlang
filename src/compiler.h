/**
    @file compiler.h

    @brief

**/
#ifndef __COMPILER_H__
#define __COMPILER_H__

#include "common.h"

typedef struct {
    Token* crnt;
    Token* prev;
    bool hadError;
    bool panicMode;
} Parser;

void advance();
void consume(TokenType type);
void expression();
void compile();

#endif
