#ifndef __U16_LISTS_H__
#define __U16_LISTS_H__
#include <stdint.h>
#include <stdlib.h>

/**
 * @brief Structure for a managed array.
 */
typedef struct
{
    size_t nitems;      // number of items currently in the array
    size_t index;       // current index for iterating the list
    size_t capacity;    // capacity in items
    uint16_t* buffer;   // raw buffer where the items are kept
} u16_list_t;

void init_u16_list(u16_list_t* list);
u16_list_t* create_u16_list(void);
void destroy_u16_list(u16_list_t* array);
void append_u16_list(u16_list_t* array, uint16_t item);
uint16_t get_u16_list_by_index(u16_list_t* array, int index);

#endif
