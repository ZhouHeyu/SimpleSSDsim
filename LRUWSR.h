//
// Created by mao on 18-1-17.
//

#ifndef SIMULATION_LRUWSR_H
#define SIMULATION_LRUWSR_H
#include "type.h"
#include "List.h"
struct cache_operation * LRUWSR_op_setup();

//缓冲区最大的大小设置
int LRUWSR_Cache_Max_Entry;
//当前缓冲区的个数
int LRUWSR_Cache_Num_Entry;

pNode LRUWSR_Head;

#endif //SIMULATION_LRUWSR_H
