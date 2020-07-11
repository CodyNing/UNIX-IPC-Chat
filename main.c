#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "syncList.h"
#include "reciever.h"
#include "sender.h"
#include "printer.h"
#include "inputHandler.h"
#include "controller.h"

int main(int argc, char** args){
    assert(argc = 4);

    char *arg_localPort = args[1];
    char *arg_remoteMacName = args[2];
    char *arg_remotePort = args[3];

    int localPort = atoi(arg_localPort), initThreadCount;

    printf("s-talk initializing...\n");

    SyncList *s_pSendList = SyncList_init(LIST_MAX_NUM_NODES / 3);
    SyncList *s_pPrintList = SyncList_init(LIST_MAX_NUM_NODES / 3);
    SyncList *s_pThreadInitStatus = SyncList_init(CONTROL_THREAD_NUM);

    Controller_init(s_pThreadInitStatus);

    Printer_init(s_pPrintList);
    Receiver_init(localPort, s_pPrintList);
    Sender_init(arg_remoteMacName, arg_remotePort, s_pSendList);
    InputHandler_init(s_pSendList);

    initThreadCount = Controller_getInitThreadCount();

    if(initThreadCount == CONTROL_THREAD_NUM)
    {
        printf("Main thread is now on blocking...\n");
        Controller_blockMain();
    }

    printf("Terminating all threads...\n");
    SyncList_cancelBlocking(s_pThreadInitStatus);
    SyncList_free(s_pThreadInitStatus);

    SyncList_cancelBlocking(s_pSendList);
    SyncList_cancelBlocking(s_pPrintList);

    Receiver_shutdown();
    Printer_shutdown();
    Sender_shutdown();
    InputHandler_shutdown();

    printf("Cleaning up all lists...\n");
    SyncList_free(s_pSendList);
    SyncList_free(s_pPrintList);

    printf("s-talk exit\n");

    return 0;
}