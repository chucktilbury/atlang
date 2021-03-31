/**
    @file scanner.c

    @brief The complete scanner. See scanner.h for interface functions.

**/
#include "common.h"

// This is supposed to be prototyped in stdio.h, but it does not seem to be.
extern FILE* fmemopen(void*, size_t, const char*);

typedef struct __file_stack {
    char* fname;
    bool is_file;
    union {
        FILE* fp;
        const char* str;
    } input;
    int line_no;
    int col_no;
    struct __file_stack* next;
} file_stack_t;

static file_stack_t* top = NULL;

typedef struct {
    const char* str;
    TokenType tok;
} token_map_t;

static int nest_depth = 0;
static char_buffer_t scanner_buffer;
static int file_flag = 0;
static int last_col;

/**
    @brief This is the data structure that converts keywords to tokens.

    This data structure must be in sorted order.
    To update, see the file keywordlist.txt in ./tests
    1. Add the keywords with the token.
    2. cat keywordlist.txt | sort
    3. copy/paste the result here.
**/
static token_map_t token_map[] = {
    {"and", AND_TOKEN},
    {"as", AS_TOKEN},
    {"bool", BOOL_TOKEN},
    {"break", BREAK_TOKEN},
    {"case", CASE_TOKEN},
    {"class", CLASS_TOKEN},
    {"constructor", CONSTRUCTOR_TOKEN},
    {"continue", CONTINUE_TOKEN},
    {"default", DEFAULT_TOKEN},
    {"destructor", DESTRUCTOR_TOKEN},
    {"dict", DICT_TOKEN},
    {"do", DO_TOKEN},
    {"else", ELSE_TOKEN},
    {"entry", ENTRY_TOKEN},
    {"equ", EQUALITY_TOKEN},
    {"except", EXCEPT_TOKEN},
    {"false", FALSE_TOKEN},
    {"float", FLOAT_TOKEN},
    {"for", FOR_TOKEN},
    {"gte", GTE_TOKEN},
    {"gt", GT_TOKEN},
    {"if", IF_TOKEN},
    {"import", IMPORT_TOKEN},
    {"inline", INLINE_TOKEN},
    {"int", INT_TOKEN},
    {"list", LIST_TOKEN},
    {"lte", LTE_TOKEN},
    {"lt", LT_TOKEN},
    {"map", MAP_TOKEN},
    {"neq", NEQ_TOKEN},
    {"not", NOT_TOKEN},
    {"nothing", NOTHING_TOKEN},
    {"or", OR_TOKEN},
    {"private", PRIVATE_TOKEN},
    {"protected", PROTECTED_TOKEN},
    {"public", PUBLIC_TOKEN},
    {"raise", RAISE_TOKEN},
    {"return", RETURN_TOKEN},
    {"string", STRING_TOKEN},
    {"super", SUPER_TOKEN},
    {"switch", SWITCH_TOKEN},
    {"true", TRUE_TOKEN},
    {"try", TRY_TOKEN},
    {"uint", UINT_TOKEN},
    {"nothing", NOTHING_TOKEN},
    {"while", WHILE_TOKEN},
};

static const size_t token_map_size = (sizeof(token_map) / sizeof(token_map_t));

/**
    @brief Close the currently open file and update the file stack.
    This is automatically called when a file ends.

**/
void close_input_file() {

    log_debug("enter top = %p", top);
    if(top != NULL) {
        file_stack_t* fsp = top;
        top = top->next; // could make top NULL
        if(fsp->fname != NULL)
            FREE(fsp->fname);

        if(fsp->is_file)
            fclose(fsp->input.fp);
        // if the stream is a string, then it's free()d by caller.

        FREE(fsp);
    }
    log_debug("leave top = %p", top);
}

