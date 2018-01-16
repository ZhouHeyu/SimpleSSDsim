//
// Created by zj on 18-1-14.
//使用双链表实现CFLRU，所以存在一个头结点Head
//

#ifndef SIMULATION_CFLRU_H
#define SIMULATION_CFLRU_H
#include "type.h"
#include "List.h"
struct cache_operation * CFLRU_op_setup();

//缓冲区最大的大小设置
int CFLRU_Cache_Max_Entry;
//当前缓冲区的个数
int CFLRU_Cache_Num_Entry;

pNode CFLRU_Head;
//CFLRU的窗口比例alpha,w(大小)
double alpha;
int Window_Size;

#endif //SIMULATION_CFLRU_H
