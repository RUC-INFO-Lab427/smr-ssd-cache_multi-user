#include "timerUtils.h"

void _TimerStart(timeval* tv_start)
{
    gettimeofday(tv_start, NULL);
}

void _TimerStop(timeval* tv_stop)
{
    gettimeofday(tv_stop, NULL);
}

microsecond_t TimerInterval_MICRO(timeval* tv_start, timeval* tv_stop)
{

    return (tv_stop->tv_sec-tv_start->tv_sec)*1000000 + (tv_stop->tv_usec-tv_start->tv_usec);
}
double TimerInterval_SECOND(timeval* tv_start, timeval* tv_stop)
{
    return (tv_stop->tv_sec-tv_start->tv_sec) + (tv_stop->tv_usec-tv_start->tv_usec)/1000000.0;
}
double Mirco2Sec(microsecond_t msecond)
{
    return msecond/1000000.0;
}

double Mirco2Milli(microsecond_t msecond)
{
    return msecond/1000.0;
}
