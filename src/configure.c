/*
 * Stand-alone command line parser.
 *
 * Command parameters have the format of "-x arg". In this, 'x' can be any letter or number
 * and they are case sensitive. Command switches are exactly 2 characters and may NOT be
 * combined. In other words, the arg "-xasc12" is parsed as "-x", "asc12". This is not
 * optimal, and may change. It would be better to have command args any arbitrary length,
 * but that is not easy to fix with this implementation.
 *
 */

// TODO Only look at the parameter for the length of it, so that the value can be
// concatenated to it.

#include "common.h"

//static char cmd_line_buffer[1024*4];
static char prog_name[1024];

static configuration_t* find_config_by_arg(const char* arg) {

    for(int i = 0; _global_config[i].type != CONFIG_TYPE_END; i++) {
        if(_global_config[i].arg == NULL)
            continue;
        else if(!strncmp(arg, _global_config[i].arg, strlen(_global_config[i].arg))) {
            return &_global_config[i];
        }
    }

    return NULL;
}

static configuration_t* find_config_by_name(const char* name) {

    for(int i = 0; _global_config[i].type != CONFIG_TYPE_END; i++) {
        if(_global_config[i].name == NULL)
            continue;
        else if(!strcmp(name, _global_config[i].name)) {
            return &_global_config[i];
        }
    }

    return NULL;
}

// aborts program if required parameter is not found
static void check_required(void) {

    int hlp = 0;

    for(int i = 0; _global_config[i].type != CONFIG_TYPE_END; i++) {
        if(_global_config[i].required != 0 && _global_config[i].touched == 0) {
            fprintf(stderr, "CMD ERROR: required parameter \"%s\" (%s) is missing\n",
                        _global_config[i].arg? _global_config[i].arg: "none",
                        _global_config[i].name);
            show_use(); // does not return
        }

        if(_global_config[i].touched > 1 && _global_config[i].once) {
            fprintf(stderr, "CMD_ERROR: parameter \"%s\" (%s) can appear only once\n",
                        _global_config[i].arg? _global_config[i].arg: "none",
                        _global_config[i].name);
            show_use(); // does not return
        }

        if(_global_config[i].type == CONFIG_TYPE_HELP && _global_config[i].value.number)
            hlp++;

    }

    if(hlp)
        show_use();
}

void destroy_config(void) {

    log_debug("destroy_config: enter");
    for(int i = 0; _global_config[i].type != CONFIG_TYPE_END; i++) {
        switch(_global_config[i].type) {
            case CONFIG_TYPE_STR: {
                //fprintf(stderr, "CFG: string: %s: %s\n", _global_config[i].name, _global_config[i].value.string);
                FREE(_global_config[i].value.string);
            }
                                break;

            case CONFIG_TYPE_LIST: {
                //fprintf(stderr, "CFG: list: %s\n", _global_config[i].name);
                ptr_list_t* lst = _global_config[i].value.list;
                reset_ptr_list(lst);
                for(void* item = get_ptr_list_next(lst); item != NULL; item = get_ptr_list_next(lst)) {
                    //fprintf(stderr, "CFG: list item: %s\n", (char*)item);
                    FREE(item);
                }
                destroy_ptr_list(lst);
            }
                                 break;

            case CONFIG_TYPE_BOOL:
            case CONFIG_TYPE_NUM:
            case CONFIG_TYPE_HELP:
            case CONFIG_TYPE_END:
                break;  // do nothing, no memory allocated

            default:
                fprintf(stderr, "CFG ERROR: unknown configuration type (hello weeds)\n");
                exit(1);
        }
    }
    log_debug("destroy_config: leave");
}

