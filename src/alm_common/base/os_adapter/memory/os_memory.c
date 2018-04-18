#include "os.h"
#include <stdlib.h>
#include <string.h>
#include "util.h"


//数组单次扩展数目。
#define OS_MEM_ARRAY_EXPAND_NUM 10

typedef struct _OsMemSt
{
    const void* pMem;
    int iSize;
}OsMemSt;

typedef struct _OsMemLogSt
{
    OsMemSt* pstMems;
    int iArrayCnt;
    int iArrayCap;

    unsigned int iAllocNum;
    unsigned int iFreeNum;

    int iMemLogLev;
    unsigned int iMemPeak;
    unsigned int iTotalSize;
}OsMemLogSt;

static OsMemLogSt s_stMemLog = {0};

static void _LAD_MemAdd(const void* pMem, int iSize);//在尾部添加
static void _LAD_MemDel(const void* pMem);
static int _LAD_MemGet(const void* pMem); // 返回索引。

//memlog全局变量保护锁。
static unsigned long s_ulMemLogSafe;
#define OS_MEM_LOG_SAFE "OS_MEM_LOG_SAFE"
static bool s_bMemLogEnable = false;

static void _OS_MemLogSafeP();
static void _OS_MemLogSafeV();

/***************************************************************************
* 描述: 分配内存。
***************************************************************************/
void* OS_MemAlloc(unsigned int uiSize)
{
    void* pRet = NULL;

    if (0 != uiSize)
    {
        pRet = malloc(uiSize);
        memset(pRet, 0, uiSize);
    }    
    
    return pRet;
}

/***************************************************************************
* 描述: 释放内存。
***************************************************************************/
unsigned int OS_MemFree(void * pAddr)
{
    if (pAddr != NULL)
    {
        free(pAddr);
    }

    return RET_OK;
}

/***************************************************************************
* 描述: 分配内存。
***************************************************************************/
void* LAD_MemAlloc(unsigned int uiSize)
{
    void* pRet = NULL;

    if (0 != uiSize)
    {
        pRet = OS_MemAlloc(uiSize);
        memset(pRet, 0, uiSize);
    }

    if (s_bMemLogEnable)
    {
        _OS_MemLogSafeP();
        if (s_stMemLog.iMemLogLev > OS_MEM_LOG_LEV_NONE)
        {
            _LAD_MemAdd(pRet, (int)uiSize);

            if (s_stMemLog.iMemLogLev > OS_MEM_LOG_LEV_TOTAL)
            {
                LOG_INFO("alloc %d : addr - 0x%ux: add - %d - %d",
                    s_stMemLog.iAllocNum,
                    pRet,
                    uiSize,
                    s_stMemLog.iTotalSize);
            }
        }
        _OS_MemLogSafeV();
    }
    
    
    return pRet;
}

/***************************************************************************
* 描述: 释放内存。
***************************************************************************/
unsigned int LAD_MemFree(void * pAddr)
{
    if (pAddr != NULL)
    {
        OS_MemFree(pAddr);
    }


    if (s_bMemLogEnable)
    {
        _OS_MemLogSafeP();
        if (s_stMemLog.iMemLogLev > OS_MEM_LOG_LEV_NONE)
        {
            _LAD_MemDel(pAddr);

            if (s_stMemLog.iMemLogLev > OS_MEM_LOG_LEV_TOTAL)
            {
                LOG_INFO("free %d : addr - 0x%ux",
                    s_stMemLog.iFreeNum,
                    pAddr);
            }
        }
        _OS_MemLogSafeV();
    }

    return RET_OK;
}



/***************************************************************************
* 描述: 启动内存申请释放记录。
***************************************************************************/
void LAD_MemLogStart(int iLevel)
{
    int iRet;

    if (s_bMemLogEnable)
        LAD_MemLogClose();

    memset(&s_stMemLog, 0, sizeof(s_stMemLog));
    s_stMemLog.iMemLogLev = iLevel;

    iRet = OS_LockCreate(OS_MEM_LOG_SAFE, &s_ulMemLogSafe);
    UTIL_CALL_FN_RET0(OS_LockCreate, iRet);

    s_bMemLogEnable = true;
}


/***************************************************************************
* 描述: 打印内存记录并，关闭内存申请释放记录。
***************************************************************************/
void LAD_MemLogClose()
{
    int iRet;

    LAD_MemLogCls();

    if (s_bMemLogEnable)
    {
        _OS_MemLogSafeP();

        memset(&s_stMemLog, 0, sizeof(s_stMemLog));

        iRet = OS_LockDelete(s_ulMemLogSafe);
        UTIL_CALL_FN_RET0(OS_LockDelete, iRet);

        s_bMemLogEnable = false;
    }
}

