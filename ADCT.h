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
int * ADCT_clru_cache_arr;
int * ADCT_dlru_cache_arr;

//设置对应的缓冲区大小
int ADCT_MAX_CACHE_SIZE;
int ADCT_CLRU_CACHE_SIZE;
int ADCT_DLRU_CACHE_SIZE;

int ADCT_Tau;

//设置最小的读写缓冲区比例
double MinTauRatio;
double MaxTauRatio;

//设置对应的命中情况变量
int CDHit_CWH;
int CDHit_CRH;
int CDHit_DRH;
int CDHit_DWH;
int TCount;
//设置周期物理读写的次数和读写时延
int cycle_physical_write;
int cycle_physical_read;

int ADCT_FW;
int ADCT_BW;
int last_flash_write;
int ADCT_t;

double cycle_flash_read_delay;
double cycle_flash_write_delay;


#endif //SIMULATION_ADCT_H
