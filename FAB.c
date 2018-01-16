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
    FAB_CACHE_SIZE=0;
    FAB_BLK_NUM=0;
    FAB_MAX_CACHE_SIZE=size;
//   创建块索引链表的头部节点
    FAB_Head=CreateBlkList();

//
    return 0;
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