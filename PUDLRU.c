//
// Created by ymb on 18/2/2.
//

#include "PUDLRU.h"
#include <stdlib.h>
#include "global.h"
#include "flash.h"
#include "Interface.h"
#include "Cache.h"


int PUD_init(int size, int DataBlk_Num)
{
    int i;
    PUD_Cache_Max_Entry=size;
    PUD_Cache_Num_Entry=0;
    Proportion=0.05;
    Update_count=0;
    PUDBlk_Num=DataBlk_Num;
    // PUDpage 直接通过LPN查找的页数组
    PUDPage_Num=DataBlk_Num*PAGE_NUM_PER_BLK;
    PUDPage=(struct PUD_Cache_entry*)malloc(sizeof(struct PUD_Cache_entry)*PUDPage_Num);
    if(PUDPage==NULL){
        printf("the create PUDpage Memeory is failed\n");
        exit(1);
    }
    memset(PUDPage,0xFF,sizeof(struct PUD_Cache_entry)*PUDPage_Num);
    for ( i = 0; i <PUDPage_Num ; ++i) {
        PUDPage[i].cache_age=0;
        PUDPage[i].cache_status=0;
        PUDPage[i].cache_update=0;
    }
   // PUDBlkArr存在缓冲区中的数据块
    PUDBlkArr=(struct PUD_Block_entry*)malloc(sizeof(struct PUD_Block_entry)*DataBlk_Num);
    if(PUDBlkArr==NULL){
        printf("the create LRUpage Memeory is failed\n");
        exit(1);
    }
    memset(PUDBlkArr,0xFF,sizeof(struct PUD_Block_entry)*DataBlk_Num);
    for ( i = 0; i <DataBlk_Num ; ++i) {
        PUDBlkArr[i].blk_UD=0;
        PUDBlkArr[i].blk_RD=0;
        PUDBlkArr[i].blk_PUD=0;
        PUDBlkArr[i].blk_frequency=0;
        PUDBlkArr[i].blk_last_hit=0;
        PUDBlkArr[i].blk_size=0;
        PUDBlkArr[i].blk_status=0;
    }
}

void UpdateUD(int LBN)
{
    int UD=PUDBlkArr[LBN].blk_UD;
    int freq=PUDBlkArr[LBN].blk_frequency;
    int last_hit=PUDBlkArr[LBN].blk_last_hit;
    PUDBlkArr[LBN].blk_UD=((UD*freq)+(Update_count-last_hit-1))/(freq+1);
    PUDBlkArr[LBN].blk_last_hit=Update_count;
}

int Del_Blk()
{
    int length=0,F_length=0;
    double Blktemp[3][PUDBlk_Num];
    for(int i=0;i<PUDBlk_Num;i++)
    {
        if(PUDBlkArr[i].blk_status==CACHE_VALID)
        {
            //更新所以缓冲区中块的PUD
            int last_hit=PUDBlkArr[i].blk_last_hit;
            PUDBlkArr[i].blk_RD=Update_count-last_hit;
            PUDBlkArr[i].blk_PUD=(PUDBlkArr[i].blk_UD+PUDBlkArr[i].blk_RD)/2;

            Blktemp[0][length]=i;
            Blktemp[1][length]=PUDBlkArr[i].blk_size;
            Blktemp[2][length]=PUDBlkArr[i].blk_PUD;
            length++;
        }
    }
    //printf("the length=%d\n",length);

    // 按脏页排序从大到小
    F_length=length*Proportion;
    for(int i=0;i<F_length+1;i++)
    {
        for(int j=i;j<length;j++)
        {
            if(Blktemp[1][i]<Blktemp[1][j])
            {
                int temp[3];
                for(int z=0;z<3;z++)temp[z]=Blktemp[z][i];
                for(int z=0;z<3;z++)Blktemp[z][i]=Blktemp[z][j];
                for(int z=0;z<3;z++)Blktemp[z][j]=temp[z];
            }
        }
    }
    //获得在IUG中页数最多的块
    int PUD_Rang=1;
    for(int i=0;i<F_length+1;i++)
    {
        if(Blktemp[2][i]>PUD_Rang)
            return Blktemp[0][i];
    }
    printf("error at Del_Blk, because of no return value");
    return Blktemp[0][0];
}


//释放对应的内存
void PUD_end()
{
    if(PUDBlkArr!=NULL){
        free(PUDBlkArr);
    }
    if(PUDPage!=NULL){
        free(PUDPage);
    }
}


//返回匹配的命中请求在数组中的位置
int PUD_Search(int LPN,int operation)
{
    int LBN=LPN/PAGE_NUM_PER_BLK;
    /*-------判断读写----------*/
    if(operation==1)  //读操作
    {
        if(PUDPage[LPN].cache_status==CACHE_INVALID) //缓冲区未命中
        {
            buffer_read_miss++;
            cache_read_num++;
            return -1;
        }
        else return LPN; // 读命中参数写在HitCache中
    }
    else //写操作
    {
        Update_count++;
        if(PUDBlkArr[LBN].blk_status==CACHE_INVALID) return -1;//块未命中
        else //块命中
        {
            PUDBlkArr[LBN].blk_frequency=PUDBlkArr[LBN].blk_frequency+1;
            ///////////////////
            UpdateUD(LBN);
            ///////////////////
            if(PUDPage[LPN].cache_status==CACHE_INVALID) //页未命中
            {
                buffer_write_miss++;
                cache_write_num++;
                return -1;
            }
            else return LPN; //页命中,具体参数写在HitCache中
        }
    }
}

