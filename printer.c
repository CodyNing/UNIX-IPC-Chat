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

static SyncList *s_pPrintList;

void* printThread(void* unused)
{
    int status = 0;
    printf("Printing Thread initialized successfully\n");
    Controller_threadReportInitStatus(&status);
    while (1)
    {
        char *msg = SyncList_get(s_pPrintList);

        if (!msg && errno == ECANCELED)
		{
			break;
		}

        printf("%s\n", msg);
    }
    return NULL;
}

void Printer_init(SyncList *pPrintList)
{
    s_pPrintList = pPrintList;
    pthread_create(&s_threadPID, NULL, printThread, NULL);
}

void Printer_shutdown(void)
{
    pthread_join(s_threadPID, NULL);
    printf("Printing Thread shutdown successfully\n");
}