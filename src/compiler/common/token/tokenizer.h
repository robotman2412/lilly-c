
// Copyright © 2024, Julian Scheffers
// SPDX-License-Identifier: MIT

#pragma once

#include "frontend.h"
#include "refcount.h"

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>



// Token type.
typedef enum {
    // Keywords.
    TOKENTYPE_KEYWORD,
    // Identifier (variable/label/function/etc name).
    TOKENTYPE_IDENT,
    // Integer constant.
    TOKENTYPE_ICONST,
    // Character constant.
    TOKENTYPE_CCONST,
    // String constant.
    TOKENTYPE_SCONST,
    // Uncategorized legal characters (operators/brackets/etc).
    TOKENTYPE_OTHER,
    // Garbage (illegal characters).
    TOKENTYPE_GARBAGE,
    // End of line (for languages where that is important).
    TOKENTYPE_EOL,
    // End of file.
    TOKENTYPE_EOF,
} tokentype_t;



// Abstract tokenizer handle.
typedef struct tokenizer tokenizer_t;
// Token data.
typedef struct token     token_t;


// Abstract tokenizer handle.
struct tokenizer {
    // Associated frontend context.
    front_ctx_t *fe_ctx;
    // Current source file.
    srcfile_t   *file;
    // Current file position.
    pos_t        pos;
    // Function to call to get next token.
    token_t (*next)(tokenizer_t *ctx);
};

// Token data.
struct token {
    // Token position.
    pos_t       pos;
    // Token type.
    tokentype_t type;
    // Language-specific subtype.
    int         subtype;
    // Refcount ptr of identifier/string/character value.
    rc_t        strval;
    // Integer value.
    uint64_t    ival;
};



// Tests whether a character is a valid hexadecimal constant character ([0-9a-fA-F]).
bool is_hex_char(int c);