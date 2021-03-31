/**
    @file memory.c

    This is a simple wrapper around the memory allocation routines to make
    error handling easier. All memory allocation errors are fatal errors.

    I considered adding garbage collection to this compiler. But I kept running
    into the question of why? This program runs one time and when it ends, the
    memory it has allocated is freed by the system. Any memory leak would have
    to be catastrophic to matter. So, the runtime of the output will have GC,
    but not the compiler.

    What this code does need is a way to check if proper cleanup is happening.
    The reason that is needed is to verify that allocated memory is actually
    getting used for something. A little like checking for unused parameters to
    a function, but for allocated memory. (low priority to implement)

    To that end, a list of allocate memory needs to be kept, along with file
    names and line numbers so that when the program ends, what memory that has
    been allocated, but not freed, can be dumped.

    Reference links:
    https://maplant.com/gc.html
    https://github.com/mkirchner/gc

*/
#include "common.h"

static uint64_t mem_segment;
#define SEG_MASK 0xFFFF00000000
#define GET_SEG(p) (((uint64_t)p)&SEG_MASK)

void init_memory() {

    // int stackvar;
    void* ptr = malloc(1);
    // fprintf(stderr, "stk=%p, mal=%p, glo=%p\n", &stackvar, ptr, &mem_segment);
    mem_segment = GET_SEG(ptr);
    free(ptr);
}

void destroy_memory() {}

void *memory_calloc(const char* file, const char* func, int line, size_t num, size_t size) {

    void* ptr = calloc(num, size);
    LOC_ASSERT(file, func, line, ptr != NULL, "cannot calloc %lu bytes\n", num*size);
    return ptr;
}

void *memory_malloc(const char* file, const char* func, int line, size_t size) {

    void* ptr = malloc(size);
    LOC_ASSERT(file, func, line, ptr != NULL, "cannot malloc %lu bytes\n", size);

    return ptr;
}

void *memory_realloc(const char* file, const char* func, int line, void* ptr, size_t size) {

    void* nptr = realloc(ptr, size);
    LOC_ASSERT(file, func, line, nptr != NULL, "cannot reallocate %lu bytes\n", size);

    return nptr;
}

void memory_free(const char* file, const char* func, int line, void* ptr) {

    log_debug("enter %s:%d: = %p", func, line, ptr);
    if(mem_segment != GET_SEG(ptr))
        fatal_error("%s: %s:%d assert failed: Attempt to free a pointer that was not allocated: %p\n", file, func, line, ptr);
    else
        free(ptr);
    log_debug("leave");
}

// FIXME
// I have absolutely no idea why this definition is not being seen by the compiler.
extern char* strdup(const char*);
char* memory_strdup(const char* file, const char* func, int line, const char* str) {

    char* nptr = strdup(str);
    LOC_ASSERT(file, func, line, nptr != NULL, "cannot strdup %lu bytes\n", strlen(str));

    return nptr;
}
