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

static int s_status = 0;

void *printThread(void *unused)
{
    puts("Printing Thread initialized successfully");
    Controller_threadReportInitStatus(&s_status);
    while (1)
    {
        char *msg = SyncList_get(s_pPrintList);

        if (!msg && errno == ECANCELED)
        {
            break;
        }

        puts(msg);

        free(msg);
        msg = NULL;
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
    s_status = -1;
    pthread_join(s_threadPID, NULL);
    puts("Printing Thread shutdown successfully");
}