#include "os.h"
#include <string.h>
#include <stdio.h>



/***************************************************************************
* 描述: 全局变量定义。
***************************************************************************/
static unsigned long s_ulDateLock;
static OsDateSt s_stSimulaDate;

#define OS_DATE_SIMULA_LOCK "OS_DATE_SIMULA_LOCK"
#define OS_DATE_SIMULA_TIMER_ID (-1)

/***************************************************************************
* 描述: 获取系统时间。
***************************************************************************/
const char* OS_DateStr()
{
    static char buf[32] = {0};
    OsDateSt stTime;

    OS_DateGet(&stTime);
    sprintf(buf,
        "%04d-%02d-%02d %02d:%02d:%02d",
        stTime.uwYear, 
        stTime.ucMonth, 
        stTime.ucDate, 
        stTime.ucHour, 
        stTime.ucMinute, 
        stTime.ucSecond);

    return buf;
}


/***************************************************************************
* 描述: 模拟时间相关操作。
***************************************************************************/

//启动日期模拟器。
void OS_DateSimulaStart()
{
    unsigned long ulTimerHandle = 0;
    OsDateSt stData;

    OS_LockCreate(OS_DATE_SIMULA_LOCK, &s_ulDateLock);

    /* 初始化系统时间处理 */
    OS_TimerNew(&ulTimerHandle,
        OS_TIMER_CORE_CRYCLE_SEC * 1000,
        0,
        _OS_DateSimulaAuto,
        0,
        0,
        (unsigned long)OS_DATE_SIMULA_TIMER_ID);

    OS_DateGet(&stData);
    OS_DateSimulaSet(&stData);
}


//关闭日期模拟器。
void OS_DateSimulaClose()
{
    OS_TimerDel((unsigned long)OS_DATE_SIMULA_TIMER_ID);
}


//获取当前的模拟时间。
int OS_DateSimulaGet(OsDateSt *pstDate)
{
    if (NULL == pstDate)
    {
        return RET_ERR;
    }
    
    memcpy(pstDate, &s_stSimulaDate, sizeof(s_stSimulaDate));

    return RET_OK;
}

//以字符串形式返回当前模拟时间。
const char* OS_DateSimulaStr()
{
    static char buf[32] = {0};

    sprintf(buf,
        "%04d-%02d-%02d %02d:%02d:%02d",
        s_stSimulaDate.uwYear, 
        s_stSimulaDate.ucMonth, 
        s_stSimulaDate.ucDate, 
        s_stSimulaDate.ucHour, 
        s_stSimulaDate.ucMinute, 
        s_stSimulaDate.ucSecond);

    return buf;
}

//增加指定秒数到模拟时间。
const char* OS_DateSimulaAddSecs(int iSecs)
{
    int iTmp;

    OS_LockP(s_ulDateLock, 0);

    //优化计算，转换成天、时、分、秒。
    iTmp = iSecs / (24 * 3600);
    while (iTmp-- > 0)
    {
        _OS_DateSimulaAddDay(&s_stSimulaDate);
    }

    iTmp = iSecs / 3600 % 24;
    while (iTmp-- > 0)
    {
        _OS_DateSimulaAddHour(&s_stSimulaDate);
    }

    iTmp = iSecs / 60 % 60;
    while (iTmp-- > 0)
    {
        _OS_DateSimulaAddMinitue(&s_stSimulaDate);
    }

    iTmp = iSecs % 60;
    while (iTmp-- > 0)
    {
        _OS_DateSimulaAddSecond(&s_stSimulaDate);
    }

    OS_LockV(s_ulDateLock);

    return OS_DateSimulaStr();
}


