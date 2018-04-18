#include "os.h"


/***************************************************************************
* 描述: OS启动。lock -> timer -> simula date
***************************************************************************/
int OS_Start()
{
    OS_LockStart();
    OS_TimerStart();
    OS_DateSimulaStart();

    LAD_MemLogStart(1);

    return RET_OK;
}

/***************************************************************************
* 描述: OS关闭。simula date -> timer -> lock
***************************************************************************/
int OS_Close()
{
    LAD_MemLogClose();
    OS_DateSimulaClose();
    OS_TimerClose();
    OS_LockClose();

    return RET_OK;
}
