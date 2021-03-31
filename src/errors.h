#ifndef __ERRORS_H__
#define __ERRORS_H__

#include "common.h"

void init_errors(FILE* fp);
void syntax(const char* str, ...);
void warning(const char* str, ...);
void fatal_error(const char* str, ...);
void runtime_error(const char* str, ...);
void runtime_warning(const char* str, ...);
void command_error(const char* str, ...);

int get_num_errors();
int get_num_warnings();
void inc_error_count();
void inc_warning_count();
FILE* get_err_stream();

// TODO: Create ASSERT(expr, msg, ...) macro for fatal errors.
void internal_assert(const char* file, const char* func, int line,
                    const char* expr, int expr_val, const char* str, ...);

#define __FILENAME__ (strrchr(__FILE__, '/')? strrchr(__FILE__, '/')+1: __FILE__ )
// FIXME need to accept the missing ...
#define ASSERT(expr, msg, ...) internal_assert(__FILENAME__, __func__, __LINE__, \
                                    #expr, expr, msg, ##__VA_ARGS__)
#define LOC_ASSERT(fi, fu, li, expr, msg, ...) internal_assert(fi, fu, li, \
                                    #expr, expr, msg, ##__VA_ARGS__)
#endif
