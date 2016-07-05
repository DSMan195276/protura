/*
 * Copyright (C) 2016 Matt Kilgore
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License v2 as published by the
 * Free Software Foundation.
 */
#include "common.h"

#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#include "input_lexer.h"

enum input_token lexer_next_token(struct input_lexer *lex)
{
    char tok;

    /* Swallow any leading white-space */
    while (tok = lex->input[lex->location],
           tok == ' ' || tok == '\t')
        lex->location++;

    if (lex->input[lex->location] == '\0')
        return TOK_EOF;

    lex->str = lex->input + lex->location;

    switch (lex->input[lex->location++]) {
    case '#':
        lex->len = 1;
        return TOK_COMMENT;

    case '<':
        lex->len = 1;
        return TOK_REDIRECT_IN;

    case '>':
        lex->len = 1;
        return TOK_REDIRECT_OUT;

    case '&':
        if (lex->input[lex->location] != '&')
            return TOK_UNKNOWN;

        lex->location++;
        lex->len = 2;
        return TOK_LOGIC_AND;

    case '|':
        if (lex->input[lex->location] != '|') {
            lex->len = 1;
            return TOK_PIPE;
        }

        lex->location++;
        lex->len = 2;
        return TOK_LOGIC_OR;

    case '\n':
        lex->line++;
        lex->len = 1;
        return TOK_NEWLINE;

    /* Everything else is considered part of a string */
    default:
        lex->len = 1;
        while (tok = lex->input[lex->location],
               (tok >= 'a' && tok <= 'z')
               || (tok >= 'A' && tok <= 'Z')
               || tok == '_' || tok == '.' || tok == '/') {
            lex->len++;
            lex->location++;
        }

        return TOK_STRING;
    }
}