static void init_config(void) {

    for(int i = 0; _global_config[i].name != NULL; i++) {
        if(_global_config[i].type == CONFIG_TYPE_LIST) {
            if(_global_config[i].value.string != NULL) {
                // save the default value
                ptr_list_t* list = create_ptr_list();
                char* ptr = STRDUP(_global_config[i].value.string);

                if(strchr(ptr, ':')) {
                    // parse through a list
                    for(char* tmp = strtok(ptr, ":"); tmp != NULL; tmp = strtok(NULL, ":"))
                        append_ptr_list(list, STRDUP(tmp));
                    FREE(ptr);
                }
                else
                    append_ptr_list(list, ptr);

                _global_config[i].value.list = list;
            }
            else
                _global_config[i].value.list = create_ptr_list();
        }
        else if(_global_config[i].type == CONFIG_TYPE_STR) {
            // this will be a literal string, so allocate it to allow us to destroy it later.
            if(_global_config[i].value.string != NULL) {
                char* ptr = STRDUP(_global_config[i].value.string);
                _global_config[i].value.string = ptr;
            }
        }
    }
    //atexit(destroy_config);
}

int configure(int argc, char** argv) {

    configuration_t* config;
    int idx;

    init_config();

    strncpy(prog_name, argv[0], sizeof(prog_name));
    for(idx = 1; idx < argc; idx++) {
        config = find_config_by_arg(argv[idx]);
        if(config == NULL) {
            if(argv[idx][0] == '-') {
                fprintf(stderr, "CMD ERROR: Unknown configuration parameter: \"%s\"\n", argv[idx]);
                show_use(); // does not return
            }
            else {
                // it's an input file
                config = find_config_by_name("INFILES");
                const char* ptr = STRDUP(argv[idx]);
                append_ptr_list(config->value.list, (void*)ptr);
                config->touched++;
                continue;
            }
        }

        switch(config->type) {
            case CONFIG_TYPE_NUM: {
                    char* value;
                    if(strlen(argv[idx]) > strlen(config->arg))
                        value = &argv[idx][strlen(config->arg)];
                    else {
                        idx++;
                        value = argv[idx];
                    }

                    int num = (int)strtol(value, NULL, 0);
                    if(num == 0 && errno != 0) {
                        fprintf(stderr, "CMD ERROR: Cannot convert string \"%s\" to a number\n", argv[idx]);
                        show_use();
                    }
                    config->value.number = num;
                    config->touched++;
                }
                break;

            case CONFIG_TYPE_LIST: {
                    char* value;
                    if(strlen(argv[idx]) > strlen(config->arg))
                        value = &argv[idx][strlen(config->arg)];
                    else {
                        idx++;
                        value = argv[idx];
                    }

                    char* ptr = STRDUP(value);
                    if(strchr(ptr, ':')) {
                        // parse through a list
                        for(char* tmp = strtok(ptr, ":"); tmp != NULL; tmp = strtok(NULL, ":"))
                            append_ptr_list(config->value.list, STRDUP(tmp));
                        FREE(ptr);
                    }
                    else
                        append_ptr_list(config->value.list, (void*)ptr);
                    config->touched++;
                }
                break;

            case CONFIG_TYPE_STR: {
                    char* value;
                    if(strlen(argv[idx]) > strlen(config->arg))
                        value = &argv[idx][strlen(config->arg)];
                    else {
                        idx++;
                        value = argv[idx];
                    }

                    config->value.string = STRDUP(value);
                    config->touched++;
                }
                break;

            case CONFIG_TYPE_HELP:
            case CONFIG_TYPE_BOOL:
                config->value.number = (config->value.number & 0x01) ^ 0x01;
                config->touched++;
                break;

            default:
                fprintf(stderr, "CFG ERROR: Unexpected configuration type (hello weeds)\n");
                exit(1);
        }
    }

    check_required();
    return idx;
}

char* get_prog_name(void) {
    return prog_name;
}