/**
    @brief Get the char object
    Read a single character from the input stream.

    @return int
**/
static int get_char() {

    int ch;

    // defer closing the file until after the END_OF_FILE token has been read.
    if(file_flag) {
        file_flag = 0;
        close_input_file();
    }

    if(top != NULL) {
        ch = fgetc(top->input.fp);
        log_debug("char: %c", ch);
        if(ch == EOF) {
            log_debug("end of file");
            ch = END_FILE;
        }
        else if(ch == '\n') {
            top->line_no++;
            last_col = top->col_no;
            top->col_no = 1;
        }
        else {
            top->col_no++;
            //last_col = top->col_no;
        }
    }
    else {
        log_debug("end of input");
        ch = END_INPUT;
    }
    return ch;
}


/**
    @brief Stuff the character back into the input stream.

    @param ch
**/
static void unget_char(int ch) {

    if(top != NULL) {
        ungetc(ch, top->input.fp);
        if(ch == '\n') {
            top->line_no--;
            top->col_no = last_col;
        }
        else {
            if(top->col_no > 1)
                top->col_no--;
            else
                top->col_no = 1;
        }
    }
}

/**
    @brief Skip white space.

**/
static void skip_ws() {

    int ch, finished = 0;

    while(!finished) {
        ch = get_char();
        if(!isspace(ch)) {
            unget_char(ch);
            finished++;
        }
        else if(ch == END_INPUT)
            finished++;
    }
    // next char in input stream is not blank.
}


/**
    @brief Eat characters until white space is encountered.
    Used for the scanner to recover from an error. The white space is returned
    to the input stream.
**/
static void eat_until_ws() {

    int ch;
    while(!isspace(ch = get_char()))
        add_char_buffer(scanner_buffer, ch);
    unget_char(ch);
}

/**
    @brief Eat a single line comment.

**/
static void eat_single_line() {

    int ch, finished = 0;

    while(!finished) {
        ch = get_char();
        if(ch == '\n' || ch == END_FILE) {
            //return NONE_TOKEN;
            unget_char(ch);
            break;
        }
    }
}

/**
    @brief Eat a multi-line comment.

**/
static void eat_multi_line() {

    int ch, state = 0;

    while(ch != END_OF_INPUT) {
        ch = get_char();
        switch(state) {
            case 0:
                if(ch == '*')
                    state = 1;
                //fprintf(stderr, "0");
                break;
            case 1:
                if(ch == '/')
                    state = 2;
                else if(ch != '*')
                    state = 0;
                //fprintf(stderr, "1");
                break;
            case 2:
                unget_char(ch);
                return;
        }
    }
}

/**
    @brief Finish reading a hex number from the input stream.
    When this is called, the "0x" has already been scanned and is a part
    of the scanned token.

    A hex number has the format of 0xnnnnnnnn, with a maimum of 16 characters.

    @return TokenType
**/
static TokenType read_hex_number() {

    int ch;
    while(isxdigit(ch = get_char()))
        add_char_buffer(scanner_buffer, ch);

    return UNUM_TOKEN;
}

/**
    @brief Finish reading an octal number.
    An octal number starts with a '0' and all digits are less than '7'.

    @return TokenType
**/
static TokenType read_octal_number() {

    int ch;
    while(isdigit(ch = get_char())) {
        if(ch <= '7')
            add_char_buffer(scanner_buffer, ch);
        else {
            // eat the rest of the number and publish an error
            while(isdigit(ch = get_char()))
                add_char_buffer(scanner_buffer, ch);
            unget_char(ch);
            syntax("malformed octal number: %s", get_char_buffer(scanner_buffer));
            return ERROR_TOKEN;
        }
    }
    return ONUM_TOKEN;
}

