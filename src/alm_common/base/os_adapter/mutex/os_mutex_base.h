#ifndef _os_mutex_base_h_
#define _os_mutex_base_h_

#include "def.h"

typedef struct _OsMutexHandleSt
{
    void* pOsExHandle;
    unsigned long ulCntThreadId;
    int iThreadPVec;//线程内P-V的次数。
}OsMutexHandleSt;

/***************************************************************************
* 描述: 创建互斥量。
***************************************************************************/
int OS_MutexCreate(const char *pcName, OsMutexHandleSt* pstMutex);

/***************************************************************************
* 描述: 删除互斥量。
***************************************************************************/
int OS_MutexDelete(OsMutexHandleSt* pstMutex);

/***************************************************************************
* 描述: 阻塞请求互斥量。
***************************************************************************/
int OS_MutexP(OsMutexHandleSt* pstMutex, unsigned long ulTimeOutInMillSec);

/***************************************************************************
* 描述: 释放互斥量。
*****************************************************.**********************/
int OS_MutexV(OsMutexHandleSt* pstMutex);

/***************************************************************************
* 描述: 阻塞互斥量。释放前序互斥量并立即P住新互斥量。
*       新互斥量请求成功后又会P住前序互斥量。
***************************************************************************/
int OS_MutexPAftP(OsMutexHandleSt* pstMutex,
                  unsigned long ulTimeOutInMillSec,
                  OsMutexHandleSt* pstBf);


#endif // _os_mutex_base_h_