void* get_config(const char* name) {

    void* retv = NULL;

    if(!strcmp(name, "PROG_NAME"))
        retv = (void*)prog_name;
    else {
        configuration_t* config = find_config_by_name(name);
        if(config == NULL) {
            fprintf(stderr, "CFG ERROR: Cannot find configuration item: \"%s\"\n", name);
            exit(1);
        }

        switch(config->type) {
            case CONFIG_TYPE_NUM:
            case CONFIG_TYPE_BOOL:
                retv = (void*)&config->value.number;
                break;

            case CONFIG_TYPE_STR:
                retv = (void*)config->value.string;
                break;

            case CONFIG_TYPE_LIST:
                if(config->value.list->nitems == 0)
                    retv = NULL;
                else
                    retv = (void*)config->value.list->buffer;
                break;

            default:
                fprintf(stderr, "CFG ERROR: Unknown config type\n");
                exit(1);
        }
    }
    return retv;
}

// given the parameter name, iterate the list. Returns any parameter by a string ptr,
// including a number. Returns NULL when the end is reached.
char* iterate_config(const char* name) {

    char* retv = NULL;
    configuration_t* config = find_config_by_name(name);
    if(config == NULL) {
        fprintf(stderr, "CFG ERROR: Cannot find configuration parameter: \"%s\"\n", name);
        exit(1);
    }

    switch(config->type) {
        case CONFIG_TYPE_NUM:
        case CONFIG_TYPE_BOOL:
            // there can be only exactly one value here. Allocate the buffer if it does not exist
            // and then return it.
            if(config->iter_buf == NULL) {
                config->iter_buf = CALLOC(12, sizeof(char)); // maximum number of characters in an int.
                snprintf(config->iter_buf, 12, "%d", config->value.number);
            }
            else {
                FREE(config->iter_buf);
                config->iter_buf = NULL;
            }
            retv = config->iter_buf;
            break;

        case CONFIG_TYPE_STR:
            // there can be only exactly one value here. It's already a (char*).
            if(config->iter_buf == NULL)
                config->iter_buf = config->value.string;
            else
                config->iter_buf = NULL;
            retv = config->iter_buf;
            break;

        case CONFIG_TYPE_LIST:
            // This type actually gets iterated. use strtok_r() to iterate it.
            if(config->iter_buf != NULL) {
                config->iter_buf = get_ptr_list_next(config->value.list);
                retv = config->iter_buf;
            }
            else {
                reset_ptr_list(config->value.list);
                config->iter_buf = get_ptr_list_next(config->value.list);
                retv = config->iter_buf;
            }
            break;

        default:
            fprintf(stderr, "CFG ERROR: Cannot iterate configuration parameter\n");
            exit(1);
    }
    return retv;
}

// call this before starting an iteration
void reset_config_list(const char* name) {

    configuration_t* config = find_config_by_name(name);
    if(config->type == CONFIG_TYPE_LIST)
        reset_ptr_list(config->value.list);
    // else just do nothing
}

