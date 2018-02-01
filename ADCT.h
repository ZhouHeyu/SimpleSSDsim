//
// Created by zj on 18-1-31.
//

#ifndef SIMULATION_ADCT_H
#define SIMULATION_ADCT_H
#include "type.h"
#include "Cache.h"
#include "global.h"
#include "flash.h"
struct cache_operation * ADCT_op_setup();

unsigned int ADCTPageNum;
struct CachePageEntry *ADCTNandPage;
//表示对应数据块的个数
unsigned int BlkTableNum;
struct  BlkTable_entry *BlkTable;

//设计缓冲区的大小的数组
int * clru_cache_arr;
int * dlru_cache_arr;

//设置对应的缓冲区大小
int ADCT_MAX_CACHE_SIZE;
int ADCT_CLRU_CACHE_SIZE;
int ADCT_DLRU_CACHE_SIZE;

int ADCT_Tau;

//设置最小的读写缓冲区比例
double MinTauRatio;
double MaxTauRatio;

//设置对应的链表头部节点


#endif //SIMULATION_ADCT_H
