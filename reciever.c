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

static int s_recvPort, s_socketDescriptor, s_status;
static pthread_t s_threadPID;

static char *s_pMsg;

static SyncList *s_pPrintList;

void *receiveThread(void *unused)
{
	s_status = -1;
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
		Controller_threadReportInitStatus(&s_status);
		return NULL;
	}

	// Bind the socket to the port (PORT) that we specify
	if (bind(s_socketDescriptor, (struct sockaddr *)&sin, sizeof(sin)) == -1)
	{
		fprintf(stderr, "Binding socket to port %d failed, error: %s\n", s_recvPort, strerror(errno));
		Controller_threadReportInitStatus(&s_status);
		return NULL;
	}

	puts("Receiving Thread initialized successfully");
	s_status = 0;
	Controller_threadReportInitStatus(&s_status);

	while (1)
	{
		// Get the data (blocking)
		// Will change sin (the address) to be the address of the client.
		// Note: sin passes information in and out of call!
		struct sockaddr_in sinRemote;
		size_t byteRecv = 0;
		unsigned int sin_len = sizeof(sinRemote);

		s_pMsg = malloc(MSG_MAX_LEN + 1);

		//max len - 1 make sure we have space left for \0
		byteRecv = recvfrom(s_socketDescriptor,
							s_pMsg, MSG_MAX_LEN, 0,
							(struct sockaddr *)&sinRemote, &sin_len);

		if (!byteRecv && s_status == -1)
		{
			break;
		}

		if (byteRecv == -1)
		{	
			fprintf(stderr, "Receiving message from port %d failed, error: %s\n", s_recvPort, strerror(errno));
			Controller_killMain();
			break;
		}

		//make sure recv string is null terminated.
		s_pMsg[byteRecv] = '\0';

		//getnameinfo((struct sockaddr *)&sinRemote, sin_len, remoteHostName, MSG_MAX_LEN, NULL, 0, 0);

		if (!strcmp(CONTROLLER_C_TERM, s_pMsg))
		{
			Controller_killMain();
		}

		if (SyncList_put(s_pPrintList, s_pMsg) == -1 && errno == ECANCELED)
		{
			break;
		}
		s_pMsg = NULL;
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
	s_status = -1;

	shutdown(s_socketDescriptor, SHUT_RDWR);
	close(s_socketDescriptor);
	puts("Receiving socket closed");

	pthread_join(s_threadPID, NULL);
	puts("Receiving Thread shutdown successfully");

	free(s_pMsg);
	s_pMsg = NULL;
}
