

#ifndef _RECEIVER_H_
#define _RECEIVER_H_

#include "syncList.h"

// Start background receive thread
// void Receiver_init(char* rxMessage, pthread_mutex_t sharedMutexWithOtherThread);
void Receiver_init(int port, SyncList *list);

// Stop background receive thread and cleanup
void Receiver_shutdown(void);

#endif