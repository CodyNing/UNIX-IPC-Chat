#include <stdbool.h>
#include <stdio.h>
#include <pthread.h>

#include "controller.h"
#include "syncList.h"

static SyncList *s_pThreadInitSignals;
static SyncList *s_pThreadKillSignal;
static int s_killSignal = CONTROL_KILL_SIGNAL;

void Controller_init()
{
    s_pThreadInitSignals = SyncList_init(CONTROL_THREAD_NUM);
    s_pThreadKillSignal = SyncList_init(1);
}

int Controller_getInitThreadCount()
{
    int i = 0, count = 0;
    while (i < CONTROL_THREAD_NUM)
    {
        int *pSignal = SyncList_get(s_pThreadInitSignals);
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
    return SyncList_put(s_pThreadInitSignals, statusCode);
}

int Controller_blockMain()
{
    int *signal = SyncList_get(s_pThreadKillSignal);
    return *signal;
}

int Controller_killMain()
{
    return SyncList_put(s_pThreadKillSignal, &s_killSignal);
}

void Controller_shutdown()
{
    SyncList_cancelBlocking(s_pThreadInitSignals);
    SyncList_free(s_pThreadInitSignals);
    SyncList_cancelBlocking(s_pThreadKillSignal);
    SyncList_free(s_pThreadKillSignal);
}