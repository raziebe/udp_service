#define _GNU_SOURCE
#include <stdio.h>
#include <stdint.h>
#include <unistd.h>
#include <stdbool.h>
#include <pthread.h>
#include <malloc.h>
#include <assert.h>
#include <fcntl.h>

#include "hs_fifo.h"
#include "hslog/hslog.h"

struct fifo_descriptor {
    pthread_mutex_t mutex;
    int             numBufs;
    int             waitSize;
    bool            flush;
    int             pipes[2];
};

//typedef struct fifo_descriptor *fifo_t;


/******************************************************************************
 * Fifo_create
 ******************************************************************************/
fifo_t Fifo_create(void)
{
    fifo_t hFifo;

    hFifo = calloc(1, sizeof(struct fifo_descriptor));

    if (hFifo == NULL) {
        hslog_error("Failed to allocate space for Fifo Object\n");
        return NULL;
    }

    if (pipe(hFifo->pipes)) {
        free(hFifo);
        return NULL;
    }

    pthread_mutex_init(&hFifo->mutex, NULL);

    return hFifo;
}

/******************************************************************************
 * Fifo_delete
 ******************************************************************************/
int Fifo_delete(fifo_t hFifo)
{
    int ret = 0;

    if (hFifo) {
        if (close(hFifo->pipes[0])) {
            ret = -1;
        }

        if (close(hFifo->pipes[1])) {
            ret = -1;
        }

        pthread_mutex_destroy(&hFifo->mutex);

        free(hFifo);
    }

    return ret;
}

/******************************************************************************
 * Fifo_get
 ******************************************************************************/
int Fifo_get(fifo_t hFifo, void** ptr)
{
    int flush;
    int numBytes;

    assert(hFifo);
    assert(ptr);

    pthread_mutex_lock(&hFifo->mutex);
    flush = hFifo->flush;
    pthread_mutex_unlock(&hFifo->mutex);

    if (flush) {
        return -1;
    }

    numBytes = read(hFifo->pipes[0], ptr, sizeof(ptr));

    if (numBytes != sizeof(ptr)) {
        pthread_mutex_lock(&hFifo->mutex);
        flush = hFifo->flush;
        if (flush) {
            hFifo->flush = false;
        }
        pthread_mutex_unlock(&hFifo->mutex);

        if (flush) {
            return -1;
        }
        return -1;
    }

    pthread_mutex_lock(&hFifo->mutex);
    hFifo->numBufs--;
    pthread_mutex_unlock(&hFifo->mutex);

    return 0;
}

/******************************************************************************
 * Fifo_flush
 ******************************************************************************/
int Fifo_flush(fifo_t hFifo)
{
    uint8_t ch = 0xff;

    assert(hFifo);

    pthread_mutex_lock(&hFifo->mutex);
    hFifo->flush = true;
    pthread_mutex_unlock(&hFifo->mutex);

    /* Make sure any Fifo_get() calls are unblocked */
    if (write(hFifo->pipes[1], &ch, 1) != 1) {
	hslog_error("%s %d\n",__func__,__LINE__);
        return -1;
    }

    return 0;
}

/******************************************************************************
 * Fifo_put
 ******************************************************************************/
int Fifo_put(fifo_t hFifo, void **ptr)
{
    assert(hFifo);
    assert(ptr);

    pthread_mutex_lock(&hFifo->mutex);
    hFifo->numBufs++;
    pthread_mutex_unlock(&hFifo->mutex);

    if (write(hFifo->pipes[1], ptr, sizeof(ptr)) != sizeof(ptr)) {
	hslog_error("%s %d\n",__func__,__LINE__);
        return -1;
    }

    return 0;
}

/******************************************************************************
 * Fifo_getNumEntries
 ******************************************************************************/
int Fifo_getNumEntries(fifo_t hFifo)
{
    int numEntries;

    assert(hFifo);

    pthread_mutex_lock(&hFifo->mutex);
    numEntries = hFifo->numBufs;
    pthread_mutex_unlock(&hFifo->mutex);

    return numEntries;
}

int set_fifo_size(fifo_t hFifo, size_t size)
{
    int res = 0;
    const int MAX_PIPE_SIZE = 1048576; // 1MB
    if(size <= 0)
        return -1;

    pthread_mutex_lock(&hFifo->mutex);
    res = fcntl(hFifo->pipes[1], F_SETPIPE_SZ, size);
    pthread_mutex_unlock(&hFifo->mutex);

    return res;
}

int set_fifo_wait_size(fifo_t hFifo, size_t size)
{
    int writeBufferSize = 0;
    if(size <= 0)
        return -1;

    writeBufferSize = fcntl(hFifo->pipes[1], F_GETPIPE_SZ);
    if(writeBufferSize < 0){
        return writeBufferSize;
    }

    if(writeBufferSize < size)
        return -1;

    pthread_mutex_lock(&hFifo->mutex);
    hFifo->waitSize = size;
    pthread_mutex_unlock(&hFifo->mutex);

    return 0;
}

