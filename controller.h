#ifndef _CONTROLLER_H_
#define _CONTROLLER_H_

#include <string.h>

#include "syncList.h"

#define CONTROL_THREAD_NUM 4
#define CONTROL_INIT_FAILED_SIGNAL -1
#define CONTROL_INIT_SUCCESS_SIGNAL 0
#define CONTROL_KILL_SIGNAL 1

//2 for !\0
static const char CONTROLLER_C_TERM[2] = "!";

void Controller_init();
int Controller_getInitThreadCount();
int Controller_threadReportInitStatus(int *statusCode);
int Controller_blockMain();
int Controller_killMain();
void Controller_shutdown();

#endif