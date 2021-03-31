/*
 * This file is included in all unit tests. It provides prototypes and macros
 * that implement the actual tests. These are simple tests that do not capture
 * exceptions and signals. The goal of these tests is to determine that
 * individual functions behave as they should.
 *
 * For the original archive see:
 * https://github.com/chucktilbury/unit_tests
 */
#ifndef _UNIT_TESTS_H_
#define _UNIT_TESTS_H_

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <stdarg.h>

/******************************************************************************
 *  Configuration Parameters
 */
/*
 * 0 = do not use any memory allocation tracking
 * 1 = use memory allocation tracking
 */
#ifndef USE_MEMORY
#define USE_MEMORY 1
#endif

/*
 *  0 = do not use the file IO tracking
 *  1 = Use the file IO tracking
 *
 *  (not supported yet)
 */
#define FILE_IO_USED 0

/*
 *  0 = No test needs to use capture.
 *  1 = Capture is being used by a test.
 */
#ifndef USE_CAPTURE
#define USE_CAPTURE 0
#endif

/*
 *  0 = print summary only
 *  1 = print failures only
 *  2 = print pass and fail messages
 *  3 = start and end messages
 *  4 = or greater enables debugging messages at that level.
 *
 *  Errors are always printed, regardless of verbosity setting.
 */
#ifndef VERBOSE
#define VERBOSE 1
#endif

/*
 *  Maximum number of stubs that can be tracked.
 */
#ifndef MAX_STUBS
#define MAX_STUBS   10
#endif

/*
 *  Maximum number of mocks that can be tracked.
 */
#ifndef MAX_MOCKS
#define MAX_MOCKS   10
#endif

/*
 *  Maximum number of tests that can be defined
 */
#ifndef MAX_TESTS
#define MAX_TESTS   20
#endif

/******************************************************************************
 *  Macros used to implement tests.
 */
/*
 * Stubs have a return type "r" followed by the name, "n", followed by the
 * parameters that the stubbed function accepts.
 */
