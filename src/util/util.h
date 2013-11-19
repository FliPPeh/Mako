#ifndef UTIL_H
#define UTIL_H

#include "util/log.h"

#define MIN(a,b) ((a) < (b) ? (a) : (b))
#define MAX(a,b) ((a) > (b) ? (a) : (b))

/*
 * Util
 */
const char *itoa(int i);

/*
 * String utils
 */

/*
 * Strip leading and trailing whitespace, returns the new beginning
 * of the string
 */
char *strstrp(char *src);

/*
 * Split the source string at the first occurence of a separator, storing the
 * parts into the destination buffers.
 *
 * Returns a pointer to the first character after the separator for easier
 * chaining.
 */
char *strpart(const char *src, char sep,
        char *desta, size_t sizea,
        char *destb, size_t sizeb);

int regex_match(const char *regex, const char *str);

char *strdup(const char *s);

#ifdef DEBUG
#   define ASSERT(cond, lev, action)                                    \
        if (!(cond)) {                                                  \
            log_logf((lev), "%s:%d (%s): assertion `%s` failed",        \
                    __FILE__,                                           \
                    __LINE__,                                           \
                    __func__,                                           \
                    #cond);                                             \
            action;                                                     \
        }
#else
#   define ASSERT(cond, lev, action)
#endif

#endif /* defined UTIL_H */
