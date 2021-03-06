/*
 * Copyright (C) 2013 Matt Kilgore
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License v2 as published by the
 * Free Software Foundation.
 */
#ifndef INCLUDE_TERMCOLS_H
#define INCLUDE_TERMCOLS_H

/* Color macros pulled from git source code
 * https://github.com/git/git */

#define TERM_COLOR_NORMAL       ""
#define TERM_COLOR_RESET        "\033[0m"
#define TERM_COLOR_BOLD         "\033[1m"
#define TERM_COLOR_REVERSE      "\033[7m"
#define TERM_COLOR_RED          "\033[31m"
#define TERM_COLOR_GREEN        "\033[32m"
#define TERM_COLOR_YELLOW       "\033[33m"
#define TERM_COLOR_BLUE         "\033[34m"
#define TERM_COLOR_MAGENTA      "\033[35m"
#define TERM_COLOR_CYAN         "\033[36m"
#define TERM_COLOR_WHITE        "\033[37m"
#define TERM_COLOR_BOLD_RED     "\033[1;31m"
#define TERM_COLOR_BOLD_GREEN   "\033[1;32m"
#define TERM_COLOR_BOLD_YELLOW  "\033[1;33m"
#define TERM_COLOR_BOLD_BLUE    "\033[1;34m"
#define TERM_COLOR_BOLD_MAGENTA "\033[1;35m"
#define TERM_COLOR_BOLD_CYAN    "\033[1;36m"
#define TERM_COLOR_BOLD_WHITE   "\033[1;37m"
#define TERM_COLOR_BG_RED       "\033[41m"
#define TERM_COLOR_BG_GREEN     "\033[42m"
#define TERM_COLOR_BG_YELLOW    "\033[43m"
#define TERM_COLOR_BG_BLUE      "\033[44m"
#define TERM_COLOR_BG_MAGENTA   "\033[45m"
#define TERM_COLOR_BG_CYAN      "\033[46m"
#define TERM_COLOR_BG_WHITE     "\033[47m"

#endif