void show_use(void) {

    fprintf(stderr, "Use: %s <parameters> <file list>\n", prog_name);
    for(int i = 0; _global_config[i].type != CONFIG_TYPE_END; i++) {
        switch(_global_config[i].type) {
            case CONFIG_TYPE_NUM:
                fprintf(stderr, "\n  %2s <num>  %s\n",
                        _global_config[i].arg != NULL? _global_config[i].arg: "",
                        _global_config[i].help);
                fprintf(stderr, "     val: %d required: %s\n",
                        _global_config[i].value.number,
                        _global_config[i].required? "TRUE": "FALSE");
                break;

            case CONFIG_TYPE_STR:
                fprintf(stderr, "\n  %2s <str>  %s\n",
                        _global_config[i].arg != NULL? _global_config[i].arg: "",
                        _global_config[i].help);
                fprintf(stderr, "     val: %s required: %s\n",
                        _global_config[i].value.string,
                        _global_config[i].required? "TRUE": "FALSE");
                break;

            case CONFIG_TYPE_BOOL:
            case CONFIG_TYPE_HELP:
                fprintf(stderr, "\n  %2s <bool> %s\n",
                        _global_config[i].arg != NULL? _global_config[i].arg: "",
                        _global_config[i].help);
                fprintf(stderr, "     val: %s required: %s\n",
                        _global_config[i].value.number? "TRUE": "FALSE",
                        _global_config[i].required? "TRUE": "FALSE");

                break;

            case CONFIG_TYPE_LIST:
                fprintf(stderr, "\n  %2s <list> %s\n",
                        _global_config[i].arg != NULL? _global_config[i].arg: "",
                        _global_config[i].help);

                fprintf(stderr, "     required: %s\n", _global_config[i].required? "TRUE": "FALSE");

                if(_global_config[i].value.list->nitems != 0) {
                    ptr_list_t* list = _global_config[i].value.list;
                    fprintf(stderr, "     ");
                    for(char* ptr = get_ptr_list_next(list); ptr != NULL; ptr = get_ptr_list_next(list))
                        fprintf(stderr, "%s ", ptr);
                    fprintf(stderr, "\n");
                }
                else
                    fprintf(stderr, "     <empty>\n");
                break;

            default:
                fprintf(stderr, "INTERNAL ERROR: Cannot parse command line\n");
                exit(1);

        }
    }
    fprintf(stderr, "\n");
    exit(1);
}

#ifdef __TESTING_CONFIGURE_C__

/*
 * Sanity check this functionality.
 *
 * example:
 * ./cfg -o filename.fn -i f1.c:f2.c:f3.c -v 10
 *    -or-
 * ./cfg -ofilename.fn -if1.c:f2.c:f3.c -v10
 *
 */

BEGIN_CONFIG
    CONFIG_NUM("-v", "VERBOSE", "Control the verbosity", 0, 0)
    CONFIG_STR("-o", "OUT_FILE", "Name the output file", 1, NULL)
    CONFIG_BOOL("-boo", "BOOL_VAL", "Demostrate a boolean value", 0, 0)
    CONFIG_LIST("-i", "INPUT_FILES", "List of files to be compiled", 1, NULL)
    CONFIG_LIST("-s", "FILE_SEARCH", "Set the search path list", 0, "./:./include")
END_CONFIG

int main(int argc, char** argv) {

    int n = configure(argc, argv);
    printf("num_parms: %d\n", n);
    printf("verbosity: %d\n", *(int*)get_config("VERBOSE"));
    printf("file_search: %s\n", (char*)get_config("FILE_SEARCH"));
    printf("file_list: %s\n", (char*)get_config("INPUT_FILES"));
    printf("out file: %s\n", (char*)get_config("OUT_FILE"));
    printf("prog_name: %s\n", (char*)get_config("PROG_NAME"));
    printf("bool_val: %d\n", GET_CONFIG_BOOL("BOOL_VAL"));

    printf("\nVERBOSE: ");
    for(char* str = iterate_config("VERBOSE"); str != NULL; str = iterate_config("VERBOSE"))
        printf("%s ", str);
    printf("\n");

    printf("FILE_SEARCH: ");
    for(char* str = iterate_config("FILE_SEARCH"); str != NULL; str = iterate_config("FILE_SEARCH"))
        printf("%s ", str);
    printf("\n");

    printf("INPUT_FILES: ");
    for(char* str = iterate_config("INPUT_FILES"); str != NULL; str = iterate_config("INPUT_FILES"))
        printf("%s ", str);
    printf("\n");

    printf("OUT_FILE: ");
    for(char* str = iterate_config("OUT_FILE"); str != NULL; str = iterate_config("OUT_FILE"))
        printf("%s ", str);
    printf("\n");

    printf("BOOL_VAL: ");
    for(char* str = iterate_config("BOOL_VAL"); str != NULL; str = iterate_config("BOOL_VAL"))
        printf("%s ", str);
    printf("\n");

    //show_use();
}

#endif
