//
// Created by zhouheyu on 17-12-27.
//

#ifndef SIMULATION_INTERFACE_H
#define SIMULATION_INTERFACE_H

#include "global.h"
#include "Cache.h"
#include "dftl.h"

//初始化底层的nand和FTL算法的选择
void initFlash();
//重置底层的flash的读写擦除的统计变量
void reset_flash_stat();
void endFlash();
void printWearout();
//这里的操作的是扇区号(tart_blk_no, block_cnt)
void send_flash_request(int start_blk_no, int block_cnt, int operation, int mapdir_flag);
double callFsim(unsigned int secno, int scount, int operation);

void find_real_max();
void find_real_min();
int find_min_ghost_entry();
int find_free_pos( int *arr, int size);
double CacheManage(unsigned int secno,int scount,int operation);
//将数据插入到(int)arr数组指定的位置pos，pos之后的数据往后挪动一位
int InsertArr(int *arr,int size,int data,int pos);

//输出运行仿真时间相关统计信息
void SimulationTime_Stat_Print(FILE *outFP);

#endif //SIMULATION_INTERFACE_H
