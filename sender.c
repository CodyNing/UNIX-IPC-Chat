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

static pthread_t s_threadPID;

static char *s_recvMacName;
static char *s_recvPort;
static int s_socketDescriptor, s_status;

static SyncList *s_pSendList;

void *sendThread(void *unused)
{
    s_status = -1;
    size_t msgLen, byteSent = 0;
    struct addrinfo hints, *remote_sin;
    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_DGRAM;

    if (getaddrinfo(s_recvMacName, s_recvPort, &hints, &remote_sin) == -1)
    {
        fprintf(stderr, "Getting address of remote machine failed, error: %s\n", strerror(errno));
        Controller_threadReportInitStatus(&s_status);
        return NULL;
    }

    s_socketDescriptor = socket(remote_sin->ai_family, remote_sin->ai_socktype, remote_sin->ai_protocol);

    if (s_socketDescriptor == -1)
    {
        fprintf(stderr, "initializing sending socket failed, error: %s\n", strerror(errno));
        Controller_threadReportInitStatus(&s_status);
        return NULL;
    }

    puts("Sending Thread initialized successfully");
    s_status = 0;
    Controller_threadReportInitStatus(&s_status);

    while (1)
    {
        char *messageRx = SyncList_get(s_pSendList);

        if (!messageRx && errno == ECANCELED)
        {
            break;
        }

        //including \0
        msgLen = strlen(messageRx) + 1;

        byteSent = sendto(s_socketDescriptor, messageRx, msgLen, 0, remote_sin->ai_addr, remote_sin->ai_addrlen);
        
        free(messageRx);
        messageRx = NULL;

        if (byteSent == -1)
        {
            fprintf(stderr, "Sending message failed, error: %s\n", strerror(errno));
            Controller_killMain();
            break;
        }

        if (byteSent < msgLen)
        {
            fprintf(stderr, "Message was not correctly sent.\n");
        }
    }

    free(remote_sin);
    remote_sin = NULL;

    return NULL;
}

void Sender_init(char *macName, char *port, SyncList *pSendList)
{
    s_recvMacName = macName;
    s_recvPort = port;
    s_pSendList = pSendList;
    pthread_create(&s_threadPID, NULL, sendThread, NULL);
}

void Sender_shutdown(void)
{
    s_status = -1;
    pthread_join(s_threadPID, NULL);

    shutdown(s_socketDescriptor, SHUT_RDWR);
    close(s_socketDescriptor);
    puts("Sending socket closed");
    puts("Sending Thread shutdown successfully");

}
