#define _POSIX_C_SOURCE 200809L

#include <inttypes.h>
#include <math.h>
#include <stdio.h>
#include <time.h>

void print_current_time_with_us (const char *msg)
{
    long            ms, us, remaining;
    time_t          s;  // Seconds
    struct timespec spec;

    clock_gettime(CLOCK_MONOTONIC, &spec);

    s  = spec.tv_sec;
    ms = round(spec.tv_nsec / 1.0e6);
    remaining = spec.tv_nsec % (long int) 1e6;
    us = round(remaining / 1.0e3);
    remaining = remaining % (long int) 1e3;
    if (ms > 999) {
        s++;
        ms = ms % 1000;
    }
    if (us > 999) {
        ms++;
        us = us % 1000;
    }

    fprintf(stdout, "%"PRIdMAX".%03ld.%03ld.%03ld %s\n", (intmax_t)s, ms, us, remaining,  msg);
}
