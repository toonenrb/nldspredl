#ifndef PRINTTM_H
#define PRINTTM_H

void print_current_time_with_us (const char *msg);

#ifdef TIMEMSGPRINT
#define TMMSG(msg) print_current_time_with_us(msg)
#else
#define TMMSG(msg)
#endif


#endif
