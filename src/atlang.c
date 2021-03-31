/**
    @file alang.c

    @brief

**/
#include <readline/readline.h>
#include <readline/history.h>

#include "common.h"

extern VMachine* vm;

// note that longer vars with the same leading letters need to appear before shorter ones.
// parm, envname, help, required, default, once
BEGIN_CONFIG
    CONFIG_NUM("-v", "VERBOSE", "Set the verbosity from 0 to 50", 0, 0, 0)
    CONFIG_STR("-o", "OUTFILE", "Specify the file name to output", 0, "output.bc", 1)
    CONFIG_LIST("-i", "FPATH", "Specify directories to search for imports", 0, ".:include", 0)
    CONFIG_BOOL("-D", "DFILE_ONLY", "Output the dot file only. No object output", 0, 0, 0)
    CONFIG_STR("-d", "DUMP_FILE", "Specify the file name to dump the AST into", 0, "ast_dump.dot", 1)
    CONFIG_LIST(NULL, "INFILES", "List of input files", 0, NULL, 0)
END_CONFIG

static InterpretResult interpret() {

    reset_vmachine();
    // call the compiler to create the code buffer
    // compile reads directly from the scanner
    compile();

    // run the code block, but do not free the VM or destroy the code.
    InterpretResult res = run_vmachine(vm);
    printf("Value = ");
    print_value(peek_value_stack());
    printf("\n");
    return res;
}

static void repl() {

    bool finished = false;

    printf("\natlang v0.1 REPL interface. (Ctrl-D to exit)\n\n");
    rl_bind_key('\t', rl_insert);

    char* buf;
    while(!finished) {
        buf = readline(">> ");
        if(buf != NULL) {
            if(strlen(buf) > 0) {
                add_history(buf);
                // set up the scanner to scan from this string.
                if(!strcmp(buf, ".quit")) {
                    finished = true;
                    continue;
                }
                else {
                    open_scanner_string(buf);
                    interpret();
                }
            }
            free(buf);  // not allocated in memory.c
        }
        else
            finished = true;
    }
    printf("\n");
}

static void init_things(int argc, char** argv) {

    init_memory();
    configure(argc, argv);
    init_errors(stderr);
    init_scanner();
    init_vmachine();
}

static void uninit_things() {

    destroy_config();
    destroy_scanner();
    destroy_vmachine();
    destroy_memory();
}

int main(int argc, char** argv) {

#ifdef _USE_LOGGING
    log_set_level(LOG_DEBUG);
#endif

    log_debug("program start");
    init_things(argc, argv);

    if(get_config("INFILES") == NULL)
        repl();
    else {
        reset_config_list("INFILES");
        for(char* str = iterate_config("INFILES"); str != NULL; str = iterate_config("INFILES")) {
            open_scanner_file(str);
            int retv = interpret();
            if(retv != INTERPRET_OK)
                break;
        }

    }

    int numerr = get_num_errors();
    fprintf(stderr, "\n    errors: %d warnings: %d\n", numerr, get_num_warnings());
    uninit_things();
    log_debug("program end");
    return numerr;
}
