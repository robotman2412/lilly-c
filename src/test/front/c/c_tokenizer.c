
// Copyright © 2024, Julian Scheffers
// SPDX-License-Identifier: MIT

#include "token/c_tokenizer.h"

#include "testcase.h"



// Simple test of the various tokens.
static char const *tkn_basic() {
    // clang-format off
    char const data[] =
    "for int\n"
    "0x1000\n"
    "++ --\n"
    "// Line comment things\n"
    "/*\n"
    "Block comment things\n"
    "*/\n"
    "an_identifier forauxiliary_identifier\n"
    "\"\\x00\\000\\x0\\x0000\\770\\377\\00\\0\"\n"
    "\'A\'\n"
    "\"" "\\\'\\\"\\?\\\\\\a\\b\\f\\n\\r\\t\\v" "\"\n"
    ;
    // clang-format on

    front_ctx_t *fe_ctx = front_create();
    srcfile_t   *src    = srcfile_create(fe_ctx, "<tkn_basic>", data, sizeof(data) - 1);

    tokenizer_t tkn_ctx = {
        .file = src,
        .pos = {
            .srcfile = src,
        },
    };
    token_t tkn;


    tkn = c_tkn_next(&tkn_ctx); // for
    EXPECT_INT(tkn.pos.line, 0);
    EXPECT_INT(tkn.pos.col, 0);
    EXPECT_INT(tkn.pos.len, 3);
    EXPECT_INT(tkn.type, TOKENTYPE_KEYWORD);
    EXPECT_INT(tkn.subtype, C_KEYW_for);

    tkn = c_tkn_next(&tkn_ctx); // int
    EXPECT_INT(tkn.pos.line, 0);
    EXPECT_INT(tkn.pos.col, 4);
    EXPECT_INT(tkn.pos.len, 3);
    EXPECT_INT(tkn.type, TOKENTYPE_KEYWORD);
    EXPECT_INT(tkn.subtype, C_KEYW_int);

    tkn = c_tkn_next(&tkn_ctx); // 0x1000
    EXPECT_INT(tkn.pos.line, 1);
    EXPECT_INT(tkn.pos.col, 0);
    EXPECT_INT(tkn.pos.len, 6);
    EXPECT_INT(tkn.type, TOKENTYPE_ICONST);
    EXPECT_INT(tkn.ival, 0x1000);

    tkn = c_tkn_next(&tkn_ctx); // ++
    EXPECT_INT(tkn.pos.line, 2);
    EXPECT_INT(tkn.pos.col, 0);
    EXPECT_INT(tkn.pos.len, 2);
    EXPECT_INT(tkn.type, TOKENTYPE_OTHER);
    EXPECT_INT(tkn.subtype, C_TKN_INC);

    tkn = c_tkn_next(&tkn_ctx); // --
    EXPECT_INT(tkn.pos.line, 2);
    EXPECT_INT(tkn.pos.col, 3);
    EXPECT_INT(tkn.pos.len, 2);
    EXPECT_INT(tkn.type, TOKENTYPE_OTHER);
    EXPECT_INT(tkn.subtype, C_TKN_DEC);

    tkn = c_tkn_next(&tkn_ctx); // an_identifier
    EXPECT_INT(tkn.pos.line, 7);
    EXPECT_INT(tkn.pos.col, 0);
    EXPECT_INT(tkn.pos.len, 13);
    EXPECT_INT(tkn.type, TOKENTYPE_IDENT);
    EXPECT_STR(tkn.strval->data, "an_identifier");

    tkn = c_tkn_next(&tkn_ctx); // forauxiliary_identifier
    EXPECT_INT(tkn.pos.line, 7);
    EXPECT_INT(tkn.pos.col, 14);
    EXPECT_INT(tkn.pos.len, 23);
    EXPECT_INT(tkn.type, TOKENTYPE_IDENT);
    EXPECT_STR(tkn.strval->data, "forauxiliary_identifier");

    tkn = c_tkn_next(&tkn_ctx); // "\x00\000\x0\x0000\770\377\00\0"
    EXPECT_INT(tkn.pos.line, 8);
    EXPECT_INT(tkn.pos.col, 0);
    EXPECT_INT(tkn.pos.len, 32);
    EXPECT_INT(tkn.type, TOKENTYPE_SCONST);
    EXPECT_STR(tkn.strval->data, "\x00\000\x0\x0000\0770\377\00\0");

    tkn = c_tkn_next(&tkn_ctx); // 'A'
    EXPECT_INT(tkn.pos.line, 9);
    EXPECT_INT(tkn.pos.col, 0);
    EXPECT_INT(tkn.pos.len, 3);
    EXPECT_INT(tkn.type, TOKENTYPE_CCONST);
    EXPECT_CHAR(tkn.ival, 'A');


    tkn = c_tkn_next(&tkn_ctx); // "\'\"\?\\\a\b\f\n\r\t\v"
    EXPECT_INT(tkn.pos.line, 10);
    EXPECT_INT(tkn.pos.col, 0);
    EXPECT_INT(tkn.pos.len, 24);
    EXPECT_INT(tkn.type, TOKENTYPE_SCONST);
    EXPECT_STR(tkn.strval->data, "\'\"\?\\\a\b\f\n\r\t\v");

    return TEST_OK;
}
LILY_TEST_CASE(tkn_basic)


