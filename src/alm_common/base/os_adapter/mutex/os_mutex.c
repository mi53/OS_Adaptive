#include "os.h"
#include "os_mutex.h"
#include "os_mutex_base.h"
#include "util.h"
#include <stdlib.h>
#include <string.h>

typedef struct _OsLockInfoSt
{
    OsMutexHandleSt stLockHandle;

    char* pcName;
    long lPCount;
    long lPOkCount;
    long lVCount;
    long lVOkCount;
    bool bIsAct;

    bool bNotFreeNode; //存储锁的结点是否有效，空闲:无效。
    //OsMutexHandleSt stSafeLock;
}OsLockInfoSt;

static OsLockInfoSt s_stOsLockSafe;
static OsLockInfoSt* s_pstLockList;
static int s_iLockCapT;

/***************************************************************************
* 描述: 记录器相关操作函数。
***************************************************************************/
static int _OS_LockLogGetFree(); //返回索引，失败时返回-1。
static int _OS_LockLogGetByName(const char* pcLockName); //返回索引，失败时返回-1。
static OsLockInfoSt* _OS_LockLogGetById(unsigned long ulSmId); //返回有效对象。
static int _OS_MutexP(OsLockInfoSt* pstLock, unsigned long ulTimeOutInMillSec);
static int _OS_MutexPAft(OsLockInfoSt* pstLock, unsigned long ulTimeOutInMillSec, OsLockInfoSt* pstBf);

#define LOCK_SAFE_P() \
    iRet = _OS_MutexP(&s_stOsLockSafe, 0);\
    UTIL_CALL_FN_RET("_OS_MutexP", iRet)

#define LOCK_SAFE_V() \
    iRet = OS_MutexV(&s_stOsLockSafe.stLockHandle);\
    UTIL_CALL_FN_RET("OS_MutexV", iRet)
/***************************************************************************
* 描述: 需要先初始化。不允许在多线程中调用。
***************************************************************************/
int OS_LockStart()
{
    int iRet;

    iRet = OS_MutexCreate("Lock_Log_Saft_Mutex", &s_stOsLockSafe.stLockHandle);
    UTIL_CALL_FN_RET("OS_MutexCreate", iRet);

    s_stOsLockSafe.pcName = "s_stOsLockSafe";

    return iRet;
}

/***************************************************************************
* 描述: 关闭该模块。不允许在多线程中调用。
***************************************************************************/
int OS_LockClose()
{
    int iRet;
    int itr;

    //记录器保护。
    LOCK_SAFE_P();

    //清除记录。
    for (itr = 0; itr < s_iLockCapT; ++itr)
    {
        if (s_pstLockList[itr].bNotFreeNode)
        {
            if (s_pstLockList[itr].bIsAct)
            {
                iRet = OS_MutexDelete(&s_pstLockList[itr].stLockHandle);
                UTIL_CALL_FN_LAB("OS_MutexDelete", iRet, LAB_END);
            }
            Util_Free(s_pstLockList[itr].pcName);
        }
        memset(s_pstLockList + itr, 0, sizeof(OsLockInfoSt));
    }
    Util_Free(s_pstLockList);
    s_pstLockList = NULL;

    //删除记录器保护互斥量。
    iRet = OS_MutexDelete(&s_stOsLockSafe.stLockHandle);
    UTIL_CALL_FN_LAB("OS_MutexDelete", iRet, LAB_END);

    return iRet;

LAB_END:
    LOCK_SAFE_V();

    return iRet;

}

/***************************************************************************
* 描述: 创建互斥量。
***************************************************************************/
int OS_LockCreate(const char *pcName, unsigned long *pulSm)
{
    int iRet;
    OsLockInfoSt* pstNewLock = NULL;
    int iLockIdx;

    LOCK_SAFE_P();

    //判断是否存在。
    iLockIdx = _OS_LockLogGetByName(pcName);

    //创建记录。
    if (iLockIdx < 0)
    {
        iLockIdx = _OS_LockLogGetFree();
        UTIL_CHK(iLockIdx >= 0);
        
        pstNewLock = s_pstLockList + iLockIdx;
        memset(pstNewLock, 0, sizeof(OsLockInfoSt));

        pstNewLock->pcName = Util_StrCpy(pcName);
        pstNewLock->bIsAct = false;
        pstNewLock->bNotFreeNode = true;
    }
    else
        pstNewLock = s_pstLockList + iLockIdx;

    //创建新的锁。
    if (!pstNewLock->bIsAct)
    {
        iRet = OS_MutexCreate(pcName, &pstNewLock->stLockHandle);
        UTIL_CALL_FN_LAB("OS_MutexCreate", iRet, LAB_END);

        pstNewLock->bIsAct = true;
    }
    else
        LOG_INFO("Create a actived mutex: %s.", pstNewLock->pcName);

    *pulSm = iLockIdx;

LAB_END:
    LOCK_SAFE_V();

    return iRet;
}

