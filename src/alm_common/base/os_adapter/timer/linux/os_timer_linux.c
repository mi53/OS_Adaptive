#include "os.h"

#if (LIC_OS == LIC_LINUX)

#include <signal.h>
#include <sys/time.h>

/***************************************************************************
* 描述: 将触发器包装成linux格式。
***************************************************************************/
typedef struct _OsTriggerTaskSt
{
    OsTimerTaskFn fnTask;
    void* pPara;
}OsTriggerTaskSt;

static OsTriggerTaskSt s_stTriggerTask;

static void _OS_TriggerTask(int iSig)
{
    if (NULL != s_stTriggerTask.fnTask)
    {
        s_stTriggerTask.fnTask(s_stTriggerTask.pPara);
    }
}
/***************************************************************************
* 描述: 以指定时间间隔进行循环触发的触发器。
***************************************************************************/
int OS_StartTrigger(int iCycleSecs, OsTimerTaskFn fnTrigger, void* pPara)
{    
    struct itimerval stItimerVal;

	s_stTriggerTask.fnTask = fnTrigger;
    s_stTriggerTask.pPara = pPara;
/*
    {
    struct sigaction stSigact; 
    stSigact.sa_handler = _OS_TriggerTask;
    stSigact.sa_flags = 0;
    sigemptyset(&stSigact.sa_mask);
    sigaction(SIGALRM, &stSigact, NULL);
    }
*/
	signal(SIGALRM, _OS_TriggerTask);
 
	
    /*quotiety of millsecond*/
    stItimerVal.it_value.tv_sec = OS_TIMER_CORE_CRYCLE_SEC;
    stItimerVal.it_value.tv_usec = 0;
    stItimerVal.it_interval = stItimerVal.it_value;

    setitimer(ITIMER_REAL, &stItimerVal, NULL);

    return 0;
}

/***************************************************************************
* 描述: 停止触发器。
***************************************************************************/
void OS_StopTrigger(unsigned long ulTriggerId)
{
    struct sigaction stSigact; 

    stSigact.sa_handler = NULL;
    stSigact.sa_flags   = 0;
    sigemptyset(&stSigact.sa_mask);
    sigaction(SIGALRM, &stSigact, NULL);
}

#endif /* OS */
