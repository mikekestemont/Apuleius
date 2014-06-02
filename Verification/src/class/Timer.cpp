#include <Timer.h>

//___________________________________________________________________
Timer::Timer()
{
    this->reset();
}

//___________________________________________________________________
void Timer::reset()
{
    iAccumMSc=0;
    iAccumSec=0;
    timerStatus=stopped;
}

//___________________________________________________________________
void Timer::start()
{
    if (timerStatus!=stopped) return;
    timerStatus=running;
    get_time(&start_tv);
}

//___________________________________________________________________
void Timer::stop()
{
    unsigned int iSecs,iMSc;
    
    if (timerStatus!=running) return;
    timerStatus=stopped;
    get_time(&stop_tv);
    
    iSecs = stop_tv.tv_sec - start_tv.tv_sec - 1;
    iMSc  = TIME_ACCURACY - start_tv.tv_usec + stop_tv.tv_usec;
    correct_time(iSecs,iMSc);
    
    iAccumSec += iSecs;
    iAccumMSc += iMSc;
    correct_time(iAccumSec,iAccumMSc);
}

//___________________________________________________________________
unsigned int Timer::iGetSec() const
{
    return iAccumSec;
}

//___________________________________________________________________
unsigned int Timer::iGetMSc() const
{
    return iAccumMSc;
}

//___________________________________________________________________
unsigned int Timer::iGetTime() const
{
    return (iAccumSec*DISP_RESOLUSION + iAccumMSc/DISP_RESOLUSION);
}

//___________________________________________________________________
string Timer::stGetTime() const
{
    char buf[TIME_BUF_LEN];
    _snprintf(buf,TIME_BUF_LEN,"%d:%d",iAccumSec,(iAccumMSc/DISP_RESOLUSION));
    return buf;
}

//___________________________________________________________________
void Timer::get_time(struct timeval * tv)
{
    if (gettimeofday(tv,NULL) < 0)
    {
        fprintf(stderr,"Could not get time: [%s] at Timer::get_time - Abort\n",strerror(errno));
        fflush(stderr);
        exit(-1);
    }
}

//___________________________________________________________________
void Timer::correct_time(unsigned int & iSecs,unsigned int & iMSc)
{
    iSecs += (iMSc / TIME_ACCURACY);
    iMSc   = (iMSc%TIME_ACCURACY);
}

//___________________________________________________________________
int gettimeofday(struct timeval * tv, struct timezone * tz)
{
    FILETIME ft;
    unsigned __int64 tmpres = 0;

    if (NULL != tv)
    {
        GetSystemTimeAsFileTime(&ft);

        tmpres |= ft.dwHighDateTime;
        tmpres <<= 32;
        tmpres |= ft.dwLowDateTime;

        /*converting file time to unix epoch*/
        tmpres -= DELTA_EPOCH_IN_MICROSECS; 
        tmpres /= 10;  /*convert into microseconds*/
        tv->tv_sec = (long)(tmpres / 1000000UL);
        tv->tv_usec = (long)(tmpres % 1000000UL);
    }

    if (NULL != tz)
    {
        _tzset();
        tz->tz_minuteswest = _timezone / 60;
        tz->tz_dsttime = _daylight;
    }

    return 0;
}
