/**
 * @file
 * ptr_lists.c
 *
 * This module manages a dynamic list of pointers. The pointers can point to any arbitrary
 * data type and the caller is responsible for freeing the data and making sure that the
 * pointer remains valid through the life of the list.
 */
#include "common.h"

/**
 * Free the list buffer. This is only used when the list will no longer
 * be used, such as when the program ends.
 */
void destroy_u16_list(u16_list_t* list)
{
    log_debug("enter: %p", list);
    if(list != NULL) {
        FREE(list->buffer);
        FREE(list);
    }
    log_debug("leave");
}

/**
 * Initialize a newly created or otherwise existing list.
 */
void init_u16_list(u16_list_t* list) {

    list->capacity = 0x01 << 3;
    list->nitems = 0;
    list->index = 0;
    list->buffer = (uint16_t*)CALLOC(list->capacity, sizeof(uint16_t));
}

/**
 * Initially create the list and initialize the contents to initial values.
 * If the list was in use before this, the buffer will be freed.
 *
 * Size is the size of each item that that will be put in the list.
 */
u16_list_t* create_u16_list(void)
{
    u16_list_t* list;

    list = (u16_list_t*)MALLOC(sizeof(u16_list_t));

    init_u16_list(list);

    return list;
}

/**
 * Store the given item in the given list at the end of the list.
 */
void append_u16_list(u16_list_t* list, uint16_t item)
{
    if(list->nitems + 2 > list->capacity) {
        list->capacity = list->capacity << 1;
        list->buffer = REALLOC(list->buffer, list->capacity * sizeof(void*));
    }

    list->buffer[list->nitems] = item;
    list->nitems++;
}

/**
 * If the index is within the bounds of the list, then return a raw pointer to
 * the element specified. If it is outside the list, or if there is nothing in
 * the list, then return NULL.
 */
uint16_t get_u16_list_by_index(u16_list_t* list, int index)
{
    if(list != NULL)
    {
        if(index >= 0 && index < (int)list->nitems)
        {
            return list->buffer[index];
        }
    }

    return list->buffer[list->nitems-1];  // last item
}

