//
// Created by zj on 18-1-15.
//

#ifndef SIMULATION_CASA_H
#define SIMULATION_CASA_H
#include "type.h"
struct cache_operation * CASA_op_setup();

//对应各个链表的长度,全局变量的cache_size在global.h中定义
int CLRU_CACHE_SIZE;
int DLRU_CACHE_SIZE;
int CASA_CACHE_SIZE;

//各自链表的头结点
pNode CLRU_Head;
pNode DLRU_Head;

//还需要定义和上一次flash读写延迟的比例
double CASA_FLASH_WRITE_DELAY;
double CASA_FLASH_READ_DELAY;


//还有算法定义中的Tau(目标干净页队列的大小),还可以指定初始的Tau值在global.h中定义
int CASA_Tau;





#endif //SIMULATION_CASA_H
