//
// Created by zj on 18-1-31.
//

#ifndef SIMULATION_ADCT_H
#define SIMULATION_ADCT_H
#include "type.h"
#include "global.h"
#include "flash.h"
#include "List.h"
struct cache_operation * ADCT_op_setup();

//定义BlkTale的节点
//定义BlkTale的节点
typedef struct ADCT_BlkNode
{
    int BlkNum;
    struct ADCTBlkNode *Pre;
    struct ADCTBlkNode *Next;
    int CleanNum;
    int DirtyNum;
    int Size;
    int CleanList[PAGE_NUM_PER_BLK];
    int DirtyList[PAGE_NUM_PER_BLK];
}ADBNode,*pADBNode;


//设置对应的缓冲区大小
int ADCT_MAX_CACHE_SIZE;
int ADCT_CLRU_CACHE_SIZE;
int ADCT_DLRU_CACHE_SIZE;
//设置块的节点个数
int ADCT_BLK_NUM;

//写队列的链表头部
pNode ADCT_DHead;
//读队列的链表头部
pNode ADCT_CHead;
//设置对应的块节点索引链表的头部
pADBNode ADCT_BHead;

int ADCT_Tau;

//设置最小的读写缓冲区比例
double MinTauRatio;
double MaxTauRatio;

//设置对应的链表头部节点


#endif //SIMULATION_ADCT_H