int PUD_HitCache(int LPN,int operation,int Hit_kindex)
{
    int LBN;
    LBN=LPN/PAGE_NUM_PER_BLK;
    buffer_hit_cnt++;
//   统计对应的读写命中情况
    if(operation==0){
        //write
        PUDPage[LPN].cache_update=1;
        buffer_write_hit++;
        cache_write_num++;
    }else{
        //read
        buffer_read_hit++;
        cache_read_num++;
    }
//   将命中的请求移动到LRU位置（因为是数组索引实现，不存在移动的问题）
//    返回命中的请求在arr中的位置索引
    return LPN;
}

double PUD_AddCacheEntry(int LPN,int operation)
{
    int LBN=LPN/PAGE_NUM_PER_BLK;
    double delay=0.0;
    PUD_Cache_Num_Entry++;
    //将新的请求加入到缓冲区中
    physical_read++;
    delay=callFsim(LPN*4,4,1);
    buffer_miss_cnt++;
    //改变相应的标识
    PUDPage[LPN].cache_status=CACHE_VALID;
    PUDBlkArr[LBN].blk_status=CACHE_VALID;
    if(operation==0){
//        write
        PUDPage[LPN].cache_update=1;
        PUDBlkArr[LBN].blk_size++;
        //做一个size的错误检测
        if(PUDBlkArr[LBN].blk_size>PAGE_NUM_PER_BLK)
        {
            fprintf(stderr,"error happened in AddCacheEntry:\n");
            fprintf(stderr,"blk-size compute exist eror: PUD_Blk_size is %d\n",PUDBlkArr[LBN].blk_size);
            fprintf(stderr,"LPN is %d\n",LPN);
            exit(0);
        }
        buffer_write_miss++;
        cache_write_num++;
    }else{
//        read
        buffer_read_miss++;
        cache_read_num++;
    }
//   做一个长度检查：debug
    return delay;
}

//不管缓冲区是否满都调用该函数（满）
double PUD_DelCacheEntry(int ReqLPN,int ReqOperation)
{
    int PageNum=0;
    double delay=0.0;
    int victim_LBN=-1;
    int victim_LPN=-1;
    if(PUD_Cache_Num_Entry<PUD_Cache_Max_Entry){
        return delay;
    }
//    选择IUG中PUD最大的项做剔除对象，函数返回LBN
    victim_LBN=Del_Blk();
    victim_LPN=victim_LBN*PAGE_NUM_PER_BLK;
//   判断剔除块中脏页个数
    for(int i=0;i<PAGE_NUM_PER_BLK;i++)
    {
        if(PUDPage[victim_LPN+i].cache_update==1)
        {
            physical_write++;
            delay=callFsim(victim_LPN*4,4,0);
        }
    }
//  重置相应的PUDpage的状态位
    for(int i=0;i<PAGE_NUM_PER_BLK;i++)
    {
        if(PUDPage[victim_LPN+i].cache_update!=0)
            PUD_Cache_Num_Entry--;
        PUDPage[victim_LPN+i].cache_update=0;
        PUDPage[victim_LPN+i].cache_age=0;
        PUDPage[victim_LPN+i].cache_status=0;
    }
    //重置PUDBlkArr中的状态位
    PUDBlkArr[victim_LBN].blk_UD=0;
    PUDBlkArr[victim_LBN].blk_RD=0;
    PUDBlkArr[victim_LBN].blk_PUD=0;
    PUDBlkArr[victim_LBN].blk_frequency=0;
    PUDBlkArr[victim_LBN].blk_last_hit=0;
    PUDBlkArr[victim_LBN].blk_size=0;
    PUDBlkArr[victim_LBN].blk_status=0;
//   做一个长度检查：debug
    /*if(calculate_arr_positive_num(lru_cache_arr,LRU_Cache_Max_Entry)!=LRU_Cache_Num_Entry){
        fprintf(stderr,"error happened in DelCacheEntry:\n");
        fprintf(stderr,"cache-size compute exist eror: LRU_Cache_Num_Entry is %d\n",LRU_Cache_Num_Entry);
        fprintf(stderr,"lru_cache_arr's valid varibale number is %d\n",calculate_arr_positive_num(lru_cache_arr,LRU_Cache_Max_Entry));
        exit(0);
    }*/
    return delay;
}



struct cache_operation PUDLRU_Operation={
                init:   PUD_init,
                SearchCache:    PUD_Search,
                HitCache:   PUD_HitCache,
                AddCacheEntry:  PUD_AddCacheEntry,
                DelCacheEntry:  PUD_DelCacheEntry,
                end:    PUD_end
        };

struct cache_operation * PUDLRU_op_setup()
{
    return &PUDLRU_Operation;
}
