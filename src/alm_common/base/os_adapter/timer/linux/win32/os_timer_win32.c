#include "os.h"

#if (LIC_OS == LIC_WINDOWS)

#include <windows.h>


#pragma comment(lib,"Winmm.lib")


/***************************************************************************
* 描述: 将触发器包装成windows格式。
***************************************************************************/
typedef struct _OsTriggerTaskSt
{
    OsTimerTaskFn fnTask;
    void* pPara;
}OsTriggerTaskSt;

static void CALLBACK _OS_TriggerTask(UINT uTimerID,
                                     UINT uMsg,
                                     DWORD_PTR dwUser,
                                     DWORD_PTR dw1,
                                     DWORD_PTR dw2)
{
    OsTriggerTaskSt* pstPara = (OsTriggerTaskSt*)dwUser;
    
    if (NULL != pstPara && NULL != pstPara->fnTask)
    {
        pstPara->fnTask(pstPara->pPara);
    }

    (void)dw1;
    (void)dw2;
    (void)uMsg;
    (void)uTimerID;
}

/***************************************************************************
* 描述: 以指定时间间隔进行循环触发的触发器。
***************************************************************************/
int OS_StartTrigger(int iCycleSecs, OsTimerTaskFn fnTrigger, void* pPara)
{
    unsigned long ulTimerId;
    static OsTriggerTaskSt stTask;

    stTask.fnTask = fnTrigger;
    stTask.pPara = pPara;
    ulTimerId = timeSetEvent(
        1000 * iCycleSecs,
        1000,
        _OS_TriggerTask,
        (DWORD_PTR)&stTask,
        TIME_PERIODIC);

    return ulTimerId;
}

/***************************************************************************
* 描述: 停止触发器。
***************************************************************************/
void OS_StopTrigger(unsigned long ulTriggerId)
{
    timeKillEvent((UINT)ulTriggerId);
}

#endif /* OS */
