
#include "common.h"


int file_exists(const char* fname) {

    FILE* fp;
    if((fp = fopen(fname, "r")) != NULL) {
        fclose(fp);
        return 1;
    }

    return 0;
}

static char* search_env_path(char* name) {

    char* tmp = getenv("G_INCLUDE");

    if(tmp == NULL)
        return NULL;

    char* buf = MALLOC(256);    // max path length under linux is 255
    char* raw = STRDUP(tmp);   // strtok destroys the string it parses

    for(char* p = strtok(raw, ":") ; p!= NULL; p = strtok(NULL, ":")) {
        strcpy(buf, p);
        strcat(buf, "/");
        strcat(buf, name);
        if(file_exists(buf)) {
            FREE(raw);
            return buf; // caller must free this
        }
    }

    FREE(raw);
    FREE(buf);
    return NULL;
}

static char* search_cmd_path(char* name) {

    char* ptr;
    char* buf = MALLOC(256);

    reset_config_list("FPATH");
    for(ptr = iterate_config("FPATH"
    ); ptr != NULL; ptr = iterate_config("FPATH")) {
        strcpy(buf, ptr);
        strcat(buf, "/");
        strcat(buf, name);
        if(file_exists(buf)) {
            return buf; // caller must free this
        }
    }

    FREE(buf);
    return NULL;
}



//char* find_import_file(const char* base) {
void open_input_file(const char* base) {

    char* name;
    char* tmp = NULL;

    //memset(fn_buf, 0, sizeof(fn_buf));

    name = MALLOC(256);

    // add the file extention if it's not present
    strncpy(name, base, 252);
    tmp = strrchr(name, '.');
    if(tmp != NULL) {
        if(strcmp(tmp, ".g")) {
            warning("unknown file extention: '%s'", tmp);
            strcat(name, ".g");
        }
        else
            warning("do not include the file extention for import names");
    }
    else
        strcat(name, ".g");

    if(NULL == (tmp = search_cmd_path(name))) {
        if(NULL == (tmp = search_env_path(name))) {
            tmp = NULL; // for illustration
        }
    }

    open_scanner_file(tmp);
    FREE(name);
    FREE(tmp);
}