#define DEF_STUB(r, n, ...) r n(__VA_ARGS__) { \
    unit_stub_entered(#n);
#define END_STUB }
/*
 * If you want to use a stub to see how many times it has been entered, then you
 * need to add it when the test begins. If it is not added then the entry of it
 * will simply not be tracked without repercussions.
 */
#define TRACK_STUB(n)  unit_track_stub(n)

/*
 * Mocks have a return type "r", followed by the name given to the mock, "n",
 * followed by the parameters for the mock. The mock can do anything that would
 * normally be done with function parameters or simply ignored, however, the
 * return value should be returned, unless the return type is void.
 */
#define DEF_MOCK(r, n, ...) \
    r n(__VA_ARGS__) { \
    unit_mock_entered(#n);
#define END_MOCK }

/*
 * If you want to use a mock to see how many times it has been entered, then you
 * need to add it when the test begins. If it is not added then the entry of it
 * will simply not be tracked without repercussions.
 */
#define TRACK_MOCK(n)  unit_track_mock(n)

/*
 * Tests always receieve a pointer to a struct that keeps the result. The struct
 * is created by add_test() and if add_test() is not called before, the test
 * will not be run. The macro parameter "n" is the name to give the test
 * function. It is used in several places, such as to report the results of a
 * test in the suite defined by the module.
 */
#define DEF_TEST(n) void n(test_list_t *test) {
#define END_TEST }
#define ADD_TEST(n) unit_add_test(n, #n)

/*
 * These macros implements the main() of the test. The macro parameter is the
 * display name of the test suite.
 */
#define DEF_TEST_MAIN(n) \
    int main(void) { \
        suite_name = (n); \
        atexit(exit_routine); \
        memset(tests, 0, sizeof(tests)); \
        memset(mocks, 0, sizeof(mocks)); \
        memset(stubs, 0, sizeof(stubs));

#define END_TEST_MAIN \
        return unit_run_all_tests(); \
    }

/******************************************************************************
 *  Print macros add a bunch of information to the print.
 *
 *  One may use unit_error() or unit_msg() in a test, but these are really
 *  intended to be used by the macros that define the testing functionality.
 */
// forward declaration
static inline void unit_print(const char *, int, const char *,
                              const char *, const char *, ...);
#define unit_pass(fmt, ...) \
    do { \
        test->pass ++; \
        if(VERBOSE >= 2) { \
            unit_print(__func__, __LINE__, "PASS", suite_name, fmt, ##__VA_ARGS__);  \
        } \
    } while(0)

#define unit_fail(fmt, ...) \
    do { \
        test->fail ++; \
        if(VERBOSE >= 1) { \
            unit_print(__func__, __LINE__, "FAIL", suite_name, fmt, ##__VA_ARGS__); \
        } \
    } while(0)

#define unit_error(fmt, ...)  \
    do { \
        total_errors ++; \
        unit_print(__func__, __LINE__, "ERROR", suite_name, fmt, ##__VA_ARGS__);  \
    } while(0)

#define unit_msg(level, fmt, ...) \
    do { \
        if(VERBOSE >= level) { \
            unit_print(__func__, __LINE__, "MSG", suite_name, fmt, ##__VA_ARGS__);  \
        } \
    } while(0)

/******************************************************************************
 *  Function definitions for test driver.
 *
 *  These are static to limit what the test has to link with.
 *
 *  None of these should be called directly. They are all referenced by the
 *  macros that implement the testing.
 *
 */
static char *suite_name;
typedef struct test_list {
    const char *name;
    void (*fptr)(struct test_list*);
    int fail;
    int pass;
    struct test_list *next;
} test_list_t;

typedef void (*fptr_t)(test_list_t*);

typedef struct mock_list {
    const char *name;
    int count;
    struct mock_list *next;
} mock_list_t;

static mock_list_t mocks[MAX_MOCKS];
static int mock_idx = 0;
static mock_list_t stubs[MAX_STUBS];
static int stub_idx = 0;
static test_list_t tests[MAX_TESTS];
static int test_idx = 0;

static int total_errors = 0;
static int total_fail = 0;
static int total_pass = 0;

#if USE_MEMORY==1
static unsigned int memory_pool = 0;
static unsigned int total_memory_allocated = 0;

int malloc_count = 0;
int calloc_count = 0;
int free_count = 0;
int realloc_count = 0;
int strdup_count = 0;

void reset_memory_stats(void) {
    malloc_count = 0;
    calloc_count = 0;
    free_count = 0;
    realloc_count = 0;
    strdup_count = 0;
}

#endif

static inline void show_mocks_and_stubs(void) {

    printf("\nMocks:\n");
    for(int i = 0; mocks[i].name != NULL; i++)
        printf("   %s: %d\n", mocks[i].name, mocks[i].count);

    printf("Stubs:\n");
    for(int i = 0; stubs[i].name != NULL; i++)
        printf("   %s: %d\n", stubs[i].name, stubs[i].count);
}

static inline void reset_mocks_and_stubs(void) {

    for(int i = 0; mocks[i].name != NULL; i++)
        mocks[i].count = 0;

    for(int i = 0; stubs[i].name != NULL; i++)
        stubs[i].count = 0;
}

static void exit_routine(void) {

    printf("\n%s: test funcs: %d, pass: %d, fail: %d, errors: %d\n",
           suite_name, test_idx, total_pass, total_fail, total_errors);
    printf("     tests: %d, stubs: %d, mocks: %d\n", test_idx, stub_idx, mock_idx);

#if USE_MEMORY==1
        printf("     memory allocated: %u, memory still in use: %u\n",
               total_memory_allocated, memory_pool);
#endif

    if(VERBOSE > 3) {
        show_mocks_and_stubs();
    }
}

static inline int unit_run_all_tests(void) {

#if USE_MEMORY==1
    total_memory_allocated = 0;
#endif

    for(int i = 0; tests[i].fptr != NULL; i++) {
        reset_mocks_and_stubs();
#if USE_MEMORY == 1
        reset_memory_stats();
        memory_pool = 0;
#endif
        (*tests[i].fptr)(&tests[i]);
        total_pass += tests[i].pass;
        total_fail += tests[i].fail;
        if(VERBOSE > 0) {
            printf("%d. %s: pass: %d, fail: %d\n",
                   i+1, tests[i].name, tests[i].pass, tests[i].fail);
        }
    }
    return total_fail;
}

static inline void unit_add_test(fptr_t test, const char* name) {

    unit_msg(5, "add test name = \"%s\"", name);
    tests[test_idx].fptr = test;
    tests[test_idx].name = name;
    test_idx ++;
}

static inline void unit_track_stub(const char* name) {

    // only add a given name one time
    for(int i = 0; stubs[i].name != NULL; i++) {
        if(stubs[i].name == name) {
            stubs[i].count = 0;
            unit_msg(5, "stub name \"%s\" is already being tracked", name);
            return;
        }
    }
    unit_msg(5, "tracking stub name = \"%s\"", name);
    stubs[stub_idx].name = name;
    stub_idx ++;
}

static inline void unit_track_mock(const char* name) {

    // only add a given name one time
    for(int i = 0; mocks[i].name != NULL; i++) {
        if(mocks[i].name == name) {
            mocks[i].count = 0;
            unit_msg(5, "mock name \"%s\" is already being tracked", name);
            return;
        }
    }
    unit_msg(5, "tracking mock name = \"%s\"", name);
    mocks[mock_idx].name = name;
    mock_idx ++;
}

// These function update a global data structure that can be polled then the
// test has finished running.
static inline void unit_mock_entered(const char* name) {

    unit_msg(5, "mock name = \"%s\"", name);
    for(int i = 0; mocks[i].name != NULL; i++)
        if(!strcmp(mocks[i].name, name)) {
            unit_msg(5, "mock found");
            mocks[i].count++;
            return;
        }
    unit_msg(5, "mock not found");
}

static inline void unit_stub_entered(const char* name) {

    unit_msg(5, "stub name = \"%s\"", name);
    for(int i = 0; stubs[i].name != NULL; i++)
        if(!strcmp(stubs[i].name, name)) {
            unit_msg(5, "stub found");
            stubs[i].count++;
            return;
        }
    unit_msg(5, "stub not found");
    // else simply do nothing if the stub is not in the list
}

static inline int unit_check_mock_entered(const char* name) {

    unit_msg(5, "mock name = \"%s\"", name);
    for(int i = 0; mocks[i].name != NULL; i++)
        if(!strcmp(mocks[i].name, name)) {
            unit_msg(5, "mock found");
            return mocks[i].count;
        }

    unit_msg(5, "mock not found");
    return 0;
}

static inline int unit_check_stub_entered(const char* name) {

    unit_msg(5, "stub name = \"%s\"", name);
    for(int i = 0; stubs[i].name != NULL; i++)
        if(!strcmp(stubs[i].name, name)) {
            unit_msg(5, "stub found");
            return stubs[i].count;
        }
    unit_msg(5, "stub not found");
    return 0;
}

// Generic print routine
static inline void unit_print(const char *preamble,
                              //const char *file_name,
                              int line_no,
                              const char *func_name,
                              const char *sname,
                              const char *fmt, ...) {
    va_list args;

    // TODO: Find a way to print this message without causing malloc() to be
    // called. Malloc() always gets called the first time that printf() gets
    // called.
    //printf("%s: %s: %s: %d: %s: ", preamble, file_name, func_name, line_no, sname);
    printf("%s: %d: %s: %s: ", preamble, line_no, func_name, sname);
    va_start(args, fmt);
    vprintf(fmt, args);
    va_end(args);
    printf("\n");
}

/******************************************************************************
 * Assert macros
 */

/*
 *  "n" is the name of the mock or stub to assert upon.
 */
#define assert_mock_entered_count(v, n) \
    do { \
        int count = unit_check_mock_entered(n); \
        if(count != v) { \
            unit_fail("assert mock entered count \"%s\" expected %d but got %d", n, v, count); \
        } \
        else { \
            unit_pass("assert mock entered \"%s\"", n); \
        } \
    } while(0)

#define assert_mock_entered(n) \
    do { \
        int count = unit_check_mock_entered(n); \
        if(count == 0) { \
            unit_fail("assert mock entered \"%s\"", n); \
        } \
        else { \
            unit_pass("assert mock entered \"%s\"", n); \
        } \
    } while(0)

#define assert_mock_not_entered(n) \
    do { \
        int count = unit_check_mock_entered(n); \
        if(count != 0) { \
            unit_fail("assert mock not entered \"%s\"", n); \
        } \
        else { \
            unit_pass("assert mock not entered \"%s\"", n); \
        } \
    } while(0)

#define assert_stub_entered_count(v, n) \
    do { \
        int count = unit_check_stub_entered(n); \
        if(count != v) { \
            unit_fail("assert stub \"%s\" entered count expected %d but got %d", n, v); \
        } \
        else {\
            unit_pass("assert stub entered \"%s\"", n); \
        } \
    } while(0)

#define assert_stub_entered(n) \
    do { \
        int count = unit_check_stub_entered(n); \
        if(count == 0) { \
            unit_fail("assert stub entered \"%s\"", n); \
        } \
        else {\
            unit_pass("assert stub entered \"%s\"", n); \
        } \
    } while(0)

#define assert_stub_not_entered(n) \
    do { \
        int count = unit_check_stub_entered(n); \
        if(count != 0) { \
            unit_fail("assert stub not entered \"%s\"", n); \
        } \
        else { \
            unit_pass("assert stub not entered \"%s\"", n); \
        } \
    } while(0)


/*
 *  "e" is the expected value
 *  "g" is the "got" value
 *  "p" is the precision for comparing doubles
 */
#define assert_int_equal(e, g) \
    do { \
        if(e != g) { \
            unit_fail("assert int equal expected %d but got %d", e, g); \
        } \
        else { \
            unit_pass("assert int equal"); \
        } \
    } while(0)

#define assert_uint_equal(e, g) \
    do { \
        if(e != g) { \
            unit_fail("assert uint equal expected %u but got %u", e, g); \
        } \
        else {\
            unit_pass("assert uint equal"); \
        } \
    } while(0)

#define assert_string_equal(e, g) \
    do { \
        if(strcmp(e, g)) { \
            unit_fail("assert string equal expected \"%s\" but got \"%s\"", e, g); \
        } \
        else { \
            unit_pass("assert string equal"); \
        } \
    } while(0)

#define assert_string_not_equal(e, g) \
    do { \
        if(!strcmp(e, g)) { \
            unit_fail("assert int not equal expected \"%s\" but got \"%s\"", e, g); \
        } \
        else { \
            unit_pass("assert int not equal"); \
        } \
    } while(0)

#define assert_int_not_equal(e, g) \
    do { \
        if(e == g) { \
            unit_fail("assert int not equal expected %d but got %d", e, g); \
        } \
        else { \
            unit_pass("assert int not equal"); \
        } \
    } while(0)

#define assert_uint_not_equal(e, g) \
    do { \
        if(e == g) { \
            unit_fail("assert uint not equal expected %u but got %u", e, g); \
        } \
        else { \
            unit_pass("assert uint not equal"); \
        } \
    } while(0)

#define assert_double_equal(e, g, p) \
    do { \
        if((e) < (g)-(p) || (e) > (g)+(p)) { \
            unit_fail("assert double equal expected %f but got %f", e, g); \
        } \
        else { \
            unit_pass("assert double equal"); \
        } \
    } while(0)

#define assert_double_not_equal(e, g, p) \
    do { \
        if((e) > (g)-(p) && (e) < (g)+(p)) { \
            unit_fail("assert double not equal expected %f but got %f", e, g); \
        } \
        else { \
            unit_pass("assert double not equal"); \
        } \
    } while(0)

/*
 *  "p" is the pointer to check.
 */
#define assert_ptr_null(p) \
    do { \
        if((p) != (void*)0) { \
            unit_fail("assert pointer is NULL"); \
        } \
        else { \
            unit_pass("assert pointer is NULL"); \
        } \
    } while(0)

#define assert_ptr_not_null(p) \
    do { \
        if((p) == (void*)0) { \
            unit_fail("assert pointer is not NULL"); \
        } \
        else { \
            unit_pass("assert pointer is not NULL"); \
        } \
    } while(0)

/*
 *  "s" is the size of the buffer
 *  "p1" is the expected data
 *  "p2" is the test data
 */
#define assert_buffer_equal(p1, p2, s) \
    do { \
        int val; \
        if(0 != (val = memcmp((p1), (p2), (s)))) { \
            unit_fail("assert buffer equal returns %d", val); \
        } \
        else { \
            unit_pass("assert buffer equal returns %d", val); \
        } \
    } while(0)

#define assert_buffer_not_equal(p1, p2, s) \
    do { \
        int val; \
        if(0 == (val = memcmp((p1), (p2), (s)))) { \
            unit_fail("assert buffer not equal returns %d", val); \
        } \
        else { \
            unit_pass("assert buffer not equal returns %d", val); \
        } \
    } while(0)

/******************************************************************************
 * Memory replacement functions. These track memory used and cause a report to
 * be printed at the end of the test suite. They are simply wrappers for the
 * default memory allocation functions that track a little extra data.
 */
#if USE_MEMORY==1

void *(*old_malloc)(size_t) = malloc;
#define malloc unit_malloc
void *unit_malloc(size_t size) {
    malloc_count ++;
    unit_msg(5, "enter unit_malloc: size = %lu", size);
    void *buf = old_malloc(size+sizeof(size_t));
    *(size_t*)buf = size;
    memory_pool += size;
    total_memory_allocated += size;
    unit_msg(5, "leave unit_malloc returning: %p", buf+sizeof(size_t));
    return buf+sizeof(size_t);
}

void *(*old_calloc)(size_t, size_t) = calloc;
#define calloc unit_calloc
void *unit_calloc(size_t num, size_t size) {
    calloc_count++;
    unit_msg(5, "enter unit_calloc: num = %lu, size = %lu", num, size);
    size_t bsize = num * size;
    size_t asize = bsize+sizeof(size_t);
    void *buf = old_malloc(asize);
    memset(buf, 0, asize);
    *(size_t*)buf = size;
    memory_pool += size;
    total_memory_allocated += size;
    unit_msg(5, "leave unit_calloc returning: %p", buf+sizeof(size_t));
    return buf+sizeof(size_t);
}

void (*old_free)(void*) = free;
#define free unit_free
void unit_free(void *ptr) {
    free_count ++;
    unit_msg(5, "enter unit_free: ptr = %p", ptr);
    size_t *nptr = (size_t*)(ptr-sizeof(size_t));
    size_t size = *nptr;
    old_free(nptr);
    memory_pool -= size;
    unit_msg(5, "leave unit_free");
}

void *(*old_realloc)(void*, size_t size) = realloc;
#define realloc unit_realloc
void *unit_realloc(void *ptr, size_t size) {
    realloc_count ++;
    unit_msg(5, "enter unit_realloc: ptr = %p, size = %lu", ptr, size);
    void *buf = ptr-sizeof(size_t);
    size_t old_size = *(size_t*)buf;
    buf = old_realloc(buf, size+sizeof(size_t));
    memory_pool += size - old_size; // could be negative
    unit_msg(5, "leave unit_realloc returning: %p", buf+sizeof(size_t));
    return buf+sizeof(size_t);
}

char *(*old_strdup)(const char*) = strdup;
#define strdup unit_strdup
char *unit_strdup(const char *str) {
    strdup_count ++;
    unit_msg(5, "enter unit_strdup: ptr = %p", (void*)str);
    size_t len = strlen(str)+1;
    void *buf = old_malloc(len+sizeof(size_t));
    char *retbuf = buf+sizeof(size_t);
    strcpy(retbuf, str);
    *(size_t*)buf = len;
    memory_pool += len;
    unit_msg(5, "leave unit_strdup returning: %p", (void*)retbuf);
    return retbuf;
}

/******************************************************************************
 *  These assert macros for the memory allocation wrappers. These are used to
 *  check how much memory is in use and where the wrapper functions have been
 *  used or not.
 */
#define assert_memory_pool_size(n) \
    do { \
        if((n) != memory_pool) { \
            unit_fail("assert memory pool size. expected %u but got %u", (n), memory_pool); \
        } \
        else { \
            unit_pass("assert memory pool size"); \
        } \
    } while(0)

#define assert_memory_pool_not_zero() \
    do { \
        if(0 == memory_pool) { \
            unit_fail("assert memory pool not zero"); \
        } \
        else { \
            unit_pass("assert memory pool not zero"); \
        } \
    } while(0)

#define assert_memory_pool_zero() \
    do { \
        if(0 != memory_pool) { \
            unit_fail("assert memory pool is zero"); \
        } \
        else { \
            unit_pass("assert memory pool is zero"); \
        } \
    } while(0)

#define assert_memory_total_size(n) \
    do { \
        if((n) != total_memory_allocated) { \
            unit_fail("assert memory total size. expected %u but got %u", (n), total_memory_allocated); \
        } \
        else { \
            unit_pass("assert memory total size"); \
        } \
    } while(0)

#define assert_memory_total_not_zero() \
    do { \
        if((n) != total_memory_allocated) { \
            unit_fail("assert memory total is not zero."); \
        } \
        else { \
            unit_pass("assert memory total is not zero."); \
        } \
    } while(0)

#define assert_memory_total_zero(n) \
    do { \
        if((n) != total_memory_allocated) { \
            unit_fail("assert memory total is zero."); \
        } \
        else { \
            unit_pass("assert memory total is zero"); \
        } \
    } while(0)

/******************************************************************************
 *  memory allocation entry asserts.
 */
#define assert_malloc_entered() \
    do { \
        if(malloc_count == 0) { \
            unit_fail("assert malloc entered."); \
        } \
        else { \
            unit_pass("assert malloc entered."); \
        } \
    } while(0)

#define assert_malloc_not_entered() \
    do { \
        if(malloc_count != 0) { \
            unit_fail("assert malloc not entered."); \
        } \
        else { \
            unit_pass("assert malloc not entered."); \
        } \
    } while(0)

#define assert_malloc_entered_count(v) \
    do { \
        if(malloc_count != v) { \
            unit_fail("assert malloc entered count expected %d but got %d.", v, malloc_count); \
        } \
        else { \
            unit_pass("assert malloc entered count."); \
        } \
    } while(0)

#define assert_calloc_entered() \
    do { \
        if(calloc_count == 0) { \
            unit_fail("assert calloc entered."); \
        } \
        else { \
            unit_pass("assert calloc entered."); \
        } \
    } while(0)

#define assert_calloc_not_entered() \
    do { \
        if(calloc_count != 0) { \
            unit_fail("assert calloc not entered."); \
        } \
        else { \
            unit_pass("assert calloc not entered."); \
        } \
    } while(0)

#define assert_calloc_entered_count(v) \
    do { \
        if(calloc_count != v) { \
            unit_fail("assert calloc entered count expected %d but got %d.", v, calloc_count); \
        } \
        else { \
            unit_pass("assert calloc entered count."); \
        } \
    } while(0)

#define assert_free_entered() \
    do { \
        if(free_count == 0) { \
            unit_fail("assert free entered."); \
        } \
        else { \
            unit_pass("assert free entered."); \
        } \
    } while(0)

#define assert_free_not_entered() \
    do { \
        if(free_count != 0) { \
            unit_fail("assert free not entered."); \
        } \
        else { \
            unit_pass("assert free not entered."); \
        } \
    } while(0)

#define assert_free_entered_count(v) \
    do { \
        if(free_count != v) { \
            unit_fail("assert free entered count expected %d but got %d.", v, free_count); \
        } \
        else { \
            unit_pass("assert free entered count."); \
        } \
    } while(0)

#define assert_realloc_entered() \
    do { \
        if(realloc_count == 0) { \
            unit_fail("assert realloc entered."); \
        } \
        else { \
            unit_pass("assert realloc entered."); \
        } \
    } while(0)

#define assert_realloc_not_entered() \
    do { \
        if(realloc_count != 0) { \
            unit_fail("assert realloc not entered."); \
        } \
        else { \
            unit_pass("assert realloc not entered."); \
        } \
    } while(0)

#define assert_realloc_entered_count(v) \
    do { \
        if(realloc_count != v) { \
            unit_fail("assert realloc entered count expected %d but got %d.", v, realloc_count); \
        } \
        else { \
            unit_pass("assert realloc entered count."); \
        } \
    } while(0)

#define assert_strdup_entered() \
    do { \
        if(strdup_count == 0) { \
            unit_fail("assert strdup entered."); \
        } \
        else { \
            unit_pass("assert strdup entered."); \
        } \
    } while(0)

#define assert_strdup_not_entered() \
    do { \
        if(strdup_count != 0) { \
            unit_fail("assert strdup not entered."); \
        } \
        else { \
            unit_pass("assert strdup not entered."); \
        } \
    } while(0)

#define assert_strdup_entered_count(v) \
    do { \
        if(strdup_count != v) { \
            unit_fail("assert strdup entered count expected %d but got %d.", v, strdup_count); \
        } \
        else { \
            unit_pass("assert strdup entered count."); \
        } \
    } while(0)

#else

/******************************************************************************
 *  If memory allocation is not enabled, then these cause an error to be
 *  published if any of the memory asserts are used in a test.
 */
#define assert_memory_pool_size(n) unit_error("Must enable USE_MEMORY to use memory assertions.")
#define assert_memory_pool_not_zero() unit_error("Must enable USE_MEMORY to use memory assertions.")
#define assert_memory_pool_zero() unit_error("Must enable USE_MEMORY to use memory assertions.")
#define assert_memory_total_size(n) unit_error("Must enable USE_MEMORY to use memory assertions.")
#define assert_memory_total_not_zero() unit_error("Must enable USE_MEMORY to use memory assertions.")
#define assert_memory_total_zero(n) unit_error("Must enable USE_MEMORY to use memory assertions.")
#define assert_malloc_entered()  unit_error("Must enable USE_MEMORY to use memory assertions.")
#define assert_malloc_not_entered()  unit_error("Must enable USE_MEMORY to use memory assertions.")
#define assert_malloc_entered_count(v)  unit_error("Must enable USE_MEMORY to use memory assertions.")
#define assert_calloc_entered()  unit_error("Must enable USE_MEMORY to use memory assertions.")
#define assert_calloc_not_entered()  unit_error("Must enable USE_MEMORY to use memory assertions.")
#define assert_calloc_entered_count(v)  unit_error("Must enable USE_MEMORY to use memory assertions.")
#define assert_free_entered()  unit_error("Must enable USE_MEMORY to use memory assertions.")
#define assert_free_not_entered()  unit_error("Must enable USE_MEMORY to use memory assertions.")
#define assert_free_entered_count(v)  unit_error("Must enable USE_MEMORY to use memory assertions.")
#define assert_realloc_entered()  unit_error("Must enable USE_MEMORY to use memory assertions.")
#define assert_realloc_not_entered()  unit_error("Must enable USE_MEMORY to use memory assertions.")
#define assert_realloc_entered_count(v)  unit_error("Must enable USE_MEMORY to use memory assertions.")
#define assert_strdup_entered()  unit_error("Must enable USE_MEMORY to use memory assertions.")
#define assert_strdup_not_entered()  unit_error("Must enable USE_MEMORY to use memory assertions.")
#define assert_strdup_entered_count(v)  unit_error("Must enable USE_MEMORY to use memory assertions.")


#endif /* USE_MEMORY */

#if USE_CAPTURE == 1
/*
 *  Capture is implemented with setjmp.h. They are used for things like exiting
 *  the function under test when that function does not have a return statement
 *  to exit upon. For example, the fatal_error function in the examples.
 */
#include <setjmp.h>
jmp_buf unit_jbuf;
#define CAPTURE do { if(0 == setjmp(unit_jbuf)) {
#define END_CAPTURE }} while(0);
#define RAISE() do { longjmp(unit_jbuf, 1); } while(0)
#else
#define CAPTURE unit_error("Must enable USE_CAPTURE to use CAPTURE macro.")
#define END_CAPTURE unit_error("Must enable USE_CAPTURE to use CAPTURE macro.")
#define RAISE() unit_error("Must enable USE_CAPTURE to use RAISE macro.")
#endif

#endif /* _UNIT_TESTS_H_ */