// we can only enter here <after> we have seen a '.'
/**
    @brief Scan a floating point number.
    When this is called the first part of the number, up to the '.' has been
    seen. The number may have a normal exponent.

    @return TokenType
**/
static TokenType read_float_number() {

    int ch;
    while(isdigit(ch = get_char())) {
        add_char_buffer(scanner_buffer, ch);
    }

    // see if we are reading a mantisa
    if(ch == 'e' || ch == 'E') {
        add_char_buffer(scanner_buffer, ch);
        ch = get_char();
        if(ch == '+' || ch == '-') {
            add_char_buffer(scanner_buffer, ch);
            ch = get_char();
            if(isdigit(ch)) {
                add_char_buffer(scanner_buffer, ch);
            }
            else {
                unget_char(ch);
                syntax("malformed float number: %s", get_char_buffer(scanner_buffer));
                return ERROR_TOKEN;
            }
            while(isdigit(ch = get_char())) {
                add_char_buffer(scanner_buffer, ch);
            }
            unget_char(ch);
        }
        else if(isdigit(ch)) {
            add_char_buffer(scanner_buffer, ch);
            while(isdigit(ch = get_char())) {
                add_char_buffer(scanner_buffer, ch);
            }
        }
        else {
            // eat the rest of the number and publish an error
            while(isdigit(ch = get_char()))
                add_char_buffer(scanner_buffer, ch);
            unget_char(ch);
            syntax("malformed float number: %s", get_char_buffer(scanner_buffer));
            return ERROR_TOKEN;
        }
    }
    else
        unget_char(ch);

    return FNUM_TOKEN;
}

/**
    @brief A digit has been seen.
    This is the entry to find out what kind of number is being read and then
    return the associated token. The token string is in the string buffer when
    this finishes. Supported types of numbers are integer, hex, float and
    octal.

    @return TokenType
**/
static TokenType read_number_top() {

    int ch = get_char();

    // first char is always a digit
    if(ch == '0') { // could be hex, octal, decimal, or float
        add_char_buffer(scanner_buffer, ch);
        ch = get_char();
        if(ch == 'x' || ch == 'X') {
            add_char_buffer(scanner_buffer, ch);
            return read_hex_number();
        }
        else if(ch == '.') {
            add_char_buffer(scanner_buffer, ch);
            return read_float_number();
        }
        else if(isdigit(ch)) { // is an octal number
            if(ch <= '7') {
                add_char_buffer(scanner_buffer, ch);
                return read_octal_number();
            }
            else {
                // it's a malformed number. eat the rest of it and post an error
                while(isdigit(ch = get_char()))
                    add_char_buffer(scanner_buffer, ch);
                unget_char(ch);
                syntax("malformed octal number: %s", get_char_buffer(scanner_buffer));
                return ERROR_TOKEN;
            }
        }
        else { // it's just a zero
            unget_char(ch);
            return INUM_TOKEN;
        }
    }
    else { // It's either a dec or a float.
        int finished = 0;
        add_char_buffer(scanner_buffer, ch);
        while(!finished) {
            ch = get_char();
            if(isdigit(ch))
                add_char_buffer(scanner_buffer, ch);
            else if(ch == '.') {
                add_char_buffer(scanner_buffer, ch);
                return read_float_number();
            }
            else {
                finished++;
                unget_char(ch);
                return INUM_TOKEN;
            }
        }
    }
    // If we reach here, then some syntax that is not covered has been submitted
    return ERROR_TOKEN;
}

/**
    @brief Get the hex escape object

**/
static void get_hex_escape() {

    // When this is called, a hex escape sequence is embedded in a string.The
    // '\x' has already been read.The converted value is in the string when this
    // returns.

    char tbuf[9];   // hex escape has a maximum of 8 characters
    int idx, ch;

    memset(tbuf, 0, sizeof(tbuf));
    for(idx = 0; idx < (int)sizeof(tbuf); idx++) {
        ch = get_char();
        if(isxdigit(ch))
            tbuf[idx] = ch;
        else
            break;
    }

    if(strlen(tbuf) == 0) {
        warning("invalid hex escape code in string: '%c' is not a hex digit. Ignored.", ch);
        unget_char(ch);
    }
    else {
        int val = (int)strtol(tbuf, NULL, 16);
        add_char_buffer_int(scanner_buffer, val);
    }
}

