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
    assert(argc == 4);

    char *arg_localPort = args[1];
    char *arg_remoteMacName = args[2];
    char *arg_remotePort = args[3];

    int localPort = atoi(arg_localPort), initThreadCount = 0;

    puts("s-talk initializing...");

    SyncList *s_pSendList = SyncList_init(LIST_MAX_NUM_NODES / 3);
    SyncList *s_pPrintList = SyncList_init(LIST_MAX_NUM_NODES / 3);

    Controller_init();

    Printer_init(s_pPrintList);
    Sender_init(arg_remoteMacName, arg_remotePort, s_pSendList);
    Receiver_init(localPort, s_pPrintList);
    InputHandler_init(s_pSendList);

    initThreadCount = Controller_getInitThreadCount();

    if(initThreadCount == CONTROL_THREAD_NUM)
    {
        puts("Main thread is now on blocking...");
        Controller_blockMain();
    }

    puts("Terminating all threads...");

    SyncList_cancelBlocking(s_pSendList);
    SyncList_cancelBlocking(s_pPrintList);

    InputHandler_shutdown();
    Receiver_shutdown();
    Sender_shutdown();
    Printer_shutdown();

    puts("Cleaning up all lists...");
    SyncList_free(s_pSendList);
    SyncList_free(s_pPrintList);

    Controller_shutdown();
    
    puts("s-talk exit");

    return 0;
}