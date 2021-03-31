
/**
 * @file hashtable.c
 * @brief Hash table implementation uses the "open addressing" technique.
 * See https://www.craftinginterpreters.com/hash-tables.html for an
 * example implementation that this is based on.
 *
 * @author Chuck Tilbury
 * @version 0.1
 * @date 2020-10-27
 *
 * @copyright Copyright (c) 2020
 *
 */
#include "common.h"
#include "hashtable.h"

#define TABLE_MAX_LOAD 0.75

/*
 * Return the smaller of the two parameters.
 */
static inline size_t _min(size_t v1, size_t v2)
{
    return (v1 < v2) ? v1 : v2;
}

/*
 * This is a “FNV-1a” hash function. Do not mess with the constants.
 */
static uint32_t make_hash(const char* key)
{
    uint32_t hash = 2166136261u;

    for(int i = 0; i < (int)strlen(key); i++)
    {
        hash ^= key[i];
        hash *= 16777619;
    }

    return (hash);
}

/*
 * If the entry is found, return the slot, if the entry is not found, then the
 * slot returned is where to put the entry. Check the slot's key to tell the
 * difference.
 */
static _table_entry_t* find_slot(_table_entry_t * ent, size_t cap, const char* key)
{
    uint32_t index = make_hash(key) & (cap - 1);

    while(1)
    {
        _table_entry_t* entry = &ent[index];

        // depends on left evaluate before right
        if((entry->key == NULL) || (!strcmp(key, entry->key)))
        {
            return (entry);
        }

        index = (index + 1) & (cap - 1);
    }
    return (NULL);
}

/**
 * Grow the table if it needs it. Since the hash values change when the table
 * size changes, this function simply re-adds them to the new table, then
 * updates the data structure.
 */
static void grow_table(hashtable_t * tab)
{
    if(tab->count + 2 > tab->capacity * TABLE_MAX_LOAD)
    {
        // table must always be an even power of 2 for this to work.
        size_t capacity = tab->capacity << 1;

        _table_entry_t* entries = (_table_entry_t *) CALLOC(capacity, sizeof(_table_entry_t));

        // re-add the table entries to the new table.
        if(tab->entries != NULL)
        {
            for(int i = 0; i < (int)tab->capacity; i++)
            {
                if(tab->entries[i].key != NULL)
                {
                    _table_entry_t* ent = find_slot(entries, capacity, tab->entries[i].key);

                    // if the key is the same, (i.e. not NULL) the replace the data. There
                    // can be no duplicate entries. No need to check it.
                    ent->key = tab->entries[i].key;
                    ent->size = tab->entries[i].size;
                    ent->data = tab->entries[i].data;
                }
            }
            // free the old table
            free(tab->entries);
        }

        tab->entries = entries;
        tab->capacity = capacity;
    }
}

/**
 * @brief Create a hash table object
 *
 * @return hashtable_t* -- pointer to the allocated memory.
 */
hashtable_t* create_hash_table(void)
{
    hashtable_t* tab;

    tab = malloc(sizeof(hashtable_t));

    tab->capacity = 0x01 << 3;
    tab->entries = (_table_entry_t *) CALLOC(tab->capacity, sizeof(_table_entry_t));
    return (tab);
}

/**
 * @brief Destroy the hash table and free all memory associated with it.
 *
 * @param tab -- Pointer to the table to destroy.
 */
void destroy_hash_table(hashtable_t * tab)
{
    if(tab != NULL)
    {
        if(tab->entries != NULL)
        {
            for(int i = 0; i < (int)tab->capacity; i++)
            {
                if(tab->entries[i].data != NULL)
                    FREE(tab->entries[i].data);
                if(tab->entries[i].key != NULL)
                    FREE((void *)tab->entries[i].key);
            }
            FREE(tab->entries);
        }
        FREE(tab);
    }
}

/**
 * @brief Insert an entry into the hash table. This function refuses to replace an
 * entry and returns an error code.
 *
 * @param tab -- Hash table to place the entry into.
 * @param key -- String that will be used to place the hash.
 * @param data -- Pointer to the data to store in the table.
 * @param size -- Size of the data to store in the table.
 * @return int -- Indicate whether the data was stored or not.
 */
hash_retv_t insert_hash(hashtable_t * tab, const char* key, void* data, size_t size)
{
    grow_table(tab);

    _table_entry_t* entry = find_slot(tab->entries, tab->capacity, key);
    int retv = (entry->key == NULL) ? HASH_NO_ERROR : HASH_EXIST;

    if(retv == HASH_NO_ERROR)
    {
        entry->key = STRDUP(key);
        entry->data = MALLOC(size);
        memcpy(entry->data, data, size);
        entry->size = size;
        tab->count++;
    }

    return (retv);
}

/**
 * @brief Replace the data that is stored in the hash table.
 *
 */
hash_retv_t replace_hash_data(hashtable_t * tab, const char* key, void* data, size_t size) {

    _table_entry_t* entry = find_slot(tab->entries, tab->capacity, key);
    int retv = HASH_NO_ERROR;

    if(entry->key != NULL) {
        if(entry->data != NULL)
            FREE(entry->data);
        entry->data = MALLOC(size);
        memcpy(entry->data, data, size);
        entry->size = size;
    }
    else
        retv = HASH_NOT_FOUND;

    return (retv);
}

/**
 * @brief Find the has table entry. If the entry does not exist, then an error
 * is returned and the value that the data parameter points to is undefined. If
 * HASH_NO_ERROR is returned, then the location that the data parameter points
 * to has been filled in with the data that was stored in the table.
 *
 * @param tab -- The table to search.
 * @param key -- The string to derive the hash from.
 * @param data -- Pointer to where the data is to be copied to.
 * @param size -- Number of bytes to copy for the data.
 * @return int -- Indicate whether there was an error or not.
 */
hash_retv_t find_hash(hashtable_t * tab, const char* key, void* data, size_t size)
{
    _table_entry_t* entry = find_slot(tab->entries, tab->capacity, key);
    int retv = HASH_NO_ERROR;

    if(entry->key != NULL)
    {
        if(entry->data != NULL)
        {
            memcpy(data, entry->data, _min(size, entry->size));
        }
    }
    else
        retv = HASH_NOT_FOUND;

    return (retv);
}

/**
 * @brief Iterate all of the keys in the hash table. Used for various
 * dump utilities. Make reset != 0 to reset to the beginning of the table.
 * This only returns valid keys and not empty slots.
 *
 * @param tab -- The specific table to dump.
 * @param reset -- Set to !0 to reset the index to the beginning of the table.
 * @return const char* -- The key that is stored at the hash location.
 */
const char* iterate_hash_table(hashtable_t * tab, int reset)
{
    static int ht_index = -1;

    if(reset)
        ht_index = -1;

    for(ht_index++; ht_index < (int)tab->capacity; ht_index++)
    {
        if(tab->entries[ht_index].key != NULL)
        {
            return (tab->entries[ht_index].key);
        }
    }

    return (NULL);
}
