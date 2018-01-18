//
// Created by zj on 18-1-18.
//需要注意的是BPLRU只是针对写请求处理的,读请求只是查找缓冲区是否存在对应的LPN不存在直接读取flash
//

#include "BPLRU.h"


//用于页补偿,记录上一次的请求的块号
int LastBlkNum;
//记录相同块的累计
int SameBlkHit;
//判断为顺序写的阈值
int Seq_Threshold;
//判定顺序写模式的标识符,块补偿机制通过这个这个标识符进行启动
int Seq_flag;

int BPLRU_init(int cache_size,int blk_num)
{
    BPLRU_BLK_NUM=0;
    BPLRU_Head=CreateBlkList();
    BPLRU_MAX_CACHE_SIZE=cache_size;
    BPLRU_CACHE_SIZE=0;
    LastBlkNum=-1;
    SameBlkHit=0;
    Seq_flag=0;
//    认为顺序写的判断的阈值
    Seq_Threshold=PAGE_NUM_PER_BLK/2;

    return 0;
}

void BPLRU_end()
{
    FreeBlkList(BPLRU_Head);
}


//该函数对
int BPLRU_Search(int LPN,int operation)
{
    int Hit_Flag=-1;
    int tempBlk;
    tempBlk=LPN/PAGE_NUM_PER_BLK;
//  针对第一次请求的处理
    if(LastBlkNum==-1){
        LastBlkNum=tempBlk;
    }

//    辨别是否未顺序写模式
    if(LastBlkNum==tempBlk){
        SameBlkHit++;
        if(SameBlkHit>=Seq_Threshold){
//         如果连续多次命中同一个块,则判断为顺序写,将标识符置位1
            Seq_flag=1;
        } else{
//           如果非连续命中模式,则判断未非顺序写模式,标识符置位0
            Seq_flag=0;
        }
    }else{
//        非连续写请求
        SameBlkHit=0;
        LastBlkNum=tempBlk;
        Seq_flag=0;
    }


    FindHitBlkNode(BPLRU_Head,LPN,&Hit_Flag);

    return Hit_Flag;
}

int BPLRU_HitCache(int LPN,int operation,int type)
{
    return 0;
}

double BPLRU_AddCacheEntry(int LPN,int operation)
{
    double delay=0.0;
    return delay;
}

double BPLRU_DelCacheEntry(int LPN,int operation)
{
    double delay=0.0;
    return delay;
}

struct cache_operation BPLRU_Operation={
        init:   BPLRU_init,
        SearchCache:    BPLRU_Search,
        HitCache:   BPLRU_HitCache,
        AddCacheEntry:  BPLRU_AddCacheEntry,
        DelCacheEntry:  BPLRU_DelCacheEntry,
        end:    BPLRU_end
};

struct cache_operation * BPLRU_op_setup()
{
    return &BPLRU_Operation;
}