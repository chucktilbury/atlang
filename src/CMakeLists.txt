project(at)

#set(CMAKE_VERBOSE_MAKEFILE ON)
add_executable(${PROJECT_NAME}
    atlang.c
    log.c
    scanner.c
    memory.c
    hashtable.c
    files.c
    errors.c
    configure.c
    chbuffer.c
    u16list.c
    ptrlist.c
    codeblocks.c
    disassembler.c
    vmachine.c
    compiler.c
    expression.c
    object.c
)

target_link_libraries(${PROJECT_NAME}
    readline
    m
)

target_include_directories(${PROJECT_NAME}
    PUBLIC
        ${PROJECT_SOURCE_DIR}/../include
)

target_compile_options(${PROJECT_NAME}
    PRIVATE "-Wall" "-Wextra" "-g" "-D_USE_LOGGING" "--std=c99"
    )
