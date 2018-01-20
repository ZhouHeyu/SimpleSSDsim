//
// Created by zj on 18-1-20.
//

#ifndef SIMULATION_CCFLRU_H
#define SIMULATION_CCFLRU_H
#include "type.h"
#include "List.h"
struct cache_operation * CCFLRU_op_setup();
//缓冲区最大的大小设置
int CCFLRU_Cache_Max_Entry;
//当前缓冲区的个数
int CCFLRU_Cache_Num_Entry;
//队列设置为两个一个是ColdClean队列,一个是Mix队列
pNode ML_Head;
pNode CCL_Head;
//同样需要两种的队列长度统计变量
int ML_Size;
int CCL_Size;

#endif //SIMULATION_CCFLRU_H
