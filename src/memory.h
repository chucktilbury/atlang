#ifndef __MEMORY_H__
#define __MEMORY_H__

#include "common.h"

#define CALLOC(n, s)    memory_calloc(__FILENAME__, __func__, __LINE__, n, s)
#define MALLOC(s)       memory_malloc(__FILENAME__, __func__, __LINE__, s)
#define REALLOC(p, s)   memory_realloc(__FILENAME__, __func__, __LINE__, p, s)
#define STRDUP(s)       memory_strdup(__FILENAME__, __func__, __LINE__, s)
#define ALLOC_DS(t)     (t*)memory_calloc(__FILENAME__, __func__, __LINE__, 1, sizeof(t))
#define FREE(p)         memory_free(__FILENAME__, __func__, __LINE__, (void*)p)

void init_memory();
void destroy_memory();
void *memory_calloc(const char*, const char*, int, size_t, size_t);
void *memory_malloc(const char*, const char*, int, size_t);
void *memory_realloc(const char*, const char*, int, void*, size_t);
void memory_free(const char*, const char*, int, void*);
char* memory_strdup(const char*, const char*, int, const char*);

#endif
