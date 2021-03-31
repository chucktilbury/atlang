#ifndef __CONFIGURE_H__
#define __CONFIGURE_H__

#include "common.h"

typedef enum {
    CONFIG_TYPE_NUM,
    CONFIG_TYPE_STR,
    CONFIG_TYPE_BOOL,
    CONFIG_TYPE_LIST,
    CONFIG_TYPE_HELP,
    CONFIG_TYPE_END,
} config_type_t;

typedef struct _configuration {
    char* arg;  // The cmd arg, such as "-a"
    char* name; // Name as it would be found in the environment, such as "NAME"
    char* help; // Help string to print
    config_type_t type; // Type of the value in the union
    char required; // if this is set, the parameter is required
    union {     // union can hold a number or a string
        int number;
        char* string;
        ptr_list_t* list;
    } value;
    char touched;
    char once;
    char* iter_buf; // used for strtok_r()
    char* sav_buf;  // used for strtok_r()
} configuration_t;

#define BEGIN_CONFIG configuration_t _global_config[] = { \
            {.arg="-h", \
             .name="HELP_FLAG", \
             .help="Print the help and exit", \
             .type=CONFIG_TYPE_HELP, \
             .required=0, \
             .value.number=0, \
             .touched=0, \
             .once=0, \
             .iter_buf=NULL, \
             .sav_buf=NULL },

#define END_CONFIG { \
                .arg=NULL, \
                .name=NULL, \
                .help=NULL, \
                .type=CONFIG_TYPE_END, \
                .required=0, \
                .value.number=-1, \
                .touched=0, \
                .once=0, \
                .iter_buf=NULL, \
                .sav_buf=NULL \
            }};

#define CONFIG_NUM(a, n, h, r, v, o) { \
                .arg=(a), \
                .name=(n), \
                .help=(h), \
                .type=CONFIG_TYPE_NUM, \
                .required=(r), \
                .value.number=(v), \
                .touched=0, \
                .once=(o), \
                .iter_buf=NULL, \
                .sav_buf=NULL \
            },

#define CONFIG_STR(a, n, h, r, v, o) { \
                .arg=(a), \
                .name=(n), \
                .help=(h), \
                .type=CONFIG_TYPE_STR, \
                .required=(r), \
                .value.string=(v), \
                .touched=0, \
                .once=(o), \
                .iter_buf=NULL, \
                .sav_buf=NULL \
            },

#define CONFIG_BOOL(a, n, h, r, v, o) { \
                .arg=(a), \
                .name=(n), \
                .help=(h), \
                .type=CONFIG_TYPE_BOOL, \
                .required=(r), \
                .value.number=(v), \
                .touched=0, \
                .once=(o), \
                .iter_buf=NULL, \
                .sav_buf=NULL \
            },

#define CONFIG_LIST(a, n, h, r, v, o) { \
                .arg=(a), \
                .name=(n), \
                .help=(h), \
                .type=CONFIG_TYPE_LIST, \
                .required=(r), \
                .value.string=(v), \
                .touched=0, \
                .once=(o), \
                .iter_buf=NULL, \
                .sav_buf=NULL \
            },

#define GET_CONFIG_NUM(n)   (*(int*)get_config(n))
#define GET_CONFIG_STR(n)   ((char*)get_config(n))
#define GET_CONFIG_BOOL(n)  (*(int*)get_config(n))
#define GET_CONFIG_LIST(n)  ((char*)get_config(n))

// this is defined when the command line parameters are defined.
extern configuration_t _global_config[];

int configure(int argc, char** argv);
void destroy_config(void);
void* get_config(const char* name);
void reset_config_list(const char* name);
char* iterate_config(const char* name);
void show_use(void);
char* get_prog_name(void);

#endif