/***************************************************************************
* 描述: 删除锁。不清理记录。
***************************************************************************/
int OS_LockDelete(unsigned long ulSemId)
{
    int iRet;
    OsLockInfoSt* pstLock = NULL;

    LOCK_SAFE_P();

    pstLock = _OS_LockLogGetById(ulSemId);
    if (NULL == pstLock)
    {
        iRet = RET_ERR;
        goto LAB_END;
    }    

    //删除。
    iRet = OS_MutexDelete(&pstLock->stLockHandle);
    UTIL_CALL_FN_LAB("OS_MutexDelete", iRet, LAB_END);

    //只更改活动状态，不改变其他记录。
    pstLock->bIsAct = false;

LAB_END:
    LOCK_SAFE_V();

    return iRet;
}

/***************************************************************************
* 描述: 请求互斥量。
***************************************************************************/
int OS_LockP(unsigned long ulSemId, unsigned long ulTimeOutInMillSec)
{
    OsLockInfoSt* pstLock = NULL;
    int iRet;
    
    //更新锁记录。
    LOCK_SAFE_P();

    pstLock = _OS_LockLogGetById(ulSemId);
    if (NULL == pstLock)
    {
        iRet = RET_ERR;
        goto LAB_END;
    }
    ++pstLock->lPCount;

    //加锁。 
    iRet = _OS_MutexPAft(pstLock, ulTimeOutInMillSec, &s_stOsLockSafe);
    UTIL_CALL_FN_RET("_OS_MutexP", iRet);

    //重新获得管理锁，数据可能被更新。
    pstLock = _OS_LockLogGetById(ulSemId);
    if (NULL == pstLock)
    {
        //锁记录被删除。
        iRet = RET_ERR;
        goto LAB_END;
    }
    ++pstLock->lPOkCount;

LAB_END:

    if (RET_OK != iRet)
        LOG_INFO("lock %s fial.", pstLock->pcName);
    
    LOCK_SAFE_V();

    return iRet;
}

/***************************************************************************
* 描述: 释放互斥量。只允许拥有者线程释放锁。
***************************************************************************/
int OS_LockV(unsigned long ulSmId)
{
    int iRet;
    OsLockInfoSt* pstLock = s_pstLockList;

    //锁保护。
    LOCK_SAFE_P();

    pstLock = _OS_LockLogGetById(ulSmId);
    if (NULL == pstLock)
    {
        iRet = RET_ERR;
        goto LAB_END;
    }
   
    ++pstLock->lVCount;

    //解锁。
    iRet = OS_MutexV(&pstLock->stLockHandle);    
    UTIL_CALL_FN_LAB("OS_MutexV", iRet, LAB_END);
    ++pstLock->lVOkCount;

LAB_END:

    if (RET_OK != iRet)
        LOG_INFO("V lock %s fial.", pstLock->pcName);
    
    LOCK_SAFE_V();
    return iRet;
}

/***************************************************************************
* 描述: 清理互斥量计数。
* bForce - 是否强制删除已激活Mutex.
* bAll   - 是否清除全部。
* ulSmId - 指定的Mutex编号。
***************************************************************************/
int OS_LockLogCls(bool bForce, bool bAll, unsigned long ulSmId)
{
    int iRet;
    int idx;
    OsLockInfoSt* pstLock = NULL;

    LOCK_SAFE_P();

    //清除指定。
    if (!bAll)
    {
        pstLock = _OS_LockLogGetById(ulSmId);
        if (NULL != pstLock
            && pstLock->bNotFreeNode
            && (bForce || !pstLock->bIsAct))
        {
            pstLock->lPCount = 0;
            pstLock->lPOkCount = 0;
            pstLock->lVCount = 0;
            pstLock->lVOkCount = 0;
        }
    }
    else //清除全部。
    {
        for (idx = 0; idx < s_iLockCapT; ++idx)
        {
            pstLock = s_pstLockList + idx;

            if (pstLock->bNotFreeNode
                && (bForce || !pstLock->bIsAct))
            {
                pstLock->lPCount = 0;
                pstLock->lPOkCount = 0;
                pstLock->lVCount = 0;
                pstLock->lVOkCount = 0;
            }
        }
    }

    LOCK_SAFE_V();

    return RET_OK;
}

/***************************************************************************
* 描述: 打印互斥量使用记录。
***************************************************************************/
int OS_LockLog()
{
    int iRet;
    int idx;
    OsLockInfoSt* pstLock = NULL;

    LOCK_SAFE_P();

    LOG("%-20s %-2s %-5s %-6s %-6s %-6s %-6s\n",
        "Name",
        "Id",
        "Valid",
        "P Tol",
        "V Tol",
        "P OK",
        "V OK");
    LOG("%-20s %-2s %-5s %-6s %-6s %-6s %-6s\n",
        "====================",
        "==",
        "=====",
        "======",
        "======",
        "======",
        "======");

    for (idx = 0; idx < s_iLockCapT; ++idx)
    {
        pstLock = s_pstLockList + idx;

        if (pstLock->bNotFreeNode)
        {
            LOG("%-20s %-2u %-5s %-6d %-6d %-6d %-6d\n",
                pstLock->pcName,
                idx,
                pstLock->bIsAct ? "yes" : "no",
                pstLock->lPCount,
                pstLock->lVCount,
                pstLock->lPOkCount,
                pstLock->lVOkCount);
        }        
    }

    LOCK_SAFE_V();

    return RET_OK;
}

