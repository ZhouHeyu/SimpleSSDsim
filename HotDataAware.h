//
// Created by ymb on 18/3/30.
//

#ifndef SIMULATION_HOTDATEAWARE_H
#define SIMULATION_HOTDATEAWARE_H
#include "type.h"
#include "List.h"
//#define hash_size 2048   //2*cache_size

struct cache_operation *HotDateAware_op_setup();
double Hp;//热度值
double H_th1;//热度阈值
double H_th2;//局部热度阈值
double HotDataAware_Tau;//队列阈值

struct hash_entry{
    int visit_count;
    int buf_state;
    int last_visit_time;
};

struct hash_entry *hash_table;


int range_k;

int hash_size;
//int hash[3][hash_size]={0};
int t_sys;//系统时间，即写更新统计
int half_time; //半衰期
//int hash_size;//记录hash大小
pNode CD_list;   //缓存的干净队列和脏队列
int CL,DL;       //干净队列  脏队列 长度
int HotDataAware_Cache_Num_Entry;
int HotDataAware_Cache_Max_Entry;
//pNode C_list;

#endif //SIMULATION_HOTDATEAWARE_H

