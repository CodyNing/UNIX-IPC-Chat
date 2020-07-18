#include <stdlib.h>
#include <signal.h>
#include <stdio.h>
#include <assert.h>
#include <errno.h>

#include "syncList.h"

static size_t size_remain = LIST_MAX_NUM_NODES;

SyncList *SyncList_init(size_t size)
{
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

void SyncList_cancelBlocking(SyncList *list)
{
    if (!list)
    {
        return;
    }
    pthread_mutex_lock(&list->mtx);
    {
        list->isCanceled = true;
        pthread_cond_signal(&list->item_avil);
        pthread_cond_signal(&list->buf_avil);
    }
    pthread_mutex_unlock(&list->mtx);
}

void SyncList_resumeBlocking(SyncList *list)
{
    if (!list)
    {
        return;
    }
    pthread_mutex_lock(&list->mtx);
    {
        list->isCanceled = false;
    }
    pthread_mutex_unlock(&list->mtx);
}

static void SyncList_free_fn(void *pItem) {}

void SyncList_free(SyncList *list)
{
    if (!list)
    {
        return;
    }
    pthread_mutex_lock(&list->mtx);
    {
        list->isCanceled = true;
        pthread_cond_signal(&list->buf_avil);
        pthread_cond_signal(&list->item_avil);
        List_free(list->list, SyncList_free_fn);
    }
    pthread_mutex_unlock(&list->mtx);

    pthread_cond_destroy(&list->item_avil);
    pthread_cond_destroy(&list->buf_avil);
    pthread_mutex_destroy(&list->mtx);
    
    free(list);
}