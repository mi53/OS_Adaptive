#include "os.h"
#include "os_mutex_base.h"

#if (LIC_OS == LIC_LINUX)

#include <sys/time.h>
#include <stdlib.h>
#include <string.h>

typedef struct _OsMutexHSt
{
    //锁本身状态变更的保护互斥量。
    pthread_mutex_t mutex;

    //用于超时等待。
    pthread_cond_t cond;
}OsMutexHSt;

//相对时间转换为相对于系统时间的绝对时间。
static void _Os_GetAbsTime(struct timespec * stAbsTime, unsigned long ulTimeOut);

/***************************************************************************
* 描述: 创建互斥量。
***************************************************************************/
int OS_MutexCreate(const char *pcName, OsMutexHandleSt* pstMutex)
{
    OsMutexHSt stMutexH; 
    int iRet;

    //初始化锁仓库的保护锁。
    iRet = pthread_mutex_init(&stMutexH.mutex, NULL);
    if (iRet != 0)
    {
        pthread_mutex_destroy(&stMutexH.mutex);
        return RET_ERR;
    }

    iRet = pthread_cond_init(&stMutexH.cond, NULL);
    if (iRet != 0)
    {
        pthread_mutex_destroy(&stMutexH.mutex);
        return RET_ERR;
    }

    pstMutex->ulCntThreadId = 0;
    pstMutex->iThreadPVec = 0;
    pstMutex->pOsExHandle = malloc(sizeof(OsMutexHSt));
    memcpy(pstMutex->pOsExHandle, &stMutexH, sizeof(OsMutexHSt));

    return RET_OK;
}

/***************************************************************************
* 描述: 删除互斥量。
***************************************************************************/
int OS_MutexDelete(OsMutexHandleSt* pstMutex)
{
    bool bRet;
    int iRet;
    OsMutexHSt* pstMutexH = (OsMutexHSt*)pstMutex->pOsExHandle;

    iRet = OS_MutexP(pstMutex, 0);
    if (RET_OK != iRet)
        return RET_ERR;

    pthread_mutex_destroy(&pstMutexH->mutex);
    pthread_cond_destroy(&pstMutexH->cond);
    free(pstMutexH);

    pstMutex->pOsExHandle = NULL;
    pstMutex->ulCntThreadId = 0;
    pstMutex->iThreadPVec = 0;

    return RET_OK;
}

/***************************************************************************
* 描述: 请求互斥量。
***************************************************************************/
int OS_MutexP(OsMutexHandleSt* pstMutex, unsigned long ulTimeOutInMillSec)
{
    return OS_MutexPAftP(pstMutex, ulTimeOutInMillSec, NULL);
}


/***************************************************************************
* 描述: 阻塞互斥量。释放前序互斥量并立即P住新互斥量。
*       新互斥量请求成功后又会P住前序互斥量。
***************************************************************************/
int OS_MutexPAftP(OsMutexHandleSt* pstMutex,
                  unsigned long ulTimeOutInMillSec,
                  OsMutexHandleSt* pstBf)
{
    OsMutexHSt* pstMutexH = (OsMutexHSt*)pstMutex->pOsExHandle;
    unsigned long ulThread = pthread_self();
    struct timespec stAbsTimeOut;
    int iRet = RET_OK;

    if (NULL == pstMutexH)
    {
        return RET_ERR;
    }

    //使用互斥量保护。
    pthread_mutex_lock(&pstMutexH->mutex);

    /***********************************************************
    * 释放前序锁。
    ***********************************************************/
    if (pstBf != NULL)
    {
        OS_MutexV(pstBf);
    }
    

   /***********************************************************
    * 1. 支持线程内递归。
    ***********************************************************/
    if (0 == pstMutex->ulCntThreadId
        || ulThread == pstMutex->ulCntThreadId)
    {
        goto LAB_WAIT_END_OK;
    }

   /***********************************************************
    * 2. 无限时等待。
    ***********************************************************/
    if (0 == ulTimeOutInMillSec)
    {
        //进入等待前，会自动释放锁。
        //等待成功会重新加锁。
        iRet = pthread_cond_wait(&pstMutexH->cond, &pstMutexH->mutex);
        if (iRet != 0)
        {
            iRet = RET_ERR;
            goto LAB_WAIT_END_OK;
        }

        //等待成功。
        iRet = RET_OK;
        goto LAB_WAIT_END_OK;
    }

    /***********************************************************
    * 3. 限时等待。
    ***********************************************************/
    _Os_GetAbsTime(&stAbsTimeOut, ulTimeOutInMillSec);

    //进入等待前，会自动释放锁。
    //等待成功会重新加锁。
    iRet = pthread_cond_timedwait(&pstMutexH->cond,
        &pstMutexH->mutex,
        &stAbsTimeOut);
    if (iRet != 0)
    {
        iRet = RET_ERR;
        goto LAB_WAIT_END_OK;
    }

    iRet = RET_OK;
    goto LAB_WAIT_END_OK;

LAB_WAIT_END_OK:

    /***********************************************************
    * 重新请求前序锁。
    ***********************************************************/
    if (pstBf != NULL)
    {
        OS_MutexP(pstBf, 0);
    }

    if (RET_OK == iRet)
    {
        pstMutex->ulCntThreadId = ulThread;
        ++pstMutex->iThreadPVec;
    }

    //获取资源成功或失败均应释放保护锁。
    pthread_mutex_unlock(&pstMutexH->mutex);

    return iRet;
}

/***************************************************************************
* 描述: 释放互斥量。只能由加锁线程释放拥有的锁。
***************************************************************************/
int OS_MutexV(OsMutexHandleSt* pstMutex)
{
    int iRet = RET_ERR;
    OsMutexHSt* pstMutexH = (OsMutexHSt*)pstMutex->pOsExHandle;
    unsigned long ulThread = pthread_self();

    if (NULL == pstMutexH)
    {
        return RET_ERR;
    }

    //使用互斥量保护。
    pthread_mutex_lock(&pstMutexH->mutex);

    if (ulThread == pstMutex->ulCntThreadId)
    {
        iRet = RET_OK;

        //释放互斥量。
        if (pstMutex->iThreadPVec <= 1)
        {
            iRet = pthread_cond_signal(&pstMutexH->cond);
            pstMutex->iThreadPVec = 0;
            pstMutex->ulCntThreadId = 0;
        }
        else
        {
            --pstMutex->iThreadPVec;
        }
    }

    //释放资源成功或失败均应释放保护锁。
    pthread_mutex_unlock(&pstMutexH->mutex);

    return iRet;
}


//相对时间转换为相对于系统时间的绝对时间。
static void _Os_GetAbsTime(struct timespec * stAbsTime, unsigned long ulTimeOut)
{
    struct timeval stCurrentTime;
    unsigned long sec,nsec;

    gettimeofday(&stCurrentTime, NULL);

    nsec = stCurrentTime.tv_usec*1000; /* quotiety of millsecond */
    nsec += (ulTimeOut%1000)*1000000;
    stAbsTime->tv_nsec = nsec%1000000000;  /* quotiety of n second */
    sec = stCurrentTime.tv_sec;
    sec += ulTimeOut/1000;
    sec += nsec/1000000000;
    stAbsTime->tv_sec = sec;

    return;
}

#endif /* OS */
