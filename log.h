#ifndef LOG_H
#define LOG_H

#include <stdbool.h>
#include <stdio.h>

#ifdef DEBUG
#define DLOG(...) fprintf(stderr, __VA_ARGS__)
#else
#define DLOG(...)
#endif

typedef enum logtype {
    LOG_DEBUG,
    LOG_WARNING,
    LOG_ERROR,
    LOG_RAW
} logtype;

typedef struct logctx {
    FILE *handle;
    bool file;
} logctx;

logctx *log_init(const char *, FILE *);

bool log_write(const logctx *, logtype, const char *, ...);

void log_free(logctx *);

#endif