// Test of various error messages.
static char const *tkn_errors() {
    // clang-format off
    char const data[] =
    "\'\n"                                                                  // Character constant spans end of line
    "\"\n"                                                                  // String constant spans end of line
    "0x0ffffffffffffffff\n"
    "0x10000000000000000\n"                                                 // Constant is too large and was truncated to 0
    "18446744073709551615\n"
    "18446744073709551616\n"                                                // Constant is too large and was truncated to 0
    "01777777777777777777777\n"
    "02000000000000000000000\n"                                             // Constant is too large and was truncated to 0
    "0b01111111111111111111111111111111111111111111111111111111111111111\n"
    "0b10000000000000000000000000000000000000000000000000000000000000000\n" // Constant is too large and was truncated to 0
    "0x 0b\n"                                                               // Invalid hexadecimal/binary constant
    "0xg 0b2 1a 08\n"                                                       // Invalid hexadecimal/binary/decimal/octal constant
    "\"\\U0\\U00000000\\u0\\u0000\\x\\x0\"\n"
    ;
    // clang-format on


    front_ctx_t *fe_ctx = front_create();
    srcfile_t   *src    = srcfile_create(fe_ctx, "<tkn_basic>", data, sizeof(data) - 1);

    tokenizer_t tkn_ctx = {
        .fe_ctx = fe_ctx,
        .file = src,
        .pos = {
            .srcfile = src,
        },
    };
    token_t tkn;
    do {
        tkn = c_tkn_next(&tkn_ctx);
    } while (tkn.type != TOKENTYPE_EOF);

    // '
    diagnostic_t *diag = (diagnostic_t *)fe_ctx->diagnostics.head;
    RETURN_ON_FALSE(diag);
    EXPECT_INT(diag->pos.line, 0);
    EXPECT_INT(diag->pos.col, 0);
    EXPECT_INT(diag->pos.len, 1);
    EXPECT_INT(diag->lvl, DIAG_ERR);
    EXPECT_STR(diag->msg, "Character constant spans end of line");

    // "
    diag = (diagnostic_t *)diag->node.next;
    RETURN_ON_FALSE(diag);
    EXPECT_INT(diag->lvl, DIAG_ERR);
    EXPECT_STR(diag->msg, "String constant spans end of line");

    // 0x10000000000000000
    diag = (diagnostic_t *)diag->node.next;
    RETURN_ON_FALSE(diag);
    EXPECT_INT(diag->pos.line, 3);
    EXPECT_INT(diag->pos.col, 0);
    EXPECT_INT(diag->pos.len, 19);
    EXPECT_INT(diag->lvl, DIAG_WARN);
    EXPECT_STR(diag->msg, "Constant is too large and was truncated to 0");

    // 18446744073709551616
    diag = (diagnostic_t *)diag->node.next;
    RETURN_ON_FALSE(diag);
    EXPECT_INT(diag->pos.line, 5);
    EXPECT_INT(diag->lvl, DIAG_WARN);
    EXPECT_STR(diag->msg, "Constant is too large and was truncated to 0");

    // 01777777777777777777777
    diag = (diagnostic_t *)diag->node.next;
    RETURN_ON_FALSE(diag);
    EXPECT_INT(diag->pos.line, 7);
    EXPECT_INT(diag->lvl, DIAG_WARN);
    EXPECT_STR(diag->msg, "Constant is too large and was truncated to 0");

    // 0b10000000000000000000000000000000000000000000000000000000000000000
    diag = (diagnostic_t *)diag->node.next;
    RETURN_ON_FALSE(diag);
    EXPECT_INT(diag->pos.line, 9);
    EXPECT_INT(diag->lvl, DIAG_WARN);
    EXPECT_STR(diag->msg, "Constant is too large and was truncated to 0");

    // 0x
    diag = (diagnostic_t *)diag->node.next;
    RETURN_ON_FALSE(diag);
    EXPECT_INT(diag->lvl, DIAG_ERR);
    EXPECT_STR(diag->msg, "Invalid hexadecimal constant");

    // 0b
    diag = (diagnostic_t *)diag->node.next;
    RETURN_ON_FALSE(diag);
    EXPECT_INT(diag->lvl, DIAG_ERR);
    EXPECT_STR(diag->msg, "Invalid binary constant");

    // 0xg
    diag = (diagnostic_t *)diag->node.next;
    RETURN_ON_FALSE(diag);
    EXPECT_INT(diag->lvl, DIAG_ERR);
    EXPECT_STR(diag->msg, "Invalid hexadecimal constant");

    // 0b2
    diag = (diagnostic_t *)diag->node.next;
    RETURN_ON_FALSE(diag);
    EXPECT_INT(diag->lvl, DIAG_ERR);
    EXPECT_STR(diag->msg, "Invalid binary constant");

    // 1a
    diag = (diagnostic_t *)diag->node.next;
    RETURN_ON_FALSE(diag);
    EXPECT_INT(diag->lvl, DIAG_ERR);
    EXPECT_STR(diag->msg, "Invalid decimal constant");

    // 08
    diag = (diagnostic_t *)diag->node.next;
    RETURN_ON_FALSE(diag);
    EXPECT_INT(diag->lvl, DIAG_ERR);
    EXPECT_STR(diag->msg, "Invalid octal constant");

    return TEST_OK;
}
LILY_TEST_CASE(tkn_errors)