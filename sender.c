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

static char* s_recvMacName;
static char* s_recvPort;
static int s_socketDescriptor;

static SyncList *s_pSendList;

void* sendThread(void* unused)
{
    int status = -1;
	struct addrinfo hints, *remote_sin;
	memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_DGRAM;
	
    if(getaddrinfo(s_recvMacName, s_recvPort, &hints, &remote_sin) == -1){
        fprintf(stderr, "Getting address of remote machine failed, error: %s\n", strerror(errno));
        Controller_threadReportInitStatus(&status);
        return NULL;
    }

	s_socketDescriptor = socket(remote_sin->ai_family, remote_sin->ai_socktype, remote_sin->ai_protocol);

    if(s_socketDescriptor == -1){
        fprintf(stderr, "initializing sending socket failed, error: %s\n", strerror(errno));
        Controller_threadReportInitStatus(&status);
        return NULL;
    }

    printf("Sending Thread initialized successfully\n");
    status = 0;
	Controller_threadReportInitStatus(&status);
    
	while (1) {
        char *messageRx = SyncList_get(s_pSendList);

        if (!messageRx && errno == ECANCELED)
		{
			break;
		}

        //TODO: handle send byte <= 0
        sendto(s_socketDescriptor, messageRx, strlen(messageRx) + 1, 0, remote_sin->ai_addr, remote_sin->ai_addrlen);
        
	}

	return NULL;
}


void Sender_init(char* macName, char* port, SyncList *pSendList)
{
    s_recvMacName = macName;
    s_recvPort = port;
	s_pSendList = pSendList;
    pthread_create(&s_threadPID, NULL, sendThread, NULL);
}

void Sender_shutdown(void)
{
    shutdown(s_socketDescriptor, SHUT_RDWR);
    close(s_socketDescriptor);
    printf("Sending socket closed\n");

    pthread_join(s_threadPID, NULL);
    printf("Sending Thread shutdown successfully\n");
}


