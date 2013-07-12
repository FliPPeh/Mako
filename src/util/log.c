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
    .name = "root",
    .file = "",
    .logf = NULL
};

/*
 * If this is not NULL, it will get used as the default logger until it is
 * not set to NULL anymore. Useful for temporary derived loggers for modules
 * that would otherwise only use the root logger.
 */
struct log_context *_log_temp_logger = NULL;
struct log_context *_log_prev_logger = NULL;

/*
 * Logging
 */
int log_init(struct log_context *log, const char *name, const char *logfile)
{
    log = PTR_OR_DEF(log);

    if (log->logf)
        fclose(log->logf);

    memset(log, 0, sizeof(*log));

    if (logfile != NULL) {
        if ((log->logf = fopen(logfile, "a")) == NULL)
            return errno;

        strncpy(log->file, logfile, sizeof(log->file) - 1);
    }

    strncpy(log->name, name, sizeof(log->name) - 1);
    log->minlev = LOG_INFO;

    return 0;
}

int log_set_minlevel(struct log_context *log, enum loglevel minlev)
{
    log = PTR_OR_DEF(log);

    return (log->minlev = minlev);
}


/*
 * Manipulate temporary default logger
 */
void log_set_default(struct log_context *log)
{
    _log_prev_logger = _log_temp_logger;
    _log_temp_logger = log;
}

void log_unset_default()
{
    _log_temp_logger = _log_prev_logger;
}

void log_restore_default()
{
    _log_temp_logger = NULL;
}


/* Derive a log context from an existing one (same minlev, same file,
 * different name) */
int log_derive(
        struct log_context *log,
        const struct log_context *olog,
        const char *name)
{
    char namebuf[LOGNAME_MAX] = {0};

    olog = PTR_OR_DEF(olog);

    memset(log, 0, sizeof(*log));

    snprintf(namebuf, sizeof(namebuf),
            LOG_DERIVED_NAME_FORMAT,
            LOG_DERIVED_NAME_ORDER(olog->name, name));

    log_init(log, namebuf, olog->logf ? olog->file : NULL);
    log->minlev = olog->minlev;

    return 0;
}

int log_destroy(struct log_context *log)
{
    log = PTR_OR_DEF(log);

    if (log->logf != NULL)
        fclose(log->logf);

    return 0;
}

int log_logf(struct log_context *log, enum loglevel lv, const char *fmt, ...)
{
    va_list args;
    int ret;

    va_start(args, fmt);
    ret = log_vlogf(log, lv, fmt, args);
    va_end(args);

    return ret;
}

int log_vlogf(
        struct log_context *log,
        enum loglevel lv,
        const char *fmt,
        va_list args)
{
    FILE *out = (lv >= LOG_ERROR) ? stderr : stdout;
    log = PTR_OR_DEF(log);

    if (lv < log->minlev)
        return 0;

    if (log->logf) {
        va_list argscp;

        va_copy(argscp, args);
        log_vlogtofilep(log, log->logf, lv, fmt, argscp);
        va_end(argscp);

        fflush(log->logf);
    }

    log_vlogtofilep(log, out, lv, fmt, args);
    fflush(out);

    return 0;
}

int log_vlogtofilep(
        struct log_context *log,
        FILE *fp,
        enum loglevel lv,
        const char *fmt,
        va_list args)
{
    char buffer[TIMEBUF_MAX] = {0};
    time_t now = time(NULL);

    struct tm *tm = TIME_GETTIME(&now);

    strftime(buffer, sizeof(buffer), STRFTIME_FORMAT, tm);

    fprintf(fp, LOG_FORMAT "%s",
            LOG_ORDER(buffer, log->name, _log_levels[lv]),
            _log_line_formats[lv].pre);

    vfprintf(fp, fmt, args);
    fprintf(fp, "%s\n", _log_line_formats[lv].post);

    return 0;
}

#define X(lvl, name, fun, prefmt, postfmt) \
    void fun(struct log_context *log, const char *fmt, ...) \
    {                                                       \
        va_list args;                                       \
                                                            \
        va_start(args, fmt);                                \
        log_vlogf(log, lvl, fmt, args);                     \
        va_end(args);                                       \
                                                            \
        return;                                             \
    }

LOGLEVELS
#undef X

