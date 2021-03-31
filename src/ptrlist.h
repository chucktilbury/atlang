#ifndef __PTR_LISTS_H__
#define __PTR_LISTS_H__

#include "common.h"

/**
 * @brief Structure for a managed array.
 */
typedef struct
{
    int nitems;         // number of items currently in the array
    size_t index;       // current index for iterating the list
    size_t capacity;    // capacity in items
    void** buffer;      // raw buffer where the items are kept
} ptr_list_t;

void init_ptr_list(ptr_list_t*);
ptr_list_t* create_ptr_list();
void destroy_ptr_list(ptr_list_t*);
void append_ptr_list(ptr_list_t*, void*);
void* get_ptr_list_by_index(ptr_list_t*, int);
void* get_ptr_list_next(ptr_list_t*);
void reset_ptr_list(ptr_list_t*);
void push_ptr_list(ptr_list_t*, void*);
void* pop_ptr_list(ptr_list_t*);
void* peek_ptr_list(ptr_list_t*);
int size_ptr_list(ptr_list_t*);

#endif
