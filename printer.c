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

//pointer to print list
static SyncList *s_pPrintList;

//thread status
static int s_status = 0;

void *printThread(void *unused)
{
    puts("Printing Thread initialized successfully");
    //report successfully init
    Controller_threadReportInitStatus(&s_status);
    while (1)
    {
        //blocking call to get next string
        char *msg = SyncList_get(s_pPrintList);

        //if got a null pointer and blocking is canceled then break the loop
        if (!msg && errno == ECANCELED)
        {
            break;
        }

        //print the string
        puts(msg);

        //free the string
        free(msg);
        msg = NULL;
    }
    return NULL;
}

void Printer_init(SyncList *pPrintList)
{
    int errCode = 0;
    //store the list pointer
    s_pPrintList = pPrintList;
    //create the start the thread
    errCode = pthread_create(&s_threadPID, NULL, printThread, NULL);
    if (errCode != 0)
	{
		s_status = -1;
		//print error to stderr
		fprintf(stderr, "Thread init failed, error: %s\n", strerror(errCode));
		//report fail status
		Controller_threadReportInitStatus(&s_status);
	}
}

void Printer_shutdown(void)
{
    //set status to shutdown
    s_status = -1;
    //join the thread
    pthread_join(s_threadPID, NULL);
    puts("Printing Thread shutdown successfully");
}