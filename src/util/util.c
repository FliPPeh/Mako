#include <stdio.h>
#include <string.h>

#include <regex.h>

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
    int c;
    int match = 0;
    regex_t regexc;

    if ((c = regcomp(&regexc, regex, REG_EXTENDED | REG_ICASE))) {
        log_error("Could not compile regular expression '%s'", regex);
        return 0;
    }

    if (!(c = regexec(&regexc, str, 0, NULL, 0))) {
        match = 1;
    } else {
        if (c != REG_NOMATCH ) {
            char err[128] = {0};

            regerror(c, &regexc, err, sizeof(err));
            log_error("Regular expression match failed: %s\n", err);
        }

        match = 0;
    }

    regfree(&regexc);
    return match;
}

