#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <errno.h>
#include <stdatomic.h>

#include "hs_buffer_pool.h"
#include "hslog/hslog.h"


/* Return count in buffer.  */
#define CIRC_CNT(head,tail,size) (((head) - (tail)) & ((size)-1))

/* Return space available, 0..size-1.  We always leave one free char
   as a completely full buffer has head == tail, which is the same as
   empty.  */
#define CIRC_SPACE(head,tail,size) CIRC_CNT((tail),((head)+1),(size))

/* Return count up to the end of the buffer.  Carefully avoid
   accessing head and tail more than once, so they can change
   underneath us without returning inconsistent results.  */
#define CIRC_CNT_TO_END(head,tail,size) \
	({int end = (size) - (tail); \
	  int n = ((head) + end) & ((size)-1); \
	  n < end ? n : end;})

/* Return space available up to the end of the buffer.  */
#define CIRC_SPACE_TO_END(head,tail,size) \
	({int end = (size) - 1 - (head); \
	  int n = (end + (tail)) & ((size)-1); \
	  n <= end ? n : end+1;})

int create_bufpool(uint32_t elem_count, uint32_t elem_size, BufPool_t **q, const char* qname)
{
    if (*q) {
        hslog_error("Can't create queue (%s), the 'q' is not NULL!", qname);
        return -1;
    }
    if (elem_size > MAX_BUFFER_SIZE || elem_count > MAX_BUFFER_COUNT) {
        hslog_error("Can't create queue (%s), incorrect params", qname);
        hslog_error("elem_count:%u MAX_BUFFER_COUNT:%u elem_size:%u MAX_BUFFER_SIZE:%u",
                elem_count, MAX_BUFFER_COUNT, elem_size, MAX_BUFFER_SIZE);
        return -1;
    }
    *q = (BufPool_t*) malloc(sizeof(BufPool_t));
    if (!*q) {
        hslog_error("queue (%s) allocation failed. err:%s", qname, strerror(errno));
        return -1;
    }
    (*q)->buf = (uint8_t*) malloc(elem_size * elem_count);
    if (!(*q)->buf) {
        hslog_error(" queue's (%s) buf allocation failed. err:%s", qname, strerror(errno));
        return -1;
    }
    atomic_store(&(*q)->head, 0);
    atomic_store(&(*q)->tail, 0);
    (*q)->elem_count = elem_count;
    (*q)->elem_size = elem_size;
    strcpy((*q)->name, qname);
    pthread_mutex_init(&(*q)->mutex, NULL);
    pthread_cond_init(&(*q)->cond, NULL);

    return 0; 
}

int delete_bufpool(BufPool_t* q)
{
    if (!q || !q->buf) {
        hslog_error("Can't delete queue:%s, the 'q' or 'q->buf' is NULL!", q->name);
        return -1;
    }
    free(q->buf);
    free(q);
    pthread_mutex_destroy(&q->mutex);
    pthread_cond_destroy(&q->cond);
    return 0;
}

int bufpool_elem_count(BufPool_t* q)
{
    return (CIRC_CNT(atomic_load(&q->head), atomic_load(&q->tail), q->elem_count));
}

void* bufpool_getNextFreeBuffer(BufPool_t* q)
{
    void *nextFreeBuffer = NULL;

    pthread_mutex_lock(&q->mutex);
    if (CIRC_SPACE(q->head, q->tail, q->elem_count) < 1) {
        pthread_cond_wait(&q->cond, &q->mutex);
    }

    nextFreeBuffer = q->buf + q->head * q->elem_size;
    q->head = (q->head + 1) & (q->elem_count - 1);
    pthread_mutex_unlock(&q->mutex);
    return nextFreeBuffer;
}

void bufpool_putBackBuffer(BufPool_t* q)
{
    pthread_mutex_lock(&q->mutex);
    if (CIRC_CNT(q->head, q->tail, q->elem_count) >= 1) {
        q->tail = (q->tail + 1) & (q->elem_count - 1);
        pthread_cond_broadcast(&q->cond);
    }
    pthread_mutex_unlock(&q->mutex);
}
