#include <stdio.h>
#include <string.h>

#include <pcre.h>

#include "util/util.h"


/*
 * Util
 */
const char *itoa(int i)
{
    static char a[32];

    memset(a, 0, sizeof a);

    snprintf(a, sizeof(a), "%d", i);
    return a;
}


int regex_match(const char *regex, const char *str)
{
    const char *err = NULL;
    int erroff = 0;
    int rc;

    pcre *reg = pcre_compile(regex, 0, &err, &erroff, NULL);

    if (!reg) {
        log_error("Cannot compile regular expression '%s': %s", regex, err);
        return 0;
    }

    rc = pcre_exec(reg, NULL, str, strlen(str), 0, 0, NULL, 0);
    if (rc < 0) {
        pcre_free(reg);

        return 0;
    }

    pcre_free(reg);
    return 1;
}

