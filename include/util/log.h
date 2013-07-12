#ifndef _LOG_H_
#define _LOG_H_

#include <stdio.h>

/* Enable debugging? */
#define DEBUG

#define LOGNAME_MAX 128
#define LOGFILE_MAX 256

/* Time formatting stuff */
#define TIMEBUF_MAX 256
#undef TIME_LOCALTIME

#ifdef TIME_LOCALTIME
#   define TIME_GETTIME localtime
#   define STRFTIME_FORMAT "%Y-%m-%dT%T%z (%Z)"
#else
#   define TIME_GETTIME gmtime
#   define STRFTIME_FORMAT "%Y-%m-%dT%T (%Z)"
#endif

/*
 * Log header output format
 */
#define LOG_FORMAT "\e[1;34m%s\e[0m [\e[1;31m%s\e[0m.\e[0;33m%s\e[0m]: "
#define LOG_ORDER(time, name, level) time, name, level

#define LOG_DERIVED_NAME_FORMAT "%s/%s"
#define LOG_DERIVED_NAME_ORDER(super, deriv) super, deriv

/*
 * Defines all possible log levels and various properties.
 *
 * 1. Name, e.g. LOG_DEBUG (will become an enum value)
 * 2. Display name, e.g. "debug" (will be displayed)
 * 3. Function to generate that logs this level by default, e.g. log_debug
 * 4. A format string that will be written before the message(!). For example,
 *    a colour code that highlights errors in red.
 * 5. A format string that will be written after the message(!). In case the
 *    preformat (4) was a format code, this would be the place to reset the
 *    terminal format to default.
 */
#define LOGLEVELS \
    X(LOG_TRACE,   "trace",   log_trace, "\e[1m", "\e[0m")    \
    X(LOG_DEBUG,   "debug",   log_debug, "\e[1m", "\e[0m")    \
    X(LOG_INFO,    "info",    log_info,  "", "")              \
    X(LOG_WARNING, "warning", log_warn,  "\e[1;33m", "\e[0m") \
    X(LOG_ERROR,   "error",   log_error, "\e[1;31m", "\e[0m") \
    X(LOG_FATAL,   "fatal",   log_fatal, "\e[1;31m", "\e[0m") \

#define X(lvl, name, fun, prefmt, postfmt) lvl,
enum loglevel
{
    LOGLEVELS
};
#undef X

struct log_context
{
    enum loglevel minlev;
    char name[LOGNAME_MAX];

    char file[LOGFILE_MAX];
    FILE *logf;
};

extern struct log_context _log_default_logger;

#define PTR_OR_DEF(ptr) \
    (ptr ? ptr : (_log_temp_logger ? _log_temp_logger : &_log_default_logger))

/*
 * Logging
 */
int log_init(struct log_context *log, const char *name, const char *logfile);
int log_set_minlevel(struct log_context *log, enum loglevel minlev);

/*
 * Manipulate temporary default logger.
*
 * These seemed like a much better idea than they turned out to be. Keeping them
 * in. In case they do get useful later.
 */
void log_set_default(struct log_context *log);
void log_unset_default();
void log_restore_default();

#define LOG_WITH_TEMP(templog, expr) \
    log_set_default(templog); \
    expr;                     \
    log_unset_default();    \

#define LOG_WITH_DERIV_TEMP(log, olog, name, expr) \
    log_derive(log, olog, name); \
    LOG_WITH_TEMP(log, expr);    \
    log_destroy(log);

/* Derive a log context from an existing one (same minlev, same file pointer,
 * different name) */
int log_derive(
        struct log_context *log,
        const struct log_context *olog,
        const char *name);

int log_destroy(struct log_context *log);

int log_logf(struct log_context *log, enum loglevel lv, const char *fmt, ...);
int log_vlogf(
        struct log_context *log,
        enum loglevel lv,
        const char *fmt,
        va_list args);

int log_vlogtofilep(
        struct log_context *log,
        FILE *fp,
        enum loglevel lv,
        const char *fmt,
        va_list args);

#define X(lvl, name, fun, prefmt, postfmt) \
    void fun(struct log_context *log, const char *fmt, ...);

LOGLEVELS
#undef X

#endif /* defined _LOG_H_ */
