#ifndef  __CHIPSEA_FIFO_H__
#define  __CHIPSEA_FIFO_H__

#include "mible_type.h"

typedef struct
{
    mible_handler_t func;
    void *arg;
}FifoItem_t;

typedef struct
{
    FifoItem_t *buffer;/* the buffer holding the data*/
    unsigned int size;/* the size of the allocated buffer*/
    unsigned int in;/* data is added at offset (in % size)*/
    unsigned int out;/* data is extracted from off. (out % size)*/
}UserFifo_t;

int userFifoInit(UserFifo_t *fifo, FifoItem_t *buffer, unsigned int size);
void userFifoReset(UserFifo_t *fifo);
int userFifoItemLen(UserFifo_t *fifo);
int userFifoSpaceLen(UserFifo_t *fifo);
int userFifoIsEmpty(UserFifo_t *fifo);
int userFifoPut(UserFifo_t *fifo, FifoItem_t *buffer, unsigned int len);
int userFifoPutItem(UserFifo_t *fifo,FifoItem_t *item);
int userFifoGet(UserFifo_t *fifo, FifoItem_t *buffer, unsigned int len);
int userFifoGetItem(UserFifo_t *fifo,FifoItem_t *item);

#endif