//////////////////////////////////////////////////////////////////////////////

static int _OS_LockLogGetFree()
{
    int idx;
    OsLockInfoSt* pstNew = NULL;

    for (idx = 0; idx < s_iLockCapT; ++idx)
    {
        if (!s_pstLockList[idx].bNotFreeNode)
        {
            return idx;
        }
    }

    //扩容。
    pstNew = (OsLockInfoSt*)malloc(sizeof(OsLockInfoSt) * (2 * s_iLockCapT + 1));
    memset(pstNew, 0, sizeof(OsLockInfoSt) * (2 * s_iLockCapT + 1));
    if (s_iLockCapT > 0)
        memcpy(pstNew, s_pstLockList, sizeof(OsLockInfoSt) * s_iLockCapT);
    Util_Free(s_pstLockList);
    s_pstLockList = pstNew;
    s_iLockCapT = 2 * s_iLockCapT + 1;

    return idx;
}

static int _OS_LockLogGetByName(const char* pcLockName)
{
    int idx;
    int iCmp;

    for (idx = 0; idx < s_iLockCapT; ++idx)
    {
        if (s_pstLockList[idx].bNotFreeNode)
        {
            iCmp = strcmp(pcLockName, s_pstLockList[idx].pcName);
            if (0 == iCmp)
            {
                return idx;
            }
        }
    }

    return -1;
}

static OsLockInfoSt* _OS_LockLogGetById(unsigned long ulSmId)
{
    if (ulSmId < (unsigned int)s_iLockCapT
        && s_pstLockList[ulSmId].bNotFreeNode)
    {
        return s_pstLockList + ulSmId;
    }
    
    return NULL;
}

static int _OS_MutexP(OsLockInfoSt* pstLock, unsigned long ulTimeOutInMillSec)
{
    unsigned long ulLeft = 0;
    int iRet = RET_OK;

    if (0 == ulTimeOutInMillSec || ulTimeOutInMillSec > OS_LOCK_TIME_OUT)
    {
        iRet = OS_MutexP(&(pstLock->stLockHandle), OS_LOCK_TIME_OUT);
        if (RET_OK != iRet)
        {
            LOG_INFO("lock %s - %d timeout %d secs.",
                pstLock->pcName,
                pstLock->stLockHandle.ulCntThreadId,
                OS_LOCK_TIME_OUT / OS_LOCK_TIME_OUT_UNIT);

            if (ulTimeOutInMillSec > OS_LOCK_TIME_OUT)
                ulLeft = ulTimeOutInMillSec - OS_LOCK_TIME_OUT;
            
            iRet = OS_MutexP(&(pstLock->stLockHandle), ulLeft);
            UTIL_CALL_FN_RET("_OS_MutexP", iRet);
        }
    }
    else
    {
        iRet = OS_MutexP(&(pstLock->stLockHandle), ulTimeOutInMillSec);
        UTIL_CALL_FN_RET("_OS_MutexP", iRet);
    }

    return RET_OK;
}

static int _OS_MutexPAft(OsLockInfoSt* pstLock,
                         unsigned long ulTimeOutInMillSec,
                         OsLockInfoSt* pstBf)
{
    unsigned long ulLeft = 0;
    int iRet = RET_OK;

    if (0 == ulTimeOutInMillSec || ulTimeOutInMillSec > OS_LOCK_TIME_OUT)
    {
        iRet = OS_MutexPAftP(&(pstLock->stLockHandle),
            OS_LOCK_TIME_OUT,
            &(pstBf->stLockHandle));
        if (RET_OK != iRet)
        {
            LOG_INFO("lock %s - %d timeout %d secs.",
                pstLock->pcName,
                pstLock->stLockHandle.ulCntThreadId,
                OS_LOCK_TIME_OUT / OS_LOCK_TIME_OUT_UNIT);

            if (ulTimeOutInMillSec > OS_LOCK_TIME_OUT)
                ulLeft = ulTimeOutInMillSec - OS_LOCK_TIME_OUT;

            iRet = OS_MutexP(&(pstLock->stLockHandle), ulLeft);
            UTIL_CALL_FN_RET("_OS_MutexP", iRet);
        }
    }
    else
    {
        iRet = OS_MutexPAftP(&(pstLock->stLockHandle),
            ulTimeOutInMillSec,
            &(pstBf->stLockHandle));
        UTIL_CALL_FN_RET("_OS_MutexP", iRet);
    }

    return RET_OK;
}
