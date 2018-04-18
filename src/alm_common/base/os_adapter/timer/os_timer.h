#include "def.h"

/***************************************************************************
* 描述: 相关配置宏，及一些基础声明、定义。
***************************************************************************/

//定时器触发中心的触发周期。
#define OS_TIMER_CORE_CRYCLE_SEC 10

//定时器最多支持的数目。
#define OS_TIMER_MAX_NUM         8

//定时器回调任务。
typedef void (*OsTimerTaskFn)(void* pPara);

/***************************************************************************
* 描述: 初始化定时器模块。
***************************************************************************/
int OS_TimerStart();

/***************************************************************************
* 描述: 关闭定时器模块。
***************************************************************************/
int OS_TimerClose();

/***************************************************************************
* 描述: 启动一个周期任务相对定时器。时间间隔单位为毫秒。
***************************************************************************/
int OS_TimerNew(unsigned long *pulTmHandler,
                unsigned long ulLength,
                OsDateSt *pstTime,
                OsTimerTaskFn pfnTmCallBack,
                unsigned long ulMode,
                unsigned long ulType,
                unsigned long ulTimerId);


/***************************************************************************
* 描述: 停止定时器。
***************************************************************************/
int OS_TimerDel(unsigned long ulTmHandler);

/***************************************************************************
* 描述: 增加(iSecs/ALM_TIMER_SEG_SECOND)*ALM_TIMER_SEG_SECOND 的时间量，
*       并同时模拟触发全部定时器。
***************************************************************************/
void OS_TimerRunSecs(int iSecs);

/***************************************************************************
* 描述: 以指定时间间隔进行循环触发的触发器。
***************************************************************************/
int OS_StartTrigger(int iCycleSecs, OsTimerTaskFn fnTrigger, void* pPara);

/***************************************************************************
* 描述: 停止触发器。
***************************************************************************/
void OS_StopTrigger(unsigned long ulTriggerId);