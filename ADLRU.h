//
// Created by mao on 18-1-16.
//

#ifndef SIMULATION_ADLRU_H
#define SIMULATION_ADLRU_H
#include "type.h"
#include "List.h"
struct cache_operation * ADLRU_op_setup();

int COLD_CACHE_SIZE;
int HOT_CACHE_SIZE;
int ADLRU_CACHE_MAX_SIZE;

pNode ColdLRU_Head;
pNode HotLRU_Head;
//算法设置的最小冷区下限
int COLD_MIN ;
#endif //SIMULATION_ADLRU_H
