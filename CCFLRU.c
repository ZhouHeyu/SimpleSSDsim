//
// Created by zj on 18-1-20.
//

#include "CCFLRU.h"


int CCFLRU_init(int cache_size,int blk_num)
{
    CCL_Head=CreateList();
    CCL_Size=0;
    ML_Head=CreateList();
    ML_Size=0;
    CCFLRU_Cache_Max_Entry=cache_size;
    CCFLRU_Cache_Num_Entry=0;
    return 0;
}

void CCFLRU_end()
{
    FreeList(CCL_Head);
    FreeList(ML_Head);
}

int CCFLRU_Search(int LPN,int operation)
{
    int type=-1;
    return type;
}


int CCFLRU_HitCache(int LPN,int operation,int HitType)
{

}

double CCFLRU_AddCacheEntry(int LPN ,int operation){
    double delay=0.0;
    return delay;
}

double CCFLRU_DelCacheEntry(int ReqLPN,int ReqOperation)
{
    double delay=0.0;
    return delay;
}


struct cache_operation CCFLRU_Operation={
        init:   CCFLRU_init,
        SearchCache:    CCFLRU_Search,
        HitCache:   CCFLRU_HitCache,
        AddCacheEntry:  CCFLRU_AddCacheEntry,
        DelCacheEntry:  CCFLRU_DelCacheEntry,
        end:    CCFLRU_end
};

struct cache_operation * CCFLRU_op_setup()
{
    return &CCFLRU_Operation;
}