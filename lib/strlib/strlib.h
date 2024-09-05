/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2017,2021 Adel Mamin
 * Copyright (c) 2017 Martin Sustrik
 * Copyright (c) 2008, 2009, 2010, 2012, 2013, 2014, 2015  William Ahern
 * Copyright (c) 2010 Przemo Nowaczyk <pnowaczyk.mail@gmail.com>
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

/**
 * @file
 * string utilities API
 */

#ifndef STRLIB_H_INCLUDED
#define STRLIB_H_INCLUDED

#include <complex.h>
#include <stdarg.h>

/**
 * Check if string has a boolean value.
 * Case insensitive.
 * @param str            the string to check
 * @param extracted_val  the extracted value of the string
 *                       as boolean, if str is boolean
 * @retval true          the string is boolean
 * @retval false         the string is not boolean
 */
bool str_is_bool(const char *str, bool *extracted_val);

/**
 * Check if the string is double and extracts it
 * @param str            the string to analyze
 * @param extracted_val  the extracted double
 * @retval true          double was recognized and extracted
 * @retval false         double was not recognized
 */
bool str_is_double(const char *str, double *extracted_val);

/**
 * Check if string has integer value in it. Case insensitive.
 * @param str            the string to check
 * @param extracted_val  the extracted value of the string
 *                       as integer, if str has integer value
 * @param base           the base of the expected integer in represented by str
 * @retval true          the string has integer value
 * @retval false         the string does not have integer value in it
 */
bool str_is_intmax(const char *str, intmax_t *extracted_val, int base);

/**
 * Check if the string contains a number in decimal form.
 * @param str            the string to analyze.
 * @param extracted_val  the extracted value
 * @retval true          the decimal number was detected
 * @retval false         the decimal number was not detected
 */
bool str_is_decimal(const char *str, intmax_t *extracted_val);

/**
 * Check if the string contains a number in hexadecimal form.
 * @param str            the string to analyze
 * @param extracted_val  the extracted value
 * @retval true          the hexadecimal number was detected
 * @retval false         the hexadecimal number was not detected
 */
bool str_is_hex(const char *str, intmax_t *extracted_val);

/**
 * Check if the string contains a number in a binary form.
 * @param str            the string to analyze
 * @param extracted_val  the extracted value
 * @retval true          the binary was detected
 * @retval false         the binary was not detected
 */
bool str_is_binary(const char *str, intmax_t *extracted_val);

/**
 * Check if the string contains a number in octal form.
 * @param str            the string to analyze
 * @param extracted_val  the extracted value
 * @retval true          the octal number was detected
 * @retval false         the octal number was not detected
 */
bool str_is_octal(const char *str, intmax_t *extracted_val);

/**
 * Check if string has "null" in it. Case insensitive.
 * @param str     the string to check
 * @retval true   the string has "null" in it
 * @retval false  the string does not have "null" in it
 */
bool str_is_null(const char *str);

/**
 * Convert uintmax to binary string.
 * @param str      the binary string buffer.
 * @param strsize  str buffer size
 * @param uintmax  the number to convert
 * @return the number of bytes printed to str
 */
int uintmax_to_binstr(char *str, int strsize, uintmax_t uintmax);

/**
 * Convert string to float complex.
 * @param str  the string to convert
 * @return the resulting float complex number
 */
float complex str_to_complex(char *str);

/**
 * Convert string to double.
 * @param str     the string to convert
 * @param val     the converted value
 * @param endptr  point to the first character past the double
 * @retval true   the string was converted successfully
 * @retval false  the string was not converted successfully
 */
bool str_to_double(const char *str, double *val, char **endptr);

/**
 * Check if all characters are in the range '0'-'9'.
 * @param str     the string to analyze
 * @retval false  not all charachters are decimal digits
 * @retval true   all characters are decimal digits
 */
bool str_is_all_decimal_digits(const char *str);

/**
 * Check if string starts with '0x' and all characters are in the range
 * '0'-'9', 'a'-'f', 'A'-'F'.
 * @param str  the string to analyze
 * @retval false  not all charachters are hexadecimal digits
 * @retval true   all characters are hexadecimal digits
 */
bool str_is_all_hexadecimal_digits(const char *str);

/**
 * Return a pointer to the first character in string that is not delim.
 * Taken from https://github.com/sustrik/libdill.
 * @param string  the string to analyze
 * @param delim   the delimeter character
 * @return the pointer to the first character in string that is not delim
 */
const char *str_lstrip(const char *string, char delim);

/**
 * Return a pointer after the last character in string that is not delim.
 * Taken from https://github.com/sustrik/libdill
 * @param string  the string to analyze
 * @param delim   the delimeter character
 * @return the pointer after the last character in string that is not delim
 */
const char *str_rstrip(const char *string, char delim);

