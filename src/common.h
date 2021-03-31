#ifndef __COMMON_H__
#define __COMMON_H__

#define DEBUG_TRACE_EXECUTION
#define DEBUG_PRINT_CODE

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <ctype.h>
#include <errno.h>
#include <assert.h>

#include "log.h"
#include "errors.h"
#include "memory.h"
#include "ptrlist.h"
#include "u16list.h"
#include "chbuffer.h"
#include "files.h"
#include "hashtable.h"

#include "configure.h"

#include "scanner.h"
#include "compiler.h"
#include "codeblocks.h"
#include "object.h"
#include "vmachine.h"
#include "disassembler.h"

#define MIN(v1, v2) (((v1) <= (v2))? (v1): (v2))
#define MAX(v1, v2) (((v1) >= (v2))? (v1): (v2))

#endif
