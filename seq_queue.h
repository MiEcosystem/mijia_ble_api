/*****************************************************************************
File Name:    seq_queue.h
Discription:  顺序队列头文件
History:
Date                Author                   Description
2017-02-18         Lucien                    Creat
****************************************************************************/


#ifndef __SEQ_QUEUE_H__
#define __SEQ_QUEUE_H__

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>


typedef uint32_t TSeqQueNode;


typedef struct{
        int             m_nSize;
        int             m_nHead;
        int             m_nTail;
        TSeqQueNode*    m_pData;
}squeue_t;




void sqInit(squeue_t *q,int size,TSeqQueNode *pdataArr);
bool sqPut(squeue_t *q,const TSeqQueNode* p_element);
bool sqGet(squeue_t *q,TSeqQueNode* p_element);
bool sqPeek(squeue_t *q,TSeqQueNode* p_element);
bool sqPeekAll(squeue_t *q,TSeqQueNode* cells);
bool sqPeekAt(squeue_t *q,TSeqQueNode* cells, int length);
void sqPop(squeue_t *q);
void sqPopAt(squeue_t *q,int pos);
void sqPopAll(squeue_t *q);
int  sqGetCount(squeue_t *q);
bool sqIsFull(squeue_t *q);
bool sqIsEmpty(squeue_t *q);
int  sqGetQueSize(squeue_t *q);















#endif