/**
    @brief Get the octal escape object
    When this is called the '\0' has already been read. The converted binary
    value is in the string when this returns.

**/
static void get_octal_escape() {

    char tbuf[4];   // hex escape has a maximum of 3 characters
    int idx, ch;

    memset(tbuf, 0, sizeof(tbuf));
    for(idx = 0; idx < (int)sizeof(tbuf); idx++) {
        ch = get_char();
        if(ch >= '0' && ch <= '7')
            tbuf[idx] = ch;
        else
            break;
    }

    if(strlen(tbuf) == 0) {
        warning("invalid octal escape code in string: '%c' is not a octal digit. Ignored.", ch);
        unget_char(ch);
    }
    else {
        int val = (int)strtol(tbuf, NULL, 8);
        add_char_buffer(scanner_buffer, val);
    }
}

/**
    @brief Get the decimal escape object
**/
static void get_decimal_escape() {

    // When this is read, a '\D' has been read.The binary value is in the
    // string when this returns.
    char tbuf[11]; // maximum of 10 characters in decimal escape
    int idx = 0, ch;

    memset(tbuf, 0, sizeof(tbuf));
    ch = get_char();
    // decimal escape may be signed
    if(ch == '+' || ch == '-') {
        tbuf[idx] = ch;
        idx++;
    }

    for(; idx < (int)sizeof(tbuf); idx++) {
        ch = get_char();
        if(isdigit(ch))
            tbuf[idx] = ch;
        else
            break;
    }

    if(strlen(tbuf) == 0) {
        warning("invalid decimal escape code in string: '%c' is not a decimal digit. Ignored.", ch);
        unget_char(ch);
    }
    else {
        int val = (int)strtol(tbuf, NULL, 10);
        add_char_buffer_int(scanner_buffer, val);
    }
}

/**
    @brief Get the string esc object
    When this is entered, a back-slash has been seen in a dquote string.
    could have an octal, hex, or character escape.

**/
static void get_string_esc() {

    int ch = get_char();
    switch(ch) {
        case 'x':
        case 'X': get_hex_escape(); break;
        case 'd':
        case 'D': get_decimal_escape(); break;
        case '0': get_octal_escape(); break;
        case 'n': add_char_buffer(scanner_buffer, '\n'); break;
        case 'r': add_char_buffer(scanner_buffer, '\r'); break;
        case 't': add_char_buffer(scanner_buffer, '\t'); break;
        case 'b': add_char_buffer(scanner_buffer, '\b'); break;
        case 'f': add_char_buffer(scanner_buffer, '\f'); break;
        case 'v': add_char_buffer(scanner_buffer, '\v'); break;
        case '\\': add_char_buffer(scanner_buffer, '\\'); break;
        case '\"': add_char_buffer(scanner_buffer, '\"'); break;
        case '\'': add_char_buffer(scanner_buffer, '\''); break;
        default: add_char_buffer(scanner_buffer, ch); break;
    }
}

/**
    @brief Read a string with escape translations.
    When this is entered, a (") has been seen and discarded. This function
    performs escape replacements, but it does not do formatting. that takes
    place in the parser. If consecutive strings are encountered, separated
    only by white space, then it is a string continuance and is a multi line
    string.

    @return TokenType
**/
static TokenType read_dquote() {

    int finished = 0;
    int ch;
    TokenType tok = QSTRG_TOKEN;

    while(!finished) {
        ch = get_char();
        switch(ch) {
            case '\\':
                get_string_esc();
                break;
            case '\"':
                finished++;
                break;
            case '\n':
                syntax("line breaks are not allowed in a string.");
                eat_until_ws();
                return ERROR_TOKEN;
                break;
            default:
                add_char_buffer(scanner_buffer, ch);
                break;
        }
    }

    return tok;
}

