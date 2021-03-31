#ifndef __HASHTABLE_H__
#define __HASHTABLE_H__
/**
 * @file hashtable.h
 * @brief Definitions and prototypes for the hash_table.c file.
 *
 * @author Chuck Tilbury
 * @version 0.1
 * @date 2020-10-31
 *
 * @copyright Copyright (c) 2020
 *
 */

#include "common.h"

typedef enum {
    HASH_NO_ERROR,
    HASH_EXIST,
    HASH_NOT_FOUND,
    HASH_NO_DATA,
    HASH_DATA_SIZE,
} hash_retv_t;

typedef struct {
    const char* key;
    size_t size;
    void* data;
} _table_entry_t;

typedef struct {
    size_t count;
    size_t capacity;
    _table_entry_t* entries;
} hashtable_t;

hashtable_t* create_hash_table(void);
void destroy_hash_table(hashtable_t*);
hash_retv_t insert_hash(hashtable_t*, const char*, void*, size_t);
hash_retv_t find_hash(hashtable_t*, const char*, void*, size_t);
hash_retv_t replace_hash_data(hashtable_t*, const char*, void*, size_t);
const char* iterate_hash_table(hashtable_t*, int);

#endif
