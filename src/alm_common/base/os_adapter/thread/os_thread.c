/**************************************************************************
	purpose  :	适配不同操作系统的线程处理函数。
***************************************************************************/
#include "os.h"//在os子组件的实现文件中，都需要先包含该文件的系统配置。

#include "base.h"
#include <stdio.h>
#include "util.h"


/***************************************************************************
* 描述: 启动一个新的线程。
***************************************************************************/
HThread thread_start(thread_fn threadfn, void* pData)
{
    HThread hThread;

#if (LIC_WINDOWS == LIC_OS)
    hThread = CreateThread(0,0,(LPTHREAD_START_ROUTINE)threadfn,pData,0,0);
#elif (LIC_LINUX == LIC_OS)
    int err;
    err = pthread_create(&hThread,0,threadfn,pData);
    if (err != 0)
    {
        LOG("ERROR: create thread failure.\n");
    }
#endif

    return hThread;
}

/***************************************************************************
* 描述: 等待指定线程运行结束。
***************************************************************************/
void thread_wait(HThread* pThread)
{
    if (NULL == pThread)
    {
        LOG_ERR("Thread handle is null.");
        return;
    }
    
#if (LIC_WINDOWS == LIC_OS)
    WaitForSingleObject(*pThread, INFINITE);
#elif (LIC_LINUX == LIC_OS)
    void* retVal = 0;
    pthread_join(*pThread, &retVal);
#endif
}
