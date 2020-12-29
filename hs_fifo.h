typedef struct fifo_descriptor *fifo_t;

// member functions
fifo_t Fifo_create(void);
int Fifo_get(fifo_t fifo, void **ptr);
int Fifo_put(fifo_t fifo, void **ptr);
int Fifo_flush(fifo_t fifo);
int Fifo_getNumEntries(fifo_t fifo);
int Fifo_delete(fifo_t fifo);
int set_fifo_size(fifo_t fifo, size_t  size);
int set_wait_size(fifo_t fifo, size_t size);
