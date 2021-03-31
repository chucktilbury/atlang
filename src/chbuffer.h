#ifndef __CHAR_BUFFER_H__
#define __CHAR_BUFFER_H__

// opaque handle
typedef void* char_buffer_t;

void init_char_buffer(char_buffer_t);
char_buffer_t create_char_buffer();
void destroy_char_buffer(char_buffer_t);
const char* get_char_buffer(char_buffer_t);
void add_char_buffer(char_buffer_t, int);
void add_char_buffer_str(char_buffer_t, const char*);
void add_char_buffer_int(char_buffer_t, int);
void truncate_char_buffer(char_buffer_t, int);
void set_char_buffer_index_str(char_buffer_t, int, const char*);
#endif