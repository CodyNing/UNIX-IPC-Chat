#ifndef _SYNCLIST_H_
#define _SYNCLIST_H_

#define MSG_MAX_LEN 65536

#include <pthread.h>

#include "list.h"

typedef struct SyncList_s SyncList;
struct SyncList_s
{
    List *list;
    size_t max_size;
    bool isCanceled;
    pthread_mutex_t mtx;
    pthread_cond_t item_avil;
    pthread_cond_t buf_avil;
};

SyncList *SyncList_init(size_t size);
int SyncList_put(SyncList *list, void *p);
void * SyncList_get(SyncList *list);
void SyncList_free(SyncList *list);
void SyncList_cancelBlocking(SyncList *list);
void SyncList_resumeBlocking(SyncList *list);

#endif