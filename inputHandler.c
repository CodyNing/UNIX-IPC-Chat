#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <signal.h>
#include <errno.h>
#include <pthread.h>
#include <unistd.h>

#include "syncList.h"
#include "controller.h"

//thread pid
static pthread_t s_threadPID;

//mutex to solve double free caused by pthread_cancel
static pthread_mutex_t s_mtx = PTHREAD_MUTEX_INITIALIZER;

//pointer to sendlist
static SyncList *s_pSendList;

//pointer to alloc string
static char *s_pMsg;

//thread status
static int s_status = 0;

void *inputHandlerThread(void *unused)
{
    puts("Input Handler Thread initialized successfully");

    //report successfully init
    Controller_threadReportInitStatus(&s_status);

    //a local copy of string for comparison
    char localMsg[MSG_MAX_LEN + 1]; 
    while (1)
    {
        //alloc a string, +1 for \0
        s_pMsg = malloc(MSG_MAX_LEN + 1);
        
        //if fgets EOF break the loop
        if(!fgets(s_pMsg, MSG_MAX_LEN, stdin)){
            break;
        }

        //make sure string end with \0
        s_pMsg[strcspn(s_pMsg, "\n")] = '\0';

        //make a local copy
        strcpy(localMsg, s_pMsg);

        //list put will signal other consumer to get string and free it
        //it shouldn't cancelled by pthread_cancel
        //if the string is pushed into the list and free'd by the consumer thread
        //freeing the same string when thread shutdown will cause double free error and crash the program.
        pthread_mutex_lock(&s_mtx);
        {
            //if put string failed (when list is full and main thread cancel blocking)
            //break the loop
            if (SyncList_put(s_pSendList, s_pMsg) == -1 && errno == ECANCELED)
            {
                break;
            }
            //every string push successfully into the list will be free by the consumer, so set the pointer to NULL
            s_pMsg = NULL;

            //use the local copy to compare with termnite char
            //shouldn't use s_pMsg because the string might have been free'd by the consumer thread
            if (!strcmp(CONTROLLER_C_TERM, localMsg))
            {
                Controller_killMain();
                pthread_mutex_unlock(&s_mtx);
                break;
            }

        }
        pthread_mutex_unlock(&s_mtx);
    }
    return NULL;
}

void InputHandler_init(SyncList *pSendList)
{
    int errCode = 0;
    //store the list pointer
    s_pSendList = pSendList;
    //create and start the thread
    errCode = pthread_create(&s_threadPID, NULL, inputHandlerThread, NULL);
    if (errCode != 0)
	{
		s_status = -1;
		//print error to stderr
		fprintf(stderr, "Thread init failed, error: %s\n", strerror(errCode));
		//report fail status
		Controller_threadReportInitStatus(&s_status);
	}
}

void InputHandler_shutdown(void)
{
    //set running status to -1, indicate thread is shuting down
    s_status = -1;
    
    //cancel is intended to cancel the fgets blocking call, so it shouldn't cancel anything else
    //canceling wrong process will cause memory leak or double free error
    pthread_mutex_lock(&s_mtx);
    {
        pthread_cancel(s_threadPID);
    }
    pthread_mutex_unlock(&s_mtx);
    puts("Input Handler Thread stop listening for input");

    //join the thread.
    pthread_join(s_threadPID, NULL);
    puts("Input Handler Thread shutdown successfully");

    //destory the protection mutex
    pthread_mutex_destroy(&s_mtx);

    //free the string pointer when fgets is cancel but the memory was allocated before that.
    free(s_pMsg);
    s_pMsg = NULL;
}