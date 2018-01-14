//
// Created by zj on 18-1-9.
//

#include "LRU.h"
#include <stdlib.h>
#include "global.h"
#include "flash.h"
#include "Interface.h"
#include "Cache.h"

int my_find_cache_max(int *arr,int arrMaxSize,int *index)
{
    int i;
    int temp=-1;
    int cache_max=-1;
    *index=-1;
    for ( i = 0; i <arrMaxSize ; ++i) {
        if (arr[i]!=-1){
            if (LRUPage[arr[i]].cache_age>temp){
                temp=LRUPage[arr[i]].cache_age;
                cache_max=arr[i];
                *index=i;
            }
        }
    }
    return cache_max;
}

//找到数组中索引对应的LPN的age最大或最小,同时添加返回数组最值的索引
int my_find_cache_min(int *arr,int arrMaxSize,int * index)
{
    int i;
    int temp=99999999;
    int cache_min=-1;
    *index=-1;
    for (i = 0; i <arrMaxSize ; ++i) {
        //attention arr[i]=-1 ,the NandPage[-1] is error
        if(arr[i]!=-1){
            if(LRUPage[arr[i]].cache_age<temp){
                temp=LRUPage[arr[i]].cache_age;
                cache_min=arr[i];
                *index=i;
            }
        }
    }
    return cache_min;
}

int LRU_init(int size, int DataBlk_Num)
{
    int i;
    LRU_Cache_Max_Entry=size;
    LRU_Cache_Num_Entry=0;
    lru_cache_arr=(int *)malloc(sizeof(int)*LRU_Cache_Max_Entry);
    if(lru_cache_arr==NULL){
        fprintf(stderr,"malloc for lru_cache_arr failed\n");
        exit(0);
    }
    //数组初始化
    for ( i = 0; i < LRU_Cache_Max_Entry; ++i) {
        lru_cache_arr[i]=-1;
    }
    LRUPage_Num=DataBlk_Num*PAGE_NUM_PER_BLK;
    LRUPage=(struct LRU_Cache_entry*)malloc(sizeof(struct LRU_Cache_entry)*LRUPage_Num);
    if(LRUPage==NULL){
        printf("the create LRUpage Memeory is failed\n");
        exit(1);
    }
    memset(LRUPage,0xFF,sizeof(struct LRU_Cache_entry)*LRUPage_Num);
    for ( i = 0; i <LRUPage_Num ; ++i) {
        LRUPage[i].cache_age=0;
        LRUPage[i].cache_status=0;
        LRUPage[i].cache_update=0;
    }
}

//释放对应的内存
void LRU_end()
{
    if(LRUPage!=NULL){
        free(LRUPage);
    }
    if(lru_cache_arr!=NULL){
        free(lru_cache_arr);
    }
}

//返回匹配的命中请求在数组中的位置
int LRU_Search(int LPN,int operation)
{
    int i;
    int index=-1;
    //如果对应的项标识的状态表示对应的LPN不存在缓冲区中
    if(LRUPage[LPN].cache_status!=CACHE_VALID){
        return index;
    }

    for(i=0;i<LRU_Cache_Max_Entry;i++){
        if(lru_cache_arr[i]==LPN){
            index=i;
            break;
        }
    }
    return index;

}

int LRU_HitCache(int LPN,int operation,int Hit_index)
{
    //先做一个错误判断
    int i;
    int index=-1;
    //做一个命中数据页的转移，找到对应请求的位置
    for(i=0;i<LRU_Cache_Max_Entry;i++){
        if(lru_cache_arr[i]==LPN){
            index=i;
            break;
        }
    }
    if(index==-1){
        fprintf(stderr,"error happended in HitCache:can not find LPN %d in arr\n",LPN);
        exit(0);
    }
    //更新对应的LRUpage的age
    buffer_hit_cnt++;
    LRUPage[LPN].cache_age=LRUPage[LRU_max_age_index].cache_age+1;
    LRU_max_age_index=LPN;
//   统计对应的读写命中情况
    if(operation==0){
        //write
        LRUPage[LPN].cache_update=1;
        buffer_write_hit++;
        cache_write_num++;
    }else{
        //read
        buffer_read_hit++;
        cache_read_num++;
    }
//   将命中的请求移动到LRU位置（因为是数组索引实现，不存在移动的问题）
//    返回命中的请求在arr中的位置索引
    return index;
}

