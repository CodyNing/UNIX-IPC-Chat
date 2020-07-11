#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <signal.h>
#include <errno.h>
#include <pthread.h>
#include <netdb.h>
#include <unistd.h>

#include "syncList.h"
#include "controller.h"

static int s_recvPort;
static pthread_t s_threadPID;
static int s_socketDescriptor;

static SyncList *s_pPrintList;

void *receiveThread(void *unused)
{
	int status = -1;
	// Address
	struct sockaddr_in sin;
	memset(&sin, 0, sizeof(sin));
	sin.sin_family = AF_INET;				 // Connection may be from network
	sin.sin_addr.s_addr = htonl(INADDR_ANY); // Host to Network long
	sin.sin_port = htons(s_recvPort);		 // Host to Network short

	// Create the socket for UDP
	s_socketDescriptor = socket(PF_INET, SOCK_DGRAM, 0);

	if (s_socketDescriptor == -1)
	{
		fprintf(stderr, "Init receiving socket failed, error: %s\n", strerror(errno));
		Controller_threadReportInitStatus(&status);
		return NULL;
	}

	// Bind the socket to the port (PORT) that we specify
	if (bind(s_socketDescriptor, (struct sockaddr *)&sin, sizeof(sin)) == -1)
	{
		fprintf(stderr, "Binding socket to port %d failed, error: %s\n", s_recvPort, strerror(errno));
		Controller_threadReportInitStatus(&status);
		return NULL;
	}

	printf("Receiving Thread initialized successfully\n");
	status = 0;
	Controller_threadReportInitStatus(&status);

	while (1)
	{
		// Get the data (blocking)
		// Will change sin (the address) to be the address of the client.
		// Note: sin passes information in and out of call!
		struct sockaddr_in sinRemote;
		size_t byteRecv = 0;
		unsigned int sin_len = sizeof(sinRemote);
		char msgRx[MSG_MAX_LEN], printMsg[PRINT_MAX_LEN], remoteHostName[MSG_MAX_LEN];

		//max len - 1 make sure we have space left for \0
		byteRecv = recvfrom(s_socketDescriptor,
							msgRx, MSG_MAX_LEN - 1, 0,
							(struct sockaddr *)&sinRemote, &sin_len);


		//make sure recv string is null terminated.
		msgRx[byteRecv] = '\0';

		getnameinfo((struct sockaddr *)&sinRemote, sin_len, remoteHostName, MSG_MAX_LEN, NULL, 0, 0);

		sprintf(printMsg, "from %s: %s", remoteHostName, msgRx);

		if (!strcmp(CONTROLLER_C_TERM, msgRx))
		{
			Controller_killMain();
		}

		if (SyncList_put(s_pPrintList, printMsg) == -1 && errno == ECANCELED)
        {
            break;
        }
	}

	return NULL;
}

void Receiver_init(int port, SyncList *pPrintList)
{
	s_recvPort = port;
	s_pPrintList = pPrintList;
	pthread_create(&s_threadPID, NULL, receiveThread, NULL);
}

void Receiver_shutdown(void)
{
	shutdown(s_socketDescriptor, SHUT_RDWR);
	close(s_socketDescriptor);
	printf("Receiving socket closed\n");

	pthread_join(s_threadPID, NULL);
	printf("Receiving Thread shutdown successfully\n");
}
