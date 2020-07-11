#ifndef _CONTROLLER_H_
#define _CONTROLLER_H_

#include <string.h>

#include "syncList.h"

#define CONTROL_THREAD_NUM 4

static const char CONTROLLER_C_TERM[2] = "!";

void Controller_init(SyncList *pStatusList);
int Controller_getInitThreadCount();
int Controller_threadReportInitStatus(int *statusCode);
int Controller_blockMain();
int Controller_killMain();

#endif