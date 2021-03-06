#include "def.h"


//毫秒、秒单位转换。
#define OS_LOCK_TIME_OUT_UNIT 1000 

//为了方便判断是否死锁，该处配置一个假定的超时死锁时间。
//认为超过这个假定时间，很可能已经出现死锁。单位为毫秒。
#define OS_LOCK_TIME_OUT  (5 * OS_LOCK_TIME_OUT_UNIT)

/***************************************************************************
* 描述: 需要先初始化。不允许在多线程中调用。
***************************************************************************/
int OS_LockStart();

/***************************************************************************
* 描述: 关闭该模块。不允许在多线程中调用。
***************************************************************************/
int OS_LockClose();

/***************************************************************************
* 描述: 创建互斥量。
***************************************************************************/
int OS_LockCreate(const char *pcName, unsigned long *pulSm);

/***************************************************************************
* 描述: 删除互斥量。
***************************************************************************/
int OS_LockDelete(unsigned long ulSmId);

/***************************************************************************
* 描述: 阻塞请求互斥量。
***************************************************************************/
int OS_LockP(unsigned long uiSem, unsigned long uiTimeOutInMillSec);

/***************************************************************************
* 描述: 释放互斥量。只允许拥有者线程释放锁。
***************************************************************************/
int OS_LockV(unsigned long ulSmId);

/***************************************************************************
* 描述: 清理互斥量计数。
* bForce - 是否强制删除已激活Mutex.
* bAll   - 是否清除全部。
* ulSmId - 指定的Mutex编号。
***************************************************************************/
int OS_LockLogCls(bool bForce, bool bAll, unsigned long ulSmId);

/***************************************************************************
* 描述: 打印互斥量使用记录。
***************************************************************************/
int OS_LockLog();
