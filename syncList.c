#include <stdlib.h>
#include <signal.h>
#include <stdio.h>
#include <assert.h>
#include <errno.h>

#include "syncList.h"

static size_t size_remain = LIST_MAX_NUM_NODES;

SyncList *SyncList_init(size_t size)
{
    //dynamic allocate a synclist struct and return the address
    if (size_remain < size || size == 0)
    {
        return NULL;
    }
    size_remain -= size;
    SyncList *list = malloc(sizeof(SyncList));
    list->max_size = size;
    list->isCanceled = false;
    list->list = List_create();
    list->mtx = (pthread_mutex_t)PTHREAD_MUTEX_INITIALIZER;
    list->item_avil = (pthread_cond_t)PTHREAD_COND_INITIALIZER;
    list->buf_avil = (pthread_cond_t)PTHREAD_COND_INITIALIZER;
    return list;
}

//consumer and producer implementation using list
//producer put
int SyncList_put(SyncList *list, void *p)
{
    if (!list)
    {
        return -1;
    }
    int res;
    pthread_mutex_lock(&list->mtx);
    {
        while (List_count(list->list) == list->max_size)
        {
            //allowing external cancel blocking
            if (list->isCanceled)
            {
                errno = ECANCELED;
                pthread_mutex_unlock(&list->mtx);
                return -1;
            }
            pthread_cond_wait(&list->buf_avil, &list->mtx);
        }

        res = List_prepend(list->list, p);

        pthread_cond_signal(&list->item_avil);
    }
    pthread_mutex_unlock(&list->mtx);
    return res;
}

//consumer and producer implementation using list
//consumer get
void *SyncList_get(SyncList *list)
{
    if (!list)
    {
        return NULL;
    }
    void *res;
    pthread_mutex_lock(&list->mtx);
    {
        while (List_count(list->list) == 0)
        {
            //allowing external cancel blocking
            if (list->isCanceled)
            {
                errno = ECANCELED;
                pthread_mutex_unlock(&list->mtx);
                return NULL;
            }
            pthread_cond_wait(&list->item_avil, &list->mtx);
        }

        res = List_trim(list->list);

        pthread_cond_signal(&list->buf_avil);
    }
    pthread_mutex_unlock(&list->mtx);
    return res;
}

//cancel the list blocking
//consumer and producer can still use the list when cancelled,
//but when list is empty, get will fail by getting null ptr
//when list is full, put will fail by getting return -1
void SyncList_cancelBlocking(SyncList *list)
{
    if (!list)
    {
        return;
    }
    pthread_mutex_lock(&list->mtx);
    {
        //set flag to true
        list->isCanceled = true;
        //singal both cv
        pthread_cond_signal(&list->item_avil);
        pthread_cond_signal(&list->buf_avil);
    }
    pthread_mutex_unlock(&list->mtx);
}

//resume the blocking
void SyncList_resumeBlocking(SyncList *list)
{
    if (!list)
    {
        return;
    }
    pthread_mutex_lock(&list->mtx);
    {
        //set flag back to false
        list->isCanceled = false;
    }
    pthread_mutex_unlock(&list->mtx);
}

//a free function does nothing
static void SyncList_free_fn(void *pItem) {}

void SyncList_free(SyncList *list)
{
    if (!list)
    {
        return;
    }
    pthread_mutex_lock(&list->mtx);
    {
        //cancel blocking
        list->isCanceled = true;
        pthread_cond_signal(&list->buf_avil);
        pthread_cond_signal(&list->item_avil);
        //free the internal list
        List_free(list->list, SyncList_free_fn);
    }
    pthread_mutex_unlock(&list->mtx);

    //destory all cv and mutex
    pthread_cond_destroy(&list->item_avil);
    pthread_cond_destroy(&list->buf_avil);
    pthread_mutex_destroy(&list->mtx);
    
    //free the allocated list itself
    free(list);
}