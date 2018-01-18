//
// Created by zj on 18-1-18.
//

#ifndef SIMULATION_BPLRU_H
#define SIMULATION_BPLRU_H
#include "type.h"
#include "BlkList.h"
struct cache_operation * BPLRU_op_setup();

//定义一个管理BPLRU的块链表
pBlkNode BPLRU_Head;
//创建关于当前BPLRU缓冲区大小的
int BPLRU_CACHE_SIZE;
int BPLRU_MAX_CACHE_SIZE;
int BPLRU_BLK_NUM;


#endif //SIMULATION_BPLRU_H