/**
    @brief Read a string without escape translations.
    When this is entered, a (') has been seen and discarded. This function
    copies whatever is found into a string, exactly as it is found in the
    source code. If consecutive strings are found separated only by white
    space, then it is a single string that is being scanned.

    @return TokenType
**/
static TokenType read_squote() {

    int finished = 0;
    int ch;
    TokenType tok = QSTRG_TOKEN;

    while(!finished) {
        ch = get_char();
        switch(ch) {
            case '\'':
                finished++;
                break;
            case '\n':
                syntax("line breaks are not allowed in a string.");
                eat_until_ws();
                return ERROR_TOKEN;
                break;
            default:
                add_char_buffer(scanner_buffer, ch);
                break;
        }
    }

    return tok;
}

/**
    @brief Convert the given keyword string to a token.
    If it is not a keyword, then SYMBOL_TOKEN is returned. Note that this does
    not convert non-keywords to a token. Non-keywords cause SYMBOL_TOKEN to be
    returned as well.

    @param str
    @return TokenType
**/
static TokenType str_to_token(const char* str) {

    // simple binary search
    TokenType retv = SYMBOL_TOKEN;
    int start = 0, end = token_map_size - 1;
    int mid = (start + end) / 2;

    while(start <= end) {
        int spot = strcmp(str, token_map[mid].str);
        if(spot > 0)
            start = mid + 1;
        else if(spot < 0)
            end = mid - 1;
        else {
            retv = token_map[mid].tok;
            break;
        }

        mid = (start + end) / 2;
    }

    return retv;
}

/**
    @brief Read a word from the input and then find out if it's a keyword.

    @return TokenType
**/
static TokenType read_word() {

    int c;
    int finished = 0;

    while(!finished) {
        c = get_char();
        if(isalnum(c) || c == '_')
            add_char_buffer(scanner_buffer, c);
        else {
            finished++;
        }
    }
    unget_char(c);
    const char* find = get_char_buffer(scanner_buffer);

    return str_to_token(find);
}

/**
    @brief Read a punctuation character from input stream.
    When this is entered, a punctuation character has been read. If it's a (_)
    then we have a symbol, otherwise, it's an operator.

    @param ch
    @return TokenType
**/
static TokenType read_punct(int ch) {

    if(ch == '_') {
        unget_char('_');
        // wasteful binary search for something that is certinly a symbol
        return read_word();
    }
    else {
        switch(ch) {
            // single character operators
            case '*': return MUL_TOKEN; break;
            case '%': return MOD_TOKEN; break;
            case ',': return COMMA_TOKEN; break;
            case ';': return SEMIC_TOKEN; break;
            case ':': return COLON_TOKEN; break;
            case '[': return OSQU_TOKEN; break;
            case ']': return CSQU_TOKEN; break;
            case '{': return OCUR_TOKEN; break;
            case '}': return CCUR_TOKEN; break;
            case '(': return OPAR_TOKEN; break;
            case ')': return CPAR_TOKEN; break;
            case '.': return DOT_TOKEN; break;
            case '|': return OR_TOKEN; break; // comparison
            case '&': return AND_TOKEN; break; // comparison

            // could be single or double
            case '=': {
                int c = get_char();
                if(c == '=')
                    return EQUALITY_TOKEN;
                else {
                    unget_char(c);
                    return EQU_TOKEN;
                }
            }
                    break;
            case '<': {
                int c = get_char();
                if(c == '=')
                    return LTE_TOKEN;
                else if(c == '>')
                    return NEQ_TOKEN;
                else {
                    unget_char(c);
                    return LT_TOKEN;
                }
            }
                    break;
            case '>': {
                int c = get_char();
                if(c == '=')
                    return GTE_TOKEN;
                else {
                    unget_char(c);
                    return GT_TOKEN;
                }
            }
                    break;
            case '-': {
                int c = get_char();
                if(c == '-')
                    return DEC_TOKEN;
                else {
                    unget_char(c);
                    return SUB_TOKEN;
                }
            }
                    break;
            case '+': {
                int c = get_char();
                if(c == '+')
                    return INC_TOKEN;
                else {
                    unget_char(c);
                    return ADD_TOKEN;
                }
            }
                    break;
            case '!': {
                int c = get_char();
                if(c == '=')
                    return NEQ_TOKEN;
                else {
                    unget_char(c);
                    return NOT_TOKEN;
                }
            }
                    break;

                    // these are not recognized
            default:
                warning("unrecognized character in input: '%c' (0x%02X). Ignored.", ch, ch);
                return NONE_TOKEN;
                break;
        }
    }
    return ERROR_TOKEN; // should never happen
}

