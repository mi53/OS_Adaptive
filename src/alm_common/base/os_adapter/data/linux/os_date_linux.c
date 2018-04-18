#include "os.h"

#if (LIC_OS == LIC_LINUX)

#include <time.h>
#include <sys/time.h>

/***************************************************************************
* 描述: 获取系统时间。
***************************************************************************/
int OS_DateGet(OsDateSt *pstDate)
{
    time_t  stSystimep;
    struct tm *pstLocalTime;

    time(&stSystimep);
    pstLocalTime = localtime(&stSystimep);

    pstDate->uwYear = pstLocalTime->tm_year + 1900;
    pstDate->ucMonth = pstLocalTime->tm_mon + 1;
    pstDate->ucDate = pstLocalTime->tm_mday;
    pstDate->ucHour = pstLocalTime->tm_hour;
    pstDate->ucMinute = pstLocalTime->tm_min;
    pstDate->ucSecond = pstLocalTime->tm_sec;

    return RET_OK;
}

#endif /* OS */
