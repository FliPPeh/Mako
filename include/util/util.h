#ifndef _UTIL_H_
#define _UTIL_H_

#include "util/log.h"

#define MIN(a,b) ((a) < (b) ? (a) : (b))
#define MAX(a,b) ((a) > (b) ? (a) : (b))

/*
 * Util
 */
const char *itoa(int i);
int regex_match(const char *regex, const char *str);

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

#endif /* defined _UTIL_H_ */
