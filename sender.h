#ifndef _SENDER_H_
#define _SENDER_H_

#include "syncList.h"

void Sender_init(char* machine_name, char* port, SyncList *list);

void Sender_shutdown(void);

#endif