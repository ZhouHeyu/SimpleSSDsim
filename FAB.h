//
// Created by zj on 18-1-16.
//

#ifndef SIMULATION_FAB_H
#define SIMULATION_FAB_H
#include "type.h"
#include "BlkList.h"
struct cache_operation * FAB_op_setup();

//定义一个管理FAB的块链表
pBlkNode FAB_Head;
//创建关于当前FAB缓冲区大小的
int FAB_CACHE_SIZE;
int FAB_MAX_CACHE_SIZE;
int FAB_BLK_NUM;



#endif //SIMULATION_FAB_H