/**
 * Copy strings.
 * The custom implementation of strlcpy() from BSD systems.
 * Designed to be safer, more consistent, and less error prone
 * replacement for strncpy. It takes the full size of the buffer (not just the
 * length) and guarantee to NUL-terminate the result (as long as size is larger
 * than 0.
 * Note that a byte for the NUL should be included in size.  Also
 * note that it only operates on true “C” strings.  This
 * means that src must be NUL-terminated.
 *
 * It copies up to size - 1 characters from the
 * NUL-terminated string src to dst, NUL-terminating the result.
 *
 * @param dst  the destination string buffer
 * @param src  the source string buffer
 * @param lim  the full size of dst buffer [bytes]
 * @return the total length of the string it tried to create, i.e.
 *         the length of src
 */
int str_lcpy(char *dst, const char *src, int lim);

/**
 * Concatenate strings.
 * It is designed to be safer, more consistent, and less
 * error prone replacement for strncat.
 * It takes the full size of the buffer (not just the length)
 * and guarantees to NUL-terminate the result (as long as there is at
 * least one byte free in dst). Note that a byte for the NUL should be
 * included in size.  Also it only operates on true “C” strings.
 * This means that both src and dst must be NUL-terminated.
 *
 * It appends the NUL-terminated string src to the end of dst.
 * It will append at most lim - strlen(dst) - 1 bytes, NUL-terminating
 * the result.
 *
 * Note, however, that if it traverses size characters without finding
 * a NUL, the length of the string is considered to be size and the
 * destination string will not be NUL-terminated (since there was no space
 * for the NUL). This keeps it from running off the end of a string.
 * In practice this should not happen (as it means that either size is
 * incorrect or that dst is not a proper “C” string).
 * The check exists to prevent potential security problems in incorrect code.
 * @param dst  the destination buffer
 * @param src  the source buffer
 * @param lim  the full size of the buffer (not just the length) [bytes]
 * @return the total length of the string it tried to create
 *         It means the initial length of dst plus the length of src.
 */
int str_lcat(char *dst, const char *src, int lim);

/**
 * Same as str_lcat(), but the source buffer is replaced with format string.
 * @param dst  the destination buffer.
 * @param lim  the full size of the buffer (not just the length) [bytes].
 * @param fmt  the format string.
 * @return Returns the total length of the string they tried to create.
 *         It means the initial length of dst plus the length of src.
 */
int str_vlcatf(char *dst, int lim, const char *fmt, va_list ap);

/**
 * Same as str_lcat(), but the source buffer is replaced with format string.
 * @param dst  the destination buffer
 * @param lim  the full size of the buffer (not just the length) [bytes]
 * @param fmt  the format string
 * @return the total length of the string they tried to create
 *         It means the initial length of dst plus the length of src.
 */
PRINTF(3, 4) int str_lcatf(char *dst, int lim, const char *fmt, ...);

/**
 * String tokenizer.
 * Return the pointer to the first token in *sp separated by any character in
 * delim. Skip any leading delimeters if any.
 *
 * A similar code was posted here:
 * https://groups.google.com/forum/message/raw?msg=comp.lang.c/ZhXAlw6VZsA/_Y5evTIkf6kJ
 * Also see:
 * https://onebyezero.blogspot.com/2018/12/string-tokenization-in-c.html
 *
 * @param sp     the input string
 * @param delim  the list of one character delimeters
 * @return The first '\0' terminated token in sp or NULL if none.
 */
char *str_sep(char **sp, const char *delim);

/**
 * Check if string has the given prefix.
 * @param str     the string to check
 * @param prefix  the prefix to search for
 * @retval true   the prefix was found
 * @retval false  the prefix was not found
 */
bool str_has_prefix(const char *str, const char *prefix);
const char *str_skip_prefix(const char *str, const char *prefix);
char *str_add_prefix(char *out, int outsz, const char *str, const char *prefix);

/**
 * Convert string to upper case in place.
 * @param str  string to convert
 * @return the converted string
 */
char *str_upr(char *str);

int str_lltoa(char *s, int sz, long long n, int base);
int str_ulltoa(char *s, int sz, unsigned long long n, int base);

/** String token */
struct str_token {
    int start; /**< start index */
    int end;   /**< end index. Total length is end - start */
};

/**
 * Split path to head and tail.
 * @param path   the path to split
 * @param head   the resulting path head
 * @param tail   the resulting path tail
 * @param delim  delimiter
 */
void str_split_path(
    const char *path,
    struct str_token *head,
    struct str_token *tail,
    const char *delim
);

/**
 * Concatenate two parts of file path.
 * @param dst    path head
 * @param src    path tail
 * @param lim    head's buffer size [bytes]
 * @param delim  delimiter character
 * @return The total length of the string it tried to create.
 *         It means the initial length of head plus the length
 *         of delim and tail.
 */
int str_lcat_path(char *dst, const char *src, int lim, char delim);

#endif /* STRLIB_H_INCLUDED */
