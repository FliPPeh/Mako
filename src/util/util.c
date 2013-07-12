#include <stdio.h>
#include <string.h>

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
