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
#define LOG_FORMAT_NAMED \
    "\033[1;34m%s\033[0m [\033[1;31m%s\033[0m.\033[0;33m%s\033[0m]: "

#define LOG_ORDER_NAMED(time, name, level) time, name, level

#define LOG_FORMAT "\033[1;34m%s\033[0m [\033[0;33m%s\033[0m]: "
#define LOG_ORDER(time, level) time, level

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
    X(LOG_TRACE,   "trace",   log_trace, "\033[1m", "\033[0m")    \
    X(LOG_DEBUG,   "debug",   log_debug, "\033[1m", "\033[0m")    \
    X(LOG_INFO,    "info",    log_info,  "", "")              \
    X(LOG_WARNING, "warning", log_warn,  "\033[1;33m", "\033[0m") \
    X(LOG_ERROR,   "error",   log_error, "\033[1;31m", "\033[0m") \
    X(LOG_FATAL,   "fatal",   log_fatal, "\033[1;31m", "\033[0m") \
    X(LOG_WTF,     "wtf?",    log_wtf,   "\033[1;31m", "!?\033[0m")

#define X(lvl, name, fun, prefmt, postfmt) lvl,
enum loglevel
{
    LOGLEVELS
};
#undef X

struct log_context
{
    enum loglevel minlev;

    char file[LOGFILE_MAX];
    FILE *logf;
};

extern struct log_context _log_default_logger;


/*
 * Core
 */
int log_init(const char *logfile);
int log_set_minlevel(enum loglevel minlev);
int log_destroy(void);


int log_logf(enum loglevel lv, const char *name, const char *fmt, ...);

int _log_vlogf(enum loglevel lv, const char *n, const char *fmt, va_list args);
int _log_vlogtofilep(
        FILE *fp,
        enum loglevel lv,
        const char *n,
        const char *fmt,
        va_list args);

/*
 * Generate specialized logging functions (log_info, log_warn, ...)
 *
 * If nam is not NULL, nam will be temporarily pushed for the current call.
 */
#define X(lvl, name, fun, prefmt, postfmt)                                  \
    void fun(const char *fmt, ...);                                         \
    void fun ## _n(const char *nam, const char *fmt, ...);

LOGLEVELS
#undef X

#endif /* defined _LOG_H_ */
