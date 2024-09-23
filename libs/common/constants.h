/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2024 Adel Mamin
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#ifndef COMMON_CONSTANTS_H_INCLUDED
#define COMMON_CONSTANTS_H_INCLUDED

/*
 * Taken from:
 * https://en.wikipedia.org/wiki/ANSI_escape_code#Colors
 * Another good explanation:
 * https://chrisyeh96.github.io/2020/03/28/terminal-colors.html
 */

#define ANSI_COLOR_RED "\x1b[31m"
#define ANSI_COLOR_RED_BOLD "\x1b[1;31m"

#define ANSI_COLOR_GREEN "\x1b[32m"
#define ANSI_COLOR_GREEN_BOLD "\x1b[1;32m"

#define ANSI_COLOR_YELLOW "\x1b[33m"
#define ANSI_COLOR_YELLOW_BOLD "\x1b[1;33m"

#define ANSI_COLOR_BLUE "\x1b[34m"
#define ANSI_COLOR_BLUE_BOLD "\x1b[1;34m"

#define ANSI_COLOR_MAGENTA "\x1b[35m"
#define ANSI_COLOR_MAGENTA_BOLD "\x1b[1;35m"

#define ANSI_COLOR_CYAN "\x1b[36m"
#define ANSI_COLOR_CYAN_BOLD "\x1b[1;36m"

#define ANSI_COLOR_WHITE "\x1b[37m"
#define ANSI_COLOR_WHITE_BOLD "\x1b[1;37m"

#define ANSI_COLOR_RESET "\x1b[0m"

#endif /* COMMON_CONSTANTS_H_INCLUDED */
