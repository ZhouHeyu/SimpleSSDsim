//
// Created by zj on 18-1-31.
//

#ifndef SIMULATION_ADCT_H
#define SIMULATION_ADCT_H
#include "type.h"
#inlcude ""
struct cache_operation * ADCT_op_setup();

//设置对应的缓冲区大小
int ADCT_MAX_CACHE_SIZE;
int ADCT_CLRU_CACHE_SIZE;
int ADCT_DLRU_CACHE_SIZE;

//设置对应的链表头部节点


#endif //SIMULATION_ADCT_H
