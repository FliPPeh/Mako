#ifndef TOKENBUCKET_H
#define TOKENBUCKET_H

#include <stdlib.h>
#include <stdbool.h>
#include <time.h>

struct tokenbucket
{
    time_t last_update;

    unsigned tokens;

    unsigned capacity;  /* maximum number of tokens */
    unsigned fill_rate; /* fill rate per second */
};

void tokenbucket_init(struct tokenbucket *b, unsigned capacity, unsigned rate);
bool tokenbucket_consume(struct tokenbucket *b, unsigned tokens);

#endif /* define TOKENBUCKET_H */