/**
    @brief Find out what to do with a '/' character.
    When this is entered, a (/) has been seen. If the next character is a
    (/) or a (*), then we have a comment, otherwise we have a / operator.

**/
static TokenType comment_or_operator() {

    int ch = get_char();

    if(ch == '/') {
        // eat a single line comment
        eat_single_line();
        return NONE_TOKEN;
    }
    else if(ch == '*') {
        // eat a multi line comment
        eat_multi_line();
        return NONE_TOKEN;
    }
    else {
        // not a comment, must be a / single-character operator
        unget_char(ch);
        return SLASH_TOKEN;
    }

    // probably an error for the parser to handle
    return END_OF_INPUT;
}

// Called by atexit()
void destroy_scanner() {

    log_trace("enter top = %p", top);
    destroy_char_buffer(scanner_buffer);
    while(top != NULL)
        close_input_file();
    log_trace("leave top = %p", top);
}

/**************************************
 * Interface functions
 ***************************************/

/**
    @brief Allocate memory for the token.

    @return Token*
**/
Token* create_token(TokenType type, const char* str) {

    Token* tok = ALLOC_DS(Token);
    tok->type = type;
    tok->str = STRDUP(str);
    tok->line_no = get_line_no();
    tok->column_no = get_column_no();

    return tok;
}

/**
    @brief Free the memory allocated to the token.

**/
void free_token(Token* tok) {

    if(tok != NULL) {
        FREE(tok->str);
        FREE(tok);
    }
}

/**
    @brief Convert a token to a string for documentation and errors.

    @param tok
    @return const char*
**/
const char* token_to_str(TokenType tok) {

    // this is the ONLY place this behemoth is expanded.
    return TOK_TO_STR(tok);
}

/**
    @brief Create the scanner.
    This must be called before any other scanner function.

**/
void init_scanner() {

    scanner_buffer = create_char_buffer();
    //atexit(destroy_scanner);
}

/**
    @brief Read text from the input stream and return a token.

    @return TokenType
**/
Token* get_tok() {

    int ch, finished = 0;
    TokenType tok = NONE_TOKEN;

    skip_ws();
    init_char_buffer(scanner_buffer);

    while(!finished) {
        ch = get_char();
        switch(ch) {
            case END_FILE:
                file_flag++;
                tok = END_OF_FILE;
                finished++;
                break;
            case END_INPUT:
                tok = END_OF_INPUT;
                finished++;
                break;
            case '\"':  // read a double quoted string
                tok = read_dquote();
                finished++;
                break;
            case '\'': // read a single quoted string
                tok = read_squote();
                finished++;
                break;
            case '/': // may have a comment or an operator
                tok = comment_or_operator();
                skip_ws();
                if(tok != NONE_TOKEN)
                    finished++;
                // continue if it was a comment
                break;
            default:
                if(ch == END_FILE)
                    unget_char(ch);
                else if(isdigit(ch)) { // beginning of a number. Could be hex, dec, or float
                    unget_char(ch);
                    tok = read_number_top();
                    if(tok != NONE_TOKEN)
                        finished++;
                }
                else if(isalpha(ch)) { // could be a symbol or a keyword
                    unget_char(ch);
                    tok = read_word();
                    if(tok != NONE_TOKEN)
                        finished++;
                }
                else if(ispunct(ch)) { // some kind of operator (but not a '/' or a quote)
                    tok = read_punct(ch);
                    if(tok != NONE_TOKEN)
                        finished++;
                }
                else {
                    warning("Unknown character, ignoring. (0x%02X) (%d) \'%c\'", ch, ch, ch);
                }
        }
    }
    return create_token(tok, get_char_buffer(scanner_buffer));
}

