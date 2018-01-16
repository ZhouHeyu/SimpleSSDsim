//
// Created by zj on 18-1-16.
//

#include "FAB.h"
#include <stdlib.h>
#include "global.h"
#include "BlkList.h"
#include "flash.h"
#include "Interface.h"



//外部调用的函数
int FAB_init(int size,int blk_num)
{

}

void FAB_end()
{

}

int FAB_Search(int LPN,int operation)
{

}

int FAB_HitCache(int LPN,int operation,int index)
{

}

double FAB_AddCacheEntry(int LPN,int operation)
{
    double delay=0.0;

    return delay;
}

double FAB_DelCacheEntry()
{
    double delay=0.0;

    return delay;
}

struct cache_operation FAB_Operation={
        init:   FAB_init,
        SearchCache:    FAB_Search,
        HitCache:   FAB_HitCache,
        AddCacheEntry:  FAB_AddCacheEntry,
        DelCacheEntry:  FAB_DelCacheEntry,
        end:    FAB_end
};

struct cache_operation * FAB_op_setup()
{
    return &FAB_Operation;
}