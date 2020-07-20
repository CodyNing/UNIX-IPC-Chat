#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <signal.h>
#include <pthread.h>
#include <netdb.h>
#include <unistd.h>

#include "syncList.h"
#include "controller.h"

//thread pid
static pthread_t s_threadPID;

//remote machine name
static char *s_remoteMacName;
//remote port
static char *s_sendPort;
//socket descriptor and thread status
static int s_socketDescriptor, s_status;

//stored send list pointer
static SyncList *s_pSendList;

void *sendThread(void *unused)
{
    //msg len and byte sent
    size_t msgLen, byteSent = 0;
    //hint for getaddrinfo, remote addr result from getaddrinfo
    struct addrinfo hints, *remote_sin;
    //set thread init status to failed first
    s_status = -1;
    //fill the hint
    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_DGRAM;

    //get address info from using machine name and port
    //if failed, print error, report failed and end thread
    if (getaddrinfo(s_remoteMacName, s_sendPort, &hints, &remote_sin))
    {
        fprintf(stderr, "Getting address of remote machine failed, error: %s\n", strerror(errno));
        Controller_threadReportInitStatus(&s_status);
        return NULL;
    }

    //create a socket using the address get
    s_socketDescriptor = socket(remote_sin->ai_family, remote_sin->ai_socktype, remote_sin->ai_protocol);

    //if failed, print error, report failed and end thread
    if (s_socketDescriptor == -1)
    {
        fprintf(stderr, "initializing sending socket failed, error: %s\n", strerror(errno));
        Controller_threadReportInitStatus(&s_status);
        return NULL;
    }

    puts("Sending Thread initialized successfully");
    //set status to success
    s_status = 0;
    //report init success
    Controller_threadReportInitStatus(&s_status);

    while (1)
    {
        //blocking call the get string from sendlist
        char *messageRx = SyncList_get(s_pSendList);

        //if got null and list blocking is cancelled, break the loop
        if (!messageRx && errno == ECANCELED)
        {
            break;
        }

        //msg len should including \0
        msgLen = strlen(messageRx) + 1;

        //send the message using socket
        byteSent = sendto(s_socketDescriptor, messageRx, msgLen, 0, remote_sin->ai_addr, remote_sin->ai_addrlen);
        
        //free the string after send
        free(messageRx);
        messageRx = NULL;

        //if send failed, print error, and terminate the program
        if (byteSent == -1)
        {
            fprintf(stderr, "Sending message failed, error: %s\n", strerror(errno));
            Controller_killMain();
            break;
        }

        //if bytesend is less the the string len, something wrong, tell the user
        if (byteSent < msgLen)
        {
            fprintf(stderr, "Message was not correctly sent.\n");
        }
    }

    //free the remote address allocated by the getaddrinfo
    free(remote_sin);
    remote_sin = NULL;

    return NULL;
}

void Sender_init(char *macName, char *port, SyncList *pSendList)
{
    //store machine name, port, and pointer to sendlist
    s_remoteMacName = macName;
    s_sendPort = port;
    s_pSendList = pSendList;
    //create and start the thread
    pthread_create(&s_threadPID, NULL, sendThread, NULL);
}

void Sender_shutdown(void)
{
    //set status to shutdown
    s_status = -1;
    //join the thread
    pthread_join(s_threadPID, NULL);

    //shutdown and close the socket after thread joined
    //order is important because we want to send all the message in the list before we actually shutdown
    shutdown(s_socketDescriptor, SHUT_RDWR);
    close(s_socketDescriptor);
    puts("Sending socket closed");
    puts("Sending Thread shutdown successfully");

}
