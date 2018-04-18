#include "def.h"

typedef enum _OsMemLogLevEn
{
    OS_MEM_LOG_LEV_NONE,
    OS_MEM_LOG_LEV_TOTAL,
    OS_MEM_LOG_LEV_DETAIL,
};


/***************************************************************************
* 描述: 分配内存。
***************************************************************************/
void* OS_MemAlloc(unsigned int uiSize);

/***************************************************************************
* 描述: 释放内存。
***************************************************************************/
unsigned int OS_MemFree(void * pAddr);

/***************************************************************************
* 描述: 分配内存。
***************************************************************************/
void* LAD_MemAlloc(unsigned int uiSize);

/***************************************************************************
* 描述: 释放内存。
***************************************************************************/
unsigned int LAD_MemFree(void * pAddr);

/***************************************************************************
* 描述: 启动内存申请释放记录。
***************************************************************************/
void LAD_MemLogStart(int iLevel);

/***************************************************************************
* 描述: 打印内存记录并，关闭内存申请释放记录。
***************************************************************************/
void LAD_MemLogClose();

/***************************************************************************
* 描述: 打印内存记录并清理记录。
***************************************************************************/
void LAD_MemLogCls();

/***************************************************************************
* 描述: 打印内存申请释放记录。
***************************************************************************/
void LAD_MemLogShow();
