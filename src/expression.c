/**
    @brief

**/
#include "common.h"
#include "expression.h"
#include "scanner.h"
#include "compiler.h"
#include "vmachine.h"

extern Parser parser;

static void fnum();
static void inum();
static void unum();
static void grouping();
static void unary();
static void abinary();
static void cbinary();
static void literal();
static void string();

static ParseRule rules[] = {
    [END_OF_INPUT] = {NULL,      NULL,       PREC_NONE},
    [END_OF_FILE] = {NULL,      NULL,       PREC_NONE},
    [SYMBOL_TOKEN] = {NULL,      NULL,       PREC_NONE},
    [UNUM_TOKEN] = {unum,      NULL,       PREC_NONE},
    [INUM_TOKEN] = {inum,      NULL,       PREC_NONE},
    [FNUM_TOKEN] = {fnum,      NULL,       PREC_NONE},
    [ONUM_TOKEN] = {NULL,      NULL,       PREC_NONE},
    [QSTRG_TOKEN] = {string,      NULL,       PREC_NONE},
    [SLASH_TOKEN] = {NULL,     abinary,     PREC_FACTOR},
    [MUL_TOKEN] = {NULL,      abinary,     PREC_FACTOR},
    [MOD_TOKEN] = {NULL,      abinary,     PREC_FACTOR},
    [COMMA_TOKEN] = {NULL,      NULL,       PREC_NONE},
    [SEMIC_TOKEN] = {NULL,      NULL,       PREC_NONE},
    [COLON_TOKEN] = {NULL,      NULL,       PREC_NONE},
    [OSQU_TOKEN] = {NULL,      NULL,       PREC_NONE},
    [CSQU_TOKEN] = {NULL,      NULL,       PREC_NONE},
    [OCUR_TOKEN] = {NULL,      NULL,       PREC_NONE},
    [CCUR_TOKEN] = {NULL,      NULL,       PREC_NONE},
    [OPAR_TOKEN] = {grouping,  NULL,       PREC_NONE},
    [CPAR_TOKEN] = {NULL,      NULL,       PREC_NONE},
    [DOT_TOKEN] = {NULL,      NULL,       PREC_NONE},
    [EQU_TOKEN] = {NULL,      NULL,       PREC_NONE},
    [LT_TOKEN] = {NULL,      cbinary,       PREC_COMPARISON},
    [GT_TOKEN] = {NULL,      cbinary,       PREC_COMPARISON},
    [SUB_TOKEN] = {unary,      abinary,     PREC_TERM},
    [ADD_TOKEN] = {NULL,      abinary,     PREC_TERM},
    [NOT_TOKEN] = {unary,      NULL,       PREC_NONE},
    [EQUALITY_TOKEN] = {NULL,      cbinary,       PREC_EQUALITY},
    [LTE_TOKEN] = {NULL,      cbinary,       PREC_COMPARISON},
    [GTE_TOKEN] = {NULL,      cbinary,       PREC_COMPARISON},
    [DEC_TOKEN] = {NULL,      NULL,       PREC_NONE},
    [INC_TOKEN] = {NULL,      NULL,       PREC_NONE},
    [NEQ_TOKEN] = {NULL,      cbinary,       PREC_EQUALITY},
    [AND_TOKEN] = {NULL,      NULL,       PREC_NONE},
    [OR_TOKEN] = {NULL,      NULL,       PREC_NONE},
    [FOR_TOKEN] = {NULL,      NULL,       PREC_NONE},
    [IF_TOKEN] = {NULL,      NULL,       PREC_NONE},
    [ELSE_TOKEN] = {NULL,      NULL,       PREC_NONE},
    [WHILE_TOKEN] = {NULL,      NULL,       PREC_NONE},
    [DO_TOKEN] = {NULL,      NULL,       PREC_NONE},
    [SWITCH_TOKEN] = {NULL,      NULL,       PREC_NONE},
    [CASE_TOKEN] = {NULL,      NULL,       PREC_NONE},
    [BREAK_TOKEN] = {NULL,      NULL,       PREC_NONE},
    [CONTINUE_TOKEN] = {NULL,      NULL,       PREC_NONE},
    [TRY_TOKEN] = {NULL,      NULL,       PREC_NONE},
    [EXCEPT_TOKEN] = {NULL,      NULL,       PREC_NONE},
    [RAISE_TOKEN] = {NULL,      NULL,       PREC_NONE},
    [DEFAULT_TOKEN] = {NULL,      NULL,       PREC_NONE},
    [CREATE_TOKEN] = {NULL,      NULL,       PREC_NONE},
    [DESTROY_TOKEN] = {NULL,      NULL,       PREC_NONE},
    [SUPER_TOKEN] = {NULL,      NULL,       PREC_NONE},
    [ENTRY_TOKEN] = {NULL,      NULL,       PREC_NONE},
    [CLASS_TOKEN] = {NULL,      NULL,       PREC_NONE},
    [CONSTRUCTOR_TOKEN] = {NULL,      NULL,       PREC_NONE},
    [DESTRUCTOR_TOKEN] = {NULL,      NULL,       PREC_NONE},
    [IMPORT_TOKEN] = {NULL,      NULL,       PREC_NONE},
    [TRUE_TOKEN] = {literal,      NULL,       PREC_NONE},
    [FALSE_TOKEN] = {literal,      NULL,       PREC_NONE},
    [NOTHING_TOKEN] = {literal,      NULL,       PREC_NONE},
    [BOOL_TOKEN] = {NULL,      NULL,       PREC_NONE},
    [MAP_TOKEN] = {NULL,      NULL,       PREC_NONE},
    [DICT_TOKEN] = {NULL,      NULL,       PREC_NONE},
    [LIST_TOKEN] = {NULL,      NULL,       PREC_NONE},
    [UINT_TOKEN] = {NULL,      NULL,       PREC_NONE},
    [INT_TOKEN] = {NULL,      NULL,       PREC_NONE},
    [FLOAT_TOKEN] = {NULL,      NULL,       PREC_NONE},
    [STRING_TOKEN] = {NULL,      NULL,       PREC_NONE},
    [INLINE_TOKEN] = {NULL,      NULL,       PREC_NONE},
    [PUBLIC_TOKEN] = {NULL,      NULL,       PREC_NONE},
    [PRIVATE_TOKEN] = {NULL,      NULL,       PREC_NONE},
    [PROTECTED_TOKEN] = {NULL,      NULL,       PREC_NONE},
    [RETURN_TOKEN] = {NULL,      NULL,       PREC_NONE},
    [AS_TOKEN] = {NULL,      NULL,       PREC_NONE},
    [NAMESPACE_TOKEN] = {NULL,      NULL,       PREC_NONE},
};

