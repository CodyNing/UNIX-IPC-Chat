#include <stdbool.h>
#include <stdio.h>
#include <pthread.h>

#include "controller.h"
#include "syncList.h"

static pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t cv = PTHREAD_COND_INITIALIZER;

static SyncList *s_pThreadInitStatus;

void Controller_init(SyncList *pStatusList)
{
    s_pThreadInitStatus = pStatusList;
}

int Controller_getInitThreadCount()
{
    int i = 0, count = 0;
    while (i < CONTROL_THREAD_NUM)
    {
        int *pStatusCode = SyncList_get(s_pThreadInitStatus);
        if (*pStatusCode != -1)
        {
            ++count;
        }
        ++i;
    }
    return count;
}

int Controller_threadReportInitStatus(int *statusCode)
{
    return SyncList_put(s_pThreadInitStatus, statusCode);
}

int Controller_blockMain()
{
    int res;
    pthread_mutex_lock(&mutex);
    {
        res = pthread_cond_wait(&cv, &mutex);
    }
    pthread_mutex_unlock(&mutex);
    return res;
}

int Controller_killMain()
{
    int res;
    pthread_mutex_lock(&mutex);
    res = pthread_cond_signal(&cv);
    pthread_mutex_unlock(&mutex);
    return res;
}