//
// Created by zj on 18-1-8.
//

#ifndef SIMULATION_CACHE_H
#define SIMULATION_CACHE_H

#include "flash.h"
#include <stdio.h>
//定义cache的读写延时
#define CACHE_READ_DELAY 0.0005
#define CACHE_WRITE_DELAY 0.0005
//和缓冲区相关的状态标识
#define CACHE_INVALID 0
#define CACHE_VALID 1

//关于cache的读写统计 在对应的fast.c中的lm_init初始化
int buffer_cnt;
int buffer_hit_cnt;
int buffer_miss_cnt;
int buffer_write_hit;
int buffer_write_miss;
int buffer_read_hit;
int buffer_read_miss;
int physical_write;
int physical_read;

//完成一次请求对缓冲区的读写次数的统计
//cache (read/write count) variable
int cache_read_num;
int cache_write_num;

//初始化所有的统计变量
void Buffer_Stat_Reset();
//输出运行的结果到指定输出文件中去
void Buffer_Stat_Print(FILE *outFP);
void reset_cache_stat();
double calculate_delay_cache();
int calculate_arr_positive_num(int *arr,int size);

//设置和ADCT算法相关的变量
int ADCT_Cycle;
//判断ADCT写队列前百分之几的数据页为热，为百分比数
double ADCT_HotTh;


//设置和块级缓冲区相关的结构体
struct BlkTable_entry{
    int BlkSize;
    int CleanNum;
    int DirtyNum;
    int Clist[PAGE_NUM_PER_BLK];
    int Dlist[PAGE_NUM_PER_BLK];
};

//和页状态标识的相关页结构体
struct CachePageEntry{
    int cache_status;
    int cache_age;
    int cache_update;
};


#endif //SIMULATION_CACHE_H