static void fnum() {

    double num = strtod(parser.prev->str, NULL);
    emit_fnum_value(num);
}

static void inum() {

    int64_t num = strtol(parser.prev->str, NULL, 10);
    emit_inum_value(num);
}

static void unum() {

    uint64_t num = strtol(parser.prev->str, NULL, 16);
    emit_unum_value(num);
}

static void grouping() {

    expression();
    consume(CPAR_TOKEN);
}

static void unary() {

    TokenType otype = parser.prev->type;

    get_precedence(PREC_UNARY);
    switch(otype) {
        case SUB_TOKEN: emit_opcode(OP_NEG);    break;
        case NOT_TOKEN: emit_opcode(OP_NOT);    break;
        default:
            fatal_error("unknown operator type in unary()");
    }
}

static void abinary() {

    TokenType type = parser.prev->type;

    ParseRule* rule = &rules[type];
    get_precedence((Precedence)(rule->prec + 1));

    switch(type) {
        case ADD_TOKEN:     emit_opcode(OP_ADD); break;
        case SUB_TOKEN:     emit_opcode(OP_SUB); break;
        case MUL_TOKEN:     emit_opcode(OP_MUL); break;
        case SLASH_TOKEN:   emit_opcode(OP_DIV); break;
        case MOD_TOKEN:     emit_opcode(OP_MOD); break;
        default:
            fatal_error("unknown type in abinary()");
    }
}

static void cbinary() {

    TokenType type = parser.prev->type;

    ParseRule* rule = &rules[type];
    get_precedence((Precedence)(rule->prec + 1));

    switch(type) {
        case EQUALITY_TOKEN: emit_opcode(OP_EQUALITY); break;
        case NEQ_TOKEN: emit_opcode(OP_NEQ); break;
        case LT_TOKEN:  emit_opcode(OP_LT); break;
        case GT_TOKEN:  emit_opcode(OP_GT); break;
        case LTE_TOKEN: emit_opcode(OP_LTE); break;
        case GTE_TOKEN: emit_opcode(OP_GTE); break;
        default:
            fatal_error("unknown type in cbinary()");
    }
}

static void literal() {

    switch(parser.prev->type) {
        case FALSE_TOKEN:   emit_opcode(OP_FALSE);  break;
        case TRUE_TOKEN:    emit_opcode(OP_TRUE);   break;
        case NOTHING_TOKEN:    emit_opcode(OP_NOTHING);   break;
        default: return; /* unreachable */
    }
}

static void string() {

    Obj* val = create_string_object(parser.prev->str);
    emit_obj_value(val);
}

void expression() {

    get_precedence(PREC_ASSIGNMENT);
}


void get_precedence(Precedence prec) {

    advance();
    ParseFunc prefix = rules[parser.prev->type].prefix;
    if(prefix == NULL) {
        parser.hadError = true;
        printf("tok value = %d\n", parser.prev->type);
        syntax("expected an expression but got %s", token_to_str(parser.prev->type));
        return;
    }
    prefix();

    while(prec <= rules[parser.crnt->type].prec) {
        advance();
        ParseFunc infix = rules[parser.prev->type].infix;
        infix();
    }
}
