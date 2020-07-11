#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <signal.h>
#include <errno.h>
#include <pthread.h>
#include <unistd.h>

#include "syncList.h"
#include "controller.h"

static pthread_t s_threadPID;

static SyncList *s_pSendList;

void *inputHandlerThread(void *unused)
{
    int status = 0;
    printf("Input Handler Thread initialized successfully\n");
    Controller_threadReportInitStatus(&status);
    while (1)
    {
        char msg[MSG_MAX_LEN];
        scanf("%s", msg);

        if (!strcmp(CONTROLLER_C_TERM, msg))
        {
            Controller_killMain();
        }

        if (SyncList_put(s_pSendList, msg) == -1 && errno == ECANCELED)
        {
            break;
        }
    }
    return NULL;
}

void InputHandler_init(SyncList *pSendList)
{
    s_pSendList = pSendList;
    pthread_create(&s_threadPID, NULL, inputHandlerThread, NULL);
}

void InputHandler_shutdown(void)
{
    pthread_cancel(s_threadPID);
    printf("Input Handler Thread stop listening for input\n");

    pthread_join(s_threadPID, NULL);
    printf("Input Handler Thread shutdown successfully\n");
}