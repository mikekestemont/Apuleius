#ifndef __Timer__
#define __Timer__

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <string>
#include <errno.h>
#include <time.h>
#include <windows.h> 

#define TIME_ACCURACY   1000000
#define DISP_RESOLUSION 1000
#define TIME_BUF_LEN    250

#if defined(_MSC_VER) || defined(_MSC_EXTENSIONS)
  #define DELTA_EPOCH_IN_MICROSECS  11644473600000000Ui64
#else
  #define DELTA_EPOCH_IN_MICROSECS  11644473600000000ULL
#endif

struct timezone 
{
    int tz_minuteswest; /* minutes W of Greenwich */
    int tz_dsttime;     /* type of dst correction */
};

int gettimeofday(struct timeval * tv, struct timezone * tz);

enum TimerStatus
{
    stopped,
    running
};

using namespace std;

class Timer
//______________________________________________________________________
//desc:Timer is a stop watch. It can be started, stopped and its value
//     can be read. The resolution of the Timer is mili seconds (1/1000).
//_______________________________________________________________________
{
    private:
        
        struct timeval start_tv;
        struct timeval stop_tv;
        
        unsigned int iAccumSec;
        unsigned int iAccumMSc;
        
        TimerStatus timerStatus;
        
        void get_time(struct timeval * tv);
        void correct_time(unsigned int & iSecs,unsigned int & iMSc);
        
        Timer(const Timer&);
        void operator=(const Timer&);
        
    public:
        
        Timer();
        virtual ~Timer(){}
        
        void reset();
        void start();
        void stop();
        
        unsigned int iGetSec() const;
        unsigned int iGetMSc() const;
        unsigned int iGetTime() const;
        
        string stGetTime() const;
};

#endif
