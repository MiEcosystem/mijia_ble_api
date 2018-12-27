#include "chipsea_fifo.h"
#include "string.h"

#define fifoMin(a,b) ((a) < (b) ? (a):(b))

//FIFO中可写的数据空间
#define fifoWriteableSize(size,in,out) ((in) < (out) ? (out - in):(size - in + out))
//FIFO中可读的数据空间
#define fifoReadableSize(size,in,out)  ((in) < (out) ? (size - out + in):(in - out))

//FIFO中剩余的可写空间,in之后的空间,不包括out之前
#define fifoRemainLen(size,in,out)     ((in) < (out) ? (out - in):(size - in))
//FIFO中剩余的可读空间,out之后的空间,不包括in之后
#define fifoValueLen(size,in,out)      ((in) > (out) ? (in - out):(size - out))

//初始化FIFO
int userFifoInit(UserFifo_t *fifo, FifoItem_t *buffer, unsigned int size)
{
    if (fifo == NULL)
        return -1;

    if(buffer == NULL)
        return -1;

    fifo->buffer = buffer;
    fifo->size = size;
    fifo->in = fifo->out = 0;

    return 0;
}

//清空FIFO
void userFifoReset(UserFifo_t *fifo)
{
    fifo->in = fifo->out = 0;
}

//获取FIFO中可读数据长度
int userFifoItemLen(UserFifo_t *fifo)
{
    return fifoReadableSize(fifo->size ,fifo->in, fifo->out);
}

//获取FIFO中可写空间长度
int userFifoSpaceLen(UserFifo_t *fifo)
{
    return fifoWriteableSize(fifo->size ,fifo->in, fifo->out);
}

//判断FIFO是否为空
int userFifoIsEmpty(UserFifo_t *fifo)
{
    return (fifoWriteableSize(fifo->size ,fifo->in, fifo->out) == fifo->size);
}

int userFifoPut(UserFifo_t *fifo, FifoItem_t *buffer, unsigned int len)
{
    unsigned int remainLen;

    len = fifoMin(len, fifoWriteableSize(fifo->size, fifo->in, fifo->out));
    remainLen = fifoMin(len, fifoRemainLen(fifo->size, fifo->in, fifo->out));
    //printf("%s(%d):len = %d;remainLen = %d\r\n",__func__,__LINE__,len,remainLen);

    memcpy(fifo->buffer + fifo->in, buffer, remainLen * sizeof(FifoItem_t));
    memcpy(fifo->buffer, buffer + remainLen, (len - remainLen) * sizeof(FifoItem_t));

	fifo->in = ((fifo->in + len)%(fifo->size));
    return len;
}

int userFifoPutItem(UserFifo_t *fifo,FifoItem_t *item)
{
	if(userFifoSpaceLen(fifo) == 0){
		return -1;
	}
	else{
		userFifoPut(fifo, item, 1);
		return 0;
	}
}

int userFifoGet(UserFifo_t *fifo, FifoItem_t *buffer, unsigned int len)
{
    unsigned int remainLen;

    len = fifoMin(len, fifoReadableSize(fifo->size, fifo->in, fifo->out));
    remainLen = fifoMin(len, fifoValueLen(fifo->size, fifo->in, fifo->out));
    //printf("%s(%d):len = %d;remainLen = %d\r\n",__func__,__LINE__,len,remainLen);

    memcpy(buffer, fifo->buffer + fifo->out, remainLen * sizeof(FifoItem_t));
    memcpy(buffer + remainLen, fifo->buffer, (len - remainLen) * sizeof(FifoItem_t));

	fifo->out = ((fifo->out + len)%(fifo->size));
    return len;
}

int userFifoGetItem(UserFifo_t *fifo,FifoItem_t *item)
{
	if(userFifoItemLen(fifo) == 0){
		return -1;
	}
	else{
		userFifoGet(fifo, item, 1);
		return 0;
	}
}

