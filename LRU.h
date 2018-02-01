//
// Created by zj on 18-1-9.
//

#ifndef SIMULATION_LRU_H
#define SIMULATION_LRU_H

#include "type.h"
#include "Cache.h"
struct cache_operation * LRU_op_setup();
//定义相关的结构体

struct CachePageEntry *LRUPage;
unsigned int LRUPage_Num;//只表示总的数据页
//定义当前的cache的age最小的索引
int LRU_min_age_index;
int LRU_max_age_index;

//缓冲区最大的大小设置
int LRU_Cache_Max_Entry;
//当前缓冲区的个数
int LRU_Cache_Num_Entry;
//下面表示存储LPN的数组
int *lru_cache_arr;


#endif //SIMULATION_LRU_H