/***************************************************************************
* 描述: 打印内存记录并清理记录。
***************************************************************************/
void LAD_MemLogCls()
{
    if (s_bMemLogEnable)
    {
        int iTmp;

        _OS_MemLogSafeP();

        LAD_MemLogShow();

        //清理内存块记录。
        iTmp = s_stMemLog.iMemLogLev;
        Util_Free(s_stMemLog.pstMems);
        memset(&s_stMemLog, 0, sizeof(s_stMemLog));
        s_stMemLog.iMemLogLev = iTmp;

        _OS_MemLogSafeV();
    }
}

/***************************************************************************
* 描述: 打印内存申请释放记录。
***************************************************************************/
void LAD_MemLogShow()
{
    int idx;

    if (s_bMemLogEnable)
    {
        _OS_MemLogSafeP();

        LOG_INFO("alloc total - %d : free total - %d",
            s_stMemLog.iAllocNum,
            s_stMemLog.iFreeNum);

        //显示未释放内存。
        for (idx = 0; idx < s_stMemLog.iArrayCnt; ++idx)
        {
            if (s_stMemLog.iMemLogLev > OS_MEM_LOG_LEV_TOTAL)

                LOG_INFO("memory not be freed: 0x%ux - %d",
                s_stMemLog.pstMems[idx].pMem,
                s_stMemLog.pstMems[idx].iSize);
        }

        LOG_INFO("memory not be freed total size: %d byte.", s_stMemLog.iTotalSize);
        LOG_INFO("memory used peak size: %d byte.", s_stMemLog.iMemPeak);

        _OS_MemLogSafeV();
    }
}

/////////////////////////////////////////////////////////////////////////////

static void _LAD_MemAdd(const void* pMem, int iSize)
{
    int iExpand;

    //扩展数组。
    if (s_stMemLog.iArrayCnt >= s_stMemLog.iArrayCap)
    {
        iExpand = s_stMemLog.iArrayCnt - s_stMemLog.iArrayCap
            + OS_MEM_ARRAY_EXPAND_NUM;

        Util_ArrayExpand((void**)&s_stMemLog.pstMems,
            sizeof(OsMemSt),
            s_stMemLog.iArrayCnt,
            iExpand);
        s_stMemLog.iArrayCap += iExpand;
    }

    //添加数据。
    s_stMemLog.pstMems[s_stMemLog.iArrayCnt].pMem = pMem;
    s_stMemLog.pstMems[s_stMemLog.iArrayCnt].iSize = iSize;

    ++s_stMemLog.iArrayCnt;
    ++s_stMemLog.iAllocNum;

    s_stMemLog.iTotalSize += iSize;

    if (s_stMemLog.iTotalSize > s_stMemLog.iMemPeak)
        s_stMemLog.iMemPeak = s_stMemLog.iTotalSize;
    
}

static void _LAD_MemDel(const void* pMem)
{
    int idx;

    //查询
    idx = _LAD_MemGet(pMem);
    if (idx < 0)
    {
        return;
    }

    s_stMemLog.iTotalSize -= s_stMemLog.pstMems[idx].iSize;
    

    //删除该项, 后面项往前挪动一位。
    for (++idx; idx < s_stMemLog.iArrayCnt; ++idx)
    {
        s_stMemLog.pstMems[idx - 1].pMem = s_stMemLog.pstMems[idx].pMem;
        s_stMemLog.pstMems[idx - 1].iSize = s_stMemLog.pstMems[idx].iSize;
    }

    s_stMemLog.iArrayCnt--;
    ++s_stMemLog.iFreeNum;    
}

// 返回索引。不存在返回-1.
static int _LAD_MemGet(const void* pMem)
{
    int idx;

    for (idx = 0; idx < s_stMemLog.iArrayCnt; ++idx)
    {
        if (pMem == s_stMemLog.pstMems[idx].pMem)
            break;
    }

    if (idx == s_stMemLog.iArrayCnt)
    {
        return -1;
    }

    return idx;
}


static void _OS_MemLogSafeP()
{
    int iRet;

    if (OS_MEM_LOG_LEV_NONE == s_stMemLog.iMemLogLev)
        return;

    iRet = OS_LockP(s_ulMemLogSafe, 0);
    UTIL_CALL_FN_RET0(OS_MutexP, iRet);
}

static void _OS_MemLogSafeV()
{
    int iRet;

    if (OS_MEM_LOG_LEV_NONE == s_stMemLog.iMemLogLev)
        return;

    iRet = OS_LockV(s_ulMemLogSafe);
    UTIL_CALL_FN_RET0(OS_MutexV, iRet);
}
