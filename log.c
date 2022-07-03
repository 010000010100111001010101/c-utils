#include "log.h"

#include "str.h"

#include <errno.h>
#include <stdarg.h>
#include <stdlib.h>

#define LOG_DEFAULT_STREAM stderr
#define LOG_TIMESTAMP_FORMAT "%m/%d/%Y %H:%M:%S"
#define LOG_TIMESTAMP_LENGTH 32

logctx *log_init(const char *filename, FILE *stream){
    logctx *log = malloc(sizeof(*log));

    if (!log){
        DLOG(
            "[%s] log_init() - log object alloc failed\n",
            __FILE__
        );

        return NULL;
    }

    if (filename){
        log->handle = fopen(filename, "a+");

        if (!log->handle){
            DLOG(
                "[%s] log_init() - failed to open '%s' in append mode\n",
                __FILE__,
                filename
            );

            free(log);

            return NULL;
        }

        log->file = true;
    }
    else {
        log->handle = stream ? stream : LOG_DEFAULT_STREAM;
        log->file = false;
    }

    return log;
}

bool log_write(const logctx *log, logtype type, const char *format, ...){
    if (!format){
        DLOG(
            "[%s] log_write() - format string is NULL\n",
            __FILE__
        );

        return false;
    }

    FILE *handle = LOG_DEFAULT_STREAM;

    if (log && log->handle){
        handle = log->handle;
    }

    char timestamp[LOG_TIMESTAMP_LENGTH];

    if (!string_from_time(LOG_TIMESTAMP_FORMAT, timestamp, sizeof(timestamp))){
        DLOG(
            "[%s] log_write() - failed to get timestamp string\n",
            __FILE__
        );
    }

    const char *typestr = NULL;

    switch (type){
    case LOG_DEBUG:
        typestr = "DEBUG";

        break;
    case LOG_ERROR:
        typestr = "ERROR";

        break;
    case LOG_WARNING:
        typestr = "WARN";

        break;
    case LOG_RAW:
        /* keep typestr NULL */

        break;
    }

    if (typestr){
        fprintf(handle, "(%s) %s ", timestamp, typestr);
    }
    else if (type != LOG_RAW) {
        fprintf(handle, "(%s) ", timestamp);
    }

    va_list args;

    va_start(args, format);

    int err = vfprintf(handle, format, args);

    va_end(args);

    if (err < 0){
        DLOG(
            "[%s] log_write() - vfprintf() failed\n",
            __FILE__
        );

        return false;
    }

    /*
    if (type == LOG_ERROR){
        print errno?
    }
    */

    return true;
}

void log_free(logctx *log){
    if (!log){
        DLOG(
            "[%s] log_free() - log is NULL\n",
            __FILE__
        );

        return;
    }

    if (log->file){
        int err = fclose(log->handle);

        switch (err){
        case EBADF:
            DLOG(
                "[%s] log_free() - handle is not active\n",
                __FILE__
            );

            break;
        case EINTR:
            DLOG(
                "[%s] log_free() - handle was interrupted\n",
                __FILE__
            );

            break;
        case EIO:
            DLOG(
                "[%s] log_free() - handle encountered I/O error\n",
                __FILE__
            );

            break;
        case 0:
            break;
        default:
            DLOG(
                "[%s] log_free() - unknown error: '%d'\n",
                __FILE__,
                err
            );
        }
    }

    free(log);
}
