#ifndef BUFFER_POOL_H
#define BUFFER_POOL_H

#include <stdint.h>
#include <pthread.h>

#define MAX_BUFFER_COUNT	8192
#define MAX_BUFFER_SIZE	2048

#ifndef BUFFERPOOL_NAME_LEN
#define BUFFERPOOL_NAME_LEN  256
#endif

/*
BufPool_t encapsulates ring buffer design for pool of buffers. 
1. create_bufpool()  - Create pool of buffer with a given count of elements, each element of given size.
2. bufpool_elem_count - Return the number of used buffers.
3. bufpool_getNextFreeBuffer - Return pointer to next available free buffer, blocks if no buffers are free till a buffer becomes available.
4. bufpool_putBackBuffer - Return the used buffer back to the buffer pool. Marks the current tail element as unused.
*/
typedef struct BufPool {
    char            name[BUFFERPOOL_NAME_LEN];
    uint8_t         *buf;
    int             head;
    int             tail;
    uint32_t        elem_size;
    uint32_t        elem_count;
    pthread_mutex_t mutex;
    pthread_cond_t  cond;
}BufPool_t;

/* The ELEM_COUNT and ELEM_SIZE MUST be POWER of 2 !!!!! */
int create_bufpool(uint32_t elem_count, uint32_t elem_size, BufPool_t **q, const char* qname);
int delete_bufpool(BufPool_t* q);

/* Returns elements count in queue or -1 in case of Failure */
int bufpool_elem_count(BufPool_t* q);

/* Get pointer to next available free buffer*/
void* bufpool_getNextFreeBuffer(BufPool_t* q);

/* Mark the Tail of ring buffer as unused, buffer returned back to pool */
void bufpool_putBackBuffer(BufPool_t* q);

#endif /* BUFFER_POOL_H  */
