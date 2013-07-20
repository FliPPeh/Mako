#include <stdio.h>
#include <time.h>
#include <string.h>
#include <stdarg.h>
#include <errno.h>

#include "util/log.h"

#define X(lvl, name, fun, prefmt, postfmt) name,
static char *_log_levels[] = {
    LOGLEVELS
};
#undef X

#define X(lvl, name, fun, prefmt, postfmt) { .pre = prefmt, .post = postfmt },
struct _log_format
{
    const char *pre;
    const char *post;
};

static struct _log_format _log_line_formats[] = {
    LOGLEVELS
};
#undef X

struct log_context _log_default_logger = {
    .minlev = LOG_INFO,
    .file = "",
    .logf = NULL
};


/*
 * Logging
 */
int log_init(const char *logfile)
{
    struct log_context *log = &_log_default_logger;

    if (log->logf)
        fclose(log->logf);

    memset(log, 0, sizeof(*log));

    if (logfile != NULL) {
        if ((log->logf = fopen(logfile, "a")) == NULL)
            return errno;

        strncpy(log->file, logfile, sizeof(log->file) - 1);
    }

    log->minlev = LOG_INFO;

    return 0;
}

int log_set_minlevel(enum loglevel minlev)
{
    struct log_context *log = &_log_default_logger;

    return (log->minlev = minlev);
}

int log_destroy()
{
    struct log_context *log = &_log_default_logger;

    if (log->logf != NULL)
        fclose(log->logf);

    return 0;
}

int log_logf(enum loglevel lv, const char *name, const char *fmt, ...)
{
    va_list args;
    int ret;

    va_start(args, fmt);
    ret = _log_vlogf(lv, name, fmt, args);
    va_end(args);

    return ret;
}

int _log_vlogf(enum loglevel lv, const char *n, const char *fmt, va_list args)
{
    FILE *out = (lv >= LOG_ERROR) ? stderr : stdout;
    struct log_context *log = &_log_default_logger;

    if (lv < log->minlev)
        return 0;

    if (log->logf) {
        va_list argscp;

        va_copy(argscp, args);
        _log_vlogtofilep(log->logf, lv, n, fmt, argscp);
        va_end(argscp);

        fflush(log->logf);
    }

    _log_vlogtofilep(out, lv, n, fmt, args);
    fflush(out);

    return 0;
}

int _log_vlogtofilep(
        FILE *fp,
        enum loglevel lv,
        const char *n,
        const char *fmt,
        va_list args)
{
    char buffer[TIMEBUF_MAX] = {0};
    time_t now = time(NULL);

    struct tm *tm = TIME_GETTIME(&now);

    strftime(buffer, sizeof(buffer), STRFTIME_FORMAT, tm);

    if (n)
        fprintf(fp, LOG_FORMAT_NAMED "%s",
            LOG_ORDER_NAMED(buffer, n, _log_levels[lv]),
            _log_line_formats[lv].pre);
    else
        fprintf(fp, LOG_FORMAT "%s",
            LOG_ORDER(buffer, _log_levels[lv]),
            _log_line_formats[lv].pre);

    vfprintf(fp, fmt, args);
    fprintf(fp, "%s\n", _log_line_formats[lv].post);

    return 0;
}

/*
 * Utility function to replace perror
 */
void log_perror(const char *p, enum loglevel lv)
{
    log_perror_n(NULL, p, lv);
}

void log_perror_n(const char *n, const char *p, enum loglevel lv)
{
    log_logf(lv, n, "%s: %s", p, strerror(errno));
}


#define X(lvl, name, fun, prefmt, postfmt)  \
    void fun(const char *fmt, ...)          \
    {                                       \
        va_list args;                       \
                                            \
        va_start(args, fmt);                \
        _log_vlogf(lvl, NULL, fmt, args);   \
        va_end(args);                       \
                                            \
        return;                             \
    }                                       \
                                            \
    void fun ## _n(const char *nam, const char *fmt, ...)   \
    {                                                       \
        va_list args;                                       \
                                                            \
        va_start(args, fmt);                                \
        _log_vlogf(lvl, nam, fmt, args);                    \
        va_end(args);                                       \
                                                            \
        return;                                             \
    }                                                       \

LOGLEVELS
#undef X

