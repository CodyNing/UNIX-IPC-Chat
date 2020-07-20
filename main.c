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
    //check arg count
    assert(argc == 4);

    //get args
    char *arg_localPort = args[1];
    char *arg_remoteMacName = args[2];
    char *arg_remotePort = args[3];

    int localPort = atoi(arg_localPort), initThreadCount = 0;

    puts("s-talk initializing...");

    //create 2 list
    //------------------------to TA: please do not change the LIST_MAX_NUM_NODES < 15, I need some nodes to store my signals -----------------
    SyncList *s_pSendList = SyncList_init(LIST_MAX_NUM_NODES / 3);
    SyncList *s_pPrintList = SyncList_init(LIST_MAX_NUM_NODES / 3);

    //init the controller
    Controller_init();

    //init 4 threads, order doesn't matter because parallel init is implemented
    Printer_init(s_pPrintList);
    Sender_init(arg_remoteMacName, arg_remotePort, s_pSendList);
    Receiver_init(localPort, s_pPrintList);
    InputHandler_init(s_pSendList);

    //blocking call.
    //waiting for all thread init
    //necessary for parallel init
    //Must wait for all threads finishing init process before entering blocking.
    initThreadCount = Controller_getInitThreadCount();

    //only block main when all threads successfully init
    //otherwise just shutdown and clean up.
    if(initThreadCount == CONTROL_THREAD_NUM)
    {
        puts("Main thread is now on blocking...");
        Controller_blockMain();
    }

    puts("Terminating all threads...");

    //cancel all blocking in sync list
    SyncList_cancelBlocking(s_pSendList);
    SyncList_cancelBlocking(s_pPrintList);

    //shutdown all threads, order does matter, producer should be shutdown first.
    //consumer will consume all entries before shutdown
    InputHandler_shutdown();
    Receiver_shutdown();
    Sender_shutdown();
    Printer_shutdown();

    puts("Cleaning up all lists...");

    //free list
    SyncList_free(s_pSendList);
    SyncList_free(s_pPrintList);

    //shutdown controller
    Controller_shutdown();
    
    puts("s-talk exit");

    return 0;
}