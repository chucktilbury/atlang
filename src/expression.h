/**
    @file expression.h

    @brief

**/
#ifndef __PRECEDENCE_H__
#define __PRECEDENCE_H__

#include "common.h"

typedef enum {
    PREC_NONE,
    PREC_ASSIGNMENT,  // =
    PREC_OR,          // or
    PREC_AND,         // and
    PREC_EQUALITY,    // == !=
    PREC_COMPARISON,  // < > <= >=
    PREC_TERM,        // + -
    PREC_FACTOR,      // * /
    PREC_UNARY,       // ! -
    PREC_CALL,        // . ()
    PREC_PRIMARY
} Precedence;

typedef void (*ParseFunc)();

typedef struct {
    ParseFunc prefix;
    ParseFunc infix;
    Precedence prec;
} ParseRule;

void get_precedence(Precedence prec);
void expression();

#endif