//设置模拟时间。
int OS_DateSimulaSet(OsDateSt* pstNewTime)
{
    //简单校验时间有效性。
    if (NULL == pstNewTime
        || pstNewTime->ucMonth > 12
        || pstNewTime->ucDate > 31
        || pstNewTime->ucHour > 24
        || pstNewTime->ucMinute > 60
        || pstNewTime->ucSecond > 60)
    {
        return RET_ERR;
    }
    

    //更新时间。
    if (0 != pstNewTime)
    {
        OS_LockP(s_ulDateLock, 0);

        memcpy(&s_stSimulaDate, pstNewTime, sizeof(OsDateSt));

        OS_LockV(s_ulDateLock);

        return RET_OK;
    }

    return RET_ERR;
}


/***************************************************************************
* 描述: 内部函数。
***************************************************************************/
static OsDateSt* _OS_DateSimulaAddMonth(OsDateSt* pstDate)
{
    pstDate->ucMonth = (pstDate->ucMonth >= 13)
        ? (pstDate->ucMonth % 13) : pstDate->ucMonth;
    pstDate->ucMonth = (pstDate->ucMonth == 0)
        ? 1 : pstDate->ucMonth;

    ++(pstDate->ucMonth);
    if (13 == pstDate->ucMonth)
    {
        pstDate->ucMonth = 1;
        ++(pstDate->uwYear);
    }

    return pstDate;
}

static OsDateSt* _OS_DateSimulaAddDay(OsDateSt* pstDate)
{
    int iCntMonDays = 0;

    pstDate->ucMonth = (pstDate->ucMonth >= 13)
        ? (pstDate->ucMonth % 13) : pstDate->ucMonth;
    pstDate->ucMonth = (pstDate->ucMonth == 0)
        ? 1 : pstDate->ucMonth;

    ++(pstDate->ucDate);

    switch (pstDate->ucMonth)
    {
    case 2:
        if (0 == pstDate->uwYear % 400
            || ((0 == pstDate->uwYear % 4) && (0 != pstDate->uwYear % 100)))
            iCntMonDays = 29;
        else
            iCntMonDays = 28;
        break;

    case 1: case 3: case 5: case 7: case 8: case 10: case 12:
        iCntMonDays = 31;
        break;

    case 4: case 6: case 9: case 11:
        iCntMonDays = 30;
        break;
    }

    if ((iCntMonDays+1) == pstDate->ucDate)
    {
        pstDate->ucDate = 1;
        _OS_DateSimulaAddMonth(pstDate);
    }

    return pstDate;
}

static OsDateSt* _OS_DateSimulaAddHour(OsDateSt* pstDate)
{
    pstDate->ucHour = (pstDate->ucHour >= 24)
        ? (pstDate->ucHour % 24) : pstDate->ucHour;

    ++(pstDate->ucHour);
    if (24 == pstDate->ucHour)
    {
        pstDate->ucHour = 0;
        _OS_DateSimulaAddDay(pstDate);
    }

    return pstDate;
}

static OsDateSt* _OS_DateSimulaAddMinitue(OsDateSt* pstDate)
{
    pstDate->ucMinute = (pstDate->ucMinute >= 60)
        ? (pstDate->ucMinute % 60) : pstDate->ucMinute;

    ++(pstDate->ucMinute);
    if (60 == pstDate->ucMinute)
    {
        pstDate->ucMinute = 0;
        _OS_DateSimulaAddHour(pstDate);
    }

    return pstDate;
}

static OsDateSt* _OS_DateSimulaAddSecond(OsDateSt* pstDate)
{
    pstDate->ucSecond = (pstDate->ucSecond >= 60)
        ? (pstDate->ucSecond % 60) : pstDate->ucSecond;

    ++(pstDate->ucSecond);
    if (60 == pstDate->ucSecond)
    {
        pstDate->ucSecond = 0;
        _OS_DateSimulaAddMinitue(pstDate);
    }

    return pstDate;
}

//定时器自动累加时间。
static void _OS_DateSimulaAuto(void* pPara)
{
    (void)pPara;
    OS_DateSimulaAddSecs(OS_TIMER_CORE_CRYCLE_SEC);
}
