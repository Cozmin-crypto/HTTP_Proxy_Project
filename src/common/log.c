#include "log.h"


#include <stdarg.h>
#include <stdio.h>
#include <time.h>

void log_line(const char *fmt, ...)
{
    char ts[16];
    time_t t = time(NULL);
    struct tm tmv;
    localtime_r(&t, &tmv);
    strftime(ts, sizeof ts, "%H:%M:%S", &tmv);

    printf("[%s] ", ts);
    va_list ap;
    va_start(ap, fmt);
    vprintf(fmt, ap);
    va_end(ap);
    printf("\n");
    fflush(stdout);
}