/**
    @brief Retrieve a token and compare it against a token array.
    If the received token is not in the array, then issue a syntax error and
    return the ERROR_TOKEN. The array must be terminated with the NONE_TOKEN.
    (AKA 0)

    @param arr
    @return TokenType
**/
Token* expect_tok_array(TokenType* arr) {

    Token* token = get_tok();
    for(int i = 0; arr[i] != NONE_TOKEN; i++) {
        if(token->type == arr[i])
            return token;
    }

    // this is contrived to look like a standard syntax error
    // calling this on an empty input stream will produce a wonky message
    FILE* fp = get_err_stream();
    fprintf(fp, "Syntax Error: %s: %d: %d: expected ",
        get_file_name(), token->line_no, token->column_no);
    for(int i = 0; arr[i] != NONE_TOKEN; i++) {
        fprintf(fp, "%s", token_to_str(arr[i]));
        if(arr[i + 1] != NONE_TOKEN)
            fprintf(fp, ",");
        fprintf(fp, " ");
    }
    fprintf(fp, "but got a %s.\n", token_to_str(token->type));
    return create_token(ERROR_TOKEN, "error");
}

/**
    @brief Retrieve the next token and check it against the specified token type.
    If the token does not match the parameter, then a syntax error is issued.

    @param tok
    @return TokenType
**/
Token* expect_tok(TokenType tok) {

    Token* token = get_tok();
    if(token->type != tok) {
        syntax("expected a %s but got a %s.", token_to_str(tok), token_to_str(token->type));
        return create_token(ERROR_TOKEN, "error");
    }
    else
        return token;
}

/*
    Open a file.
*/

/**
    @brief Open an input file.
    This pushes the file onto a file stack and causes get_tok() to begin
    scanning the file. When the file ends, then scanning resumes where it
    left off when a new file was opened.

    @param fname
**/
void open_scanner_file(const char* fname) {

    nest_depth++;
    if(nest_depth > MAX_FILE_NESTING) {
        fatal_error("Maximum file nesting depth exceeded.");
        exit(1);
    }

    FILE* fp = fopen(fname, "r");
    if(fp == NULL) {
        fatal_error("Cannot open input file: \"%s\": %s", fname, strerror(errno));
    }

    file_stack_t* fstk = ALLOC_DS(file_stack_t);
    fstk->fname = STRDUP(fname);
    fstk->input.fp = fp;
    fstk->line_no = 1;
    fstk->col_no = 1;

    if(top != NULL)
        fstk->next = top;
    top = fstk;
}

void open_scanner_string(const char* str) {

    FILE* fp = fmemopen((void*)str, strlen(str), "r");
    if(fp == NULL) {
        fatal_error("Cannot open input stream: \"%s\": %s", "REPL", strerror(errno));
    }

    file_stack_t* fstk = ALLOC_DS(file_stack_t);
    fstk->fname = STRDUP("REPL");
    fstk->input.fp = fp;
    fstk->line_no = 1;
    fstk->col_no = 1;

    if(top != NULL)
        fstk->next = top;
    top = fstk;
}

/*
    Return the name of the currently open file.
*/

/**
    @brief Return the current file name.
    For error handling. If no file is open, then return the string "no file
    open".

    @return const char*
**/
const char* get_file_name() {

    if(top != NULL)
        return top->fname;
    else
        return "no open file";
}

/**
    @brief Return the current line number in the current file.
    If there is no file open then return -1.

    @return int
**/
int get_line_no() {

    if(top != NULL)
        return top->line_no;
    else
        return -1;
}

/**
    @brief Return the current column number in the current file.
    If there is no file open then return -1.

    @return int
**/
int get_column_no() {

    if(top != NULL)
        return top->col_no;
    else
        return -1;
}

#if 0
/**
    @brief Return the current token string.

    @return const char*
**/
const char* get_tok_str() {
    return get_char_buffer(scanner_buffer);
}
#endif
