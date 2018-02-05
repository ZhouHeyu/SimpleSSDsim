//
// Created by ymb on 18/2/2.
//

#ifndef SIMULATION_PUDLRU_H
#define SIMULATION_PUDLRU_H

struct cache_operation * PUD_op_setup();
//定义相关的结构体
struct PUD_Cache_entry{
    int cache_status;
    int cache_age;
    int cache_update;
    };
struct PUD_Block_entry{
    int blk_UD;
    int blk_RD;
    double blk_PUD;
    int I_F_flag;//0_I  1_F
    int blk_frequency;
    int blk_last_hit;
    int blk_size;
    int blk_status;
};
struct PUD_Cache_entry *PUDPage;
struct PUD_BlockList *PUDBlkArr;
unsigned int PUDPage_Num;//只表示总的数据页
unsigned int PUD_Update_count;//每次写命中
unsigned int Threshold; //PUD 值判断的阈值
unsigned int Proportion; // FUG/IUG比例
unsigned int Update_Count;//写更新计数

 //缓冲区最大的大小设置
 int PUD_Cache_Max_Entry;
//当前缓冲区的个数
int PUD_Cache_Num_Entry;

#endif //SIMULATION_PUDLRU_H
