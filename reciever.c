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

//port number, socket descriptor, thread status
static int s_recvPort, s_socketDescriptor, s_status;
//thread pid
static pthread_t s_threadPID;

//pointer to allocated string
static char *s_pMsg;

//pointer to print list
static SyncList *s_pPrintList;

void *receiveThread(void *unused)
{
	// Address
	struct sockaddr_in sin;
	memset(&sin, 0, sizeof(sin));
	sin.sin_family = AF_INET;				 // Connection may be from network
	sin.sin_addr.s_addr = htonl(INADDR_ANY); // Host to Network long
	sin.sin_port = htons(s_recvPort);		 // Host to Network short

	//set status to fail first
	s_status = -1;

	// Create the socket for UDP
	s_socketDescriptor = socket(PF_INET, SOCK_DGRAM, 0);

	//if create socket failed
	if (s_socketDescriptor == -1)
	{
		//print error to stderr
		fprintf(stderr, "Init receiving socket failed, error: %s\n", strerror(errno));
		//report fail status
		Controller_threadReportInitStatus(&s_status);
		//end thread execution
		return NULL;
	}

	// Bind the socket to the port (PORT) that we specify
	// if bind socket to port failed
	if (bind(s_socketDescriptor, (struct sockaddr *)&sin, sizeof(sin)) == -1)
	{
		//print error to stderr
		fprintf(stderr, "Binding socket to port %d failed, error: %s\n", s_recvPort, strerror(errno));
		//report fail status
		Controller_threadReportInitStatus(&s_status);
		//end thread execution
		return NULL;
	}

	puts("Receiving Thread initialized successfully");
	//change status to success
	s_status = 0;
	//report success init
	Controller_threadReportInitStatus(&s_status);

	while (1)
	{
		//remote address
		struct sockaddr_in sinRemote;
		//remote address len
		unsigned int sin_len = sizeof(sinRemote);
		//recv'd byte
		size_t byteRecv = 0;

		//allocate memory for string. +1 for \0
		s_pMsg = malloc(MSG_MAX_LEN + 1);

		//blocking call get message from socket of the port
		byteRecv = recvfrom(s_socketDescriptor,
							s_pMsg, MSG_MAX_LEN, 0,
							(struct sockaddr *)&sinRemote, &sin_len);

		//if recv'd 0 byte and thread status is shuting down
		//then the socket should be can shutdown and closed by the thread shutdown process
		//so break the loop
		if (!byteRecv && s_status == -1)
		{
			break;
		}

		//if recvfrom failed, then print error and terminate the program
		if (byteRecv == -1)
		{	
			fprintf(stderr, "Receiving message from port %d failed, error: %s\n", s_recvPort, strerror(errno));
			Controller_killMain();
			break;
		}

		//make sure recv string is null terminated.
		s_pMsg[byteRecv] = '\0';

		//getnameinfo((struct sockaddr *)&sinRemote, sin_len, remoteHostName, MSG_MAX_LEN, NULL, 0, 0);

		//if string recv is terminate char then terminate the program
		if (!strcmp(CONTROLLER_C_TERM, s_pMsg))
		{
			Controller_killMain();
		}

		//put the string to print anyway
		//if put failed and blocking is cancelled then break
		if (SyncList_put(s_pPrintList, s_pMsg) == -1 && errno == ECANCELED)
		{
			break;
		}
		//any thing put into the list will be free by consumer so set the pointer to null
		s_pMsg = NULL;
	}

	return NULL;
}

void Receiver_init(int port, SyncList *pPrintList)
{
	//store port and pointer to list
	s_recvPort = port;
	s_pPrintList = pPrintList;
	//create and start thread
	pthread_create(&s_threadPID, NULL, receiveThread, NULL);
}

void Receiver_shutdown(void)
{
	//set status to shutdown
	s_status = -1;

	//shutdown and close the socket
	shutdown(s_socketDescriptor, SHUT_RDWR);
	close(s_socketDescriptor);
	puts("Receiving socket closed");

	//join thread
	pthread_join(s_threadPID, NULL);
	puts("Receiving Thread shutdown successfully");

	//free the string pointer when memory is allocated but didn't have chance to be put in the list 
	free(s_pMsg);
	s_pMsg = NULL;
}
