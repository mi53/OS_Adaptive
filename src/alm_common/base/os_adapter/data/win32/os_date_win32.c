#include "os.h"

#if (LIC_OS == LIC_WINDOWS)

#include <Windows.h>

/***************************************************************************
* 描述: 获取系统时间。
***************************************************************************/
int OS_DateGet(OsDateSt *pstDate)
{
    LPSYSTEMTIME lpSystemTime;

    lpSystemTime = (SYSTEMTIME*)OS_MemAlloc(sizeof(SYSTEMTIME));
    if (NULL == lpSystemTime)
    {
        return RET_ERR;
    }

    GetLocalTime(lpSystemTime);

    pstDate->uwYear = (unsigned short)lpSystemTime->wYear;
    pstDate->ucMonth = (unsigned char)lpSystemTime->wMonth;
    pstDate->ucDate = (unsigned char)lpSystemTime->wDay;
    pstDate->ucHour = (unsigned char)lpSystemTime->wHour;
    pstDate->ucMinute = (unsigned char)lpSystemTime->wMinute;
    pstDate->ucSecond = (unsigned char)lpSystemTime->wSecond;
    pstDate->ucWeek = (unsigned char)lpSystemTime->wDayOfWeek;

    OS_MemFree(lpSystemTime);

    return RET_OK;
}

#endif /* OS */
