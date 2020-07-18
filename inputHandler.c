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
static pthread_mutex_t s_mtx = PTHREAD_MUTEX_INITIALIZER;

static SyncList *s_pSendList;

static char *s_pMsg;

static int s_status = 0;

void *inputHandlerThread(void *unused)
{
    puts("Input Handler Thread initialized successfully");

    Controller_threadReportInitStatus(&s_status);

    char localMsg[MSG_MAX_LEN + 1]; 
    while (1)
    {
        s_pMsg = malloc(MSG_MAX_LEN + 1);
        
        if(!fgets(s_pMsg, MSG_MAX_LEN, stdin)){
            break;
        }

        s_pMsg[strcspn(s_pMsg, "\n")] = '\0';

        strcpy(localMsg, s_pMsg);

        pthread_mutex_lock(&s_mtx);
        {
            if (SyncList_put(s_pSendList, s_pMsg) == -1 && errno == ECANCELED)
            {
                break;
            }

            if (!strcmp(CONTROLLER_C_TERM, localMsg))
            {
                Controller_killMain();
            }

            s_pMsg = NULL;
        }
        pthread_mutex_unlock(&s_mtx);
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
    s_status = -1;
    
    pthread_mutex_lock(&s_mtx);
    {
        pthread_cancel(s_threadPID);
    }
    pthread_mutex_unlock(&s_mtx);
    puts("Input Handler Thread stop listening for input");

    pthread_join(s_threadPID, NULL);
    puts("Input Handler Thread shutdown successfully");

    pthread_mutex_destroy(&s_mtx);

    free(s_pMsg);
    s_pMsg = NULL;
}