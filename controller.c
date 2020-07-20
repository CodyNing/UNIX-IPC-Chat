#include <stdbool.h>
#include <stdio.h>
#include <pthread.h>

#include "controller.h"
#include "syncList.h"

//signal queue for init status
static SyncList *s_pThreadInitSignals;
//signal queue for kill signal
static SyncList *s_pThreadKillSignal;
//a kill signal
static int s_killSignal = CONTROL_KILL_SIGNAL;

void Controller_init()
{
    //init both queue
    s_pThreadInitSignals = SyncList_init(CONTROL_THREAD_NUM);
    s_pThreadKillSignal = SyncList_init(1);
}

int Controller_getInitThreadCount()
{
    int i = 0, count = 0;

    //get init status from 4 threads
    while (i < CONTROL_THREAD_NUM)
    {
        //blocking call
        //only gets the signal when queue is non empty
        int *pSignal = SyncList_get(s_pThreadInitSignals);
        //count sucessfull init threads
        if (*pSignal == CONTROL_INIT_SUCCESS_SIGNAL)
        {
            ++count;
        }
        ++i;
    }
    return count;
}

int Controller_threadReportInitStatus(int *statusCode)
{
    //push status into the init signal queue
    return SyncList_put(s_pThreadInitSignals, statusCode);
}

int Controller_blockMain()
{
    //use a blocking call to get kill signal
    int *signal;
    do
    {
        signal = SyncList_get(s_pThreadKillSignal);
    } while (*signal != CONTROL_KILL_SIGNAL);
    return *signal;
}

int Controller_killMain()
{
    //push a kill signal into queue
    return SyncList_put(s_pThreadKillSignal, &s_killSignal);
}

void Controller_shutdown()
{
    //cancel blocking for both queue and free them
    SyncList_cancelBlocking(s_pThreadInitSignals);
    SyncList_free(s_pThreadInitSignals);
    SyncList_cancelBlocking(s_pThreadKillSignal);
    SyncList_free(s_pThreadKillSignal);
}