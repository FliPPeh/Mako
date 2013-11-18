#include "tokenbucket.h"

#include "util/util.h"


void tokenbucket_init(struct tokenbucket *b, unsigned capacity, unsigned rate)
{
    b->last_update = time(NULL);

    b->tokens = capacity;

    b->capacity = capacity;
    b->fill_rate = rate;
}

bool tokenbucket_consume(struct tokenbucket *b, unsigned tokens)
{
    time_t now = time(NULL);

    if (b->tokens < b->capacity) {
        unsigned newtoks = b->fill_rate * (now - b->last_update);

        b->tokens = MIN(b->capacity, b->tokens + newtoks);
    }

    b->last_update = now;

    if (tokens <= b->tokens) {
        b->tokens -= tokens;
        return true;
    }

    return false;
}