double LRU_AddCacheEntry(int LPN,int operation)
{
    //将新的请求加入到缓冲区中
    double delay=0.0;
    int free_pos=-1;
    free_pos=find_free_pos(lru_cache_arr,LRU_Cache_Max_Entry);
    //debug test
    if(free_pos==-1){
        fprintf(stderr,"error happened in AddCacheEntry:can not find free pos for LPN %d in cache-arr\n",LPN);
        exit(0);
    }
    lru_cache_arr[free_pos]=LPN;
    LRU_Cache_Num_Entry++;
    physical_read++;
    delay=callFsim(LPN*4,4,1);
//    改变相应的标识
    LRUPage[LPN].cache_age=LRUPage[LRU_max_age_index].cache_age+1;
    LRU_max_age_index=LPN;
    LRUPage[LPN].cache_status=CACHE_VALID;
    buffer_miss_cnt++;
    if(operation==0){
//        write
        LRUPage[LPN].cache_update=1;
        buffer_write_miss++;
        cache_write_num++;
    }else{
//        read
        buffer_read_miss++;
        cache_read_num++;
    }
//   做一个长度检查：debug
    if(calculate_arr_positive_num(lru_cache_arr,LRU_Cache_Max_Entry)!=LRU_Cache_Num_Entry){
        fprintf(stderr,"error happened in AddCacheEntry:\n");
        fprintf(stderr,"cache-size compute exist eror: LRU_Cache_Num_Entry is %d\n",LRU_Cache_Num_Entry);
        fprintf(stderr,"lru_cache_arr's valid varibale number is %d\n",calculate_arr_positive_num(lru_cache_arr,LRU_Cache_Max_Entry));
        exit(0);
    }

    return delay;
}

//不管缓冲区是否满都调用该函数（满）
double LRU_DelCacheEntry()
{
    double delay=0.0;
    int victim_index=-1;
    int victim_LPN=-1;
    if(LRU_Cache_Num_Entry<LRU_Cache_Max_Entry){
        return delay;
    }
//    选择age最小的项做剔除对象,函数返回的是最小的age的LPN号
    victim_LPN=my_find_cache_min(lru_cache_arr,LRU_Cache_Max_Entry,&victim_index);
//    debug 测试是否找到对应的最小age，LPN和index是否符合
    if(victim_LPN==-1){
        fprintf(stderr,"error happend in DelCacheEntry can not find min age(victim) in lru_cache_arr\n ");
        exit(0);
    }
    if(victim_index==-1 || victim_index >=LRU_Cache_Max_Entry)
    {
        fprintf(stderr,"error happend in DelCacheEntry: the victim_index is error\n");
        exit(0);
    }

//   判断选择的数据页是否为脏页
    if(LRUPage[victim_LPN].cache_update==1){
        physical_write++;
        delay=callFsim(victim_LPN*4,4,0);
    }
//  重置相应的LRUpage的状态位,删除对应数组中的LPN
    lru_cache_arr[victim_index] = -1;
    LRUPage[victim_LPN].cache_update=0;
    LRUPage[victim_LPN].cache_age=0;
    LRUPage[victim_LPN].cache_status=0;
    LRU_Cache_Num_Entry--;
//   做一个长度检查：debug
    if(calculate_arr_positive_num(lru_cache_arr,LRU_Cache_Max_Entry)!=LRU_Cache_Num_Entry){
        fprintf(stderr,"error happened in DelCacheEntry:\n");
        fprintf(stderr,"cache-size compute exist eror: LRU_Cache_Num_Entry is %d\n",LRU_Cache_Num_Entry);
        fprintf(stderr,"lru_cache_arr's valid varibale number is %d\n",calculate_arr_positive_num(lru_cache_arr,LRU_Cache_Max_Entry));
        exit(0);
    }
    return delay;
}



struct cache_operation LRU_Operation={
        init:   LRU_init,
        SearchCache:    LRU_Search,
        HitCache:   LRU_HitCache,
        AddCacheEntry:  LRU_AddCacheEntry,
        DelCacheEntry:  LRU_DelCacheEntry,
        end:    LRU_end
};

struct cache_operation * LRU_op_setup()
{
    return &LRU_Operation;
}
