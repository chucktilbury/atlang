/**
    @file compiler.c

    @brief

**/
#include "common.h"

//#include "vmachine.h"
#ifdef DEBUG_PRINT_CODE
#   include "disassembler.h"
#endif

Parser parser;

void advance() {

    if(parser.prev != NULL)
        FREE(parser.prev);

    parser.prev = parser.crnt;
    while(true) {
        parser.crnt = get_tok();
        if(parser.crnt->type != ERROR_TOKEN)
            break;
        // Skip error tokens that are delivered by the scanner.
        // The scanner has already posted an error.
    }
}

void consume(TokenType type) {

    if(parser.crnt->type == type) {
        advance();
        return;
    }

    if(parser.panicMode)
        return;

    parser.hadError = true;
    syntax("expected a %s but got a %s", token_to_str(type), token_to_str(parser.crnt->type));
    parser.panicMode = true;
}


/**
    @brief Compile from the input stream.
    This assumes that there is an open input stream for the scanner to scan
    from.

    The compiler and the parser are integrated together.
**/
void compile() {

    parser.hadError = false;
    parser.panicMode = false;

    advance();
    expression();
    consume(END_OF_FILE);
    consume(END_OF_INPUT);

    emit_opcode(OP_RETURN);
#ifdef DEBUG_PRINT_CODE
    //if(!parser.hadError) {
    disassemble_codeblock("code");
    //}
#endif
}
