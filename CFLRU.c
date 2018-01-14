//
// Created by zj on 18-1-14.
//

#include "CFLRU.h"
#include <stdlib.h>
#include "global.h"
#include "flash.h"
#include "Interface.h"

int CFLRU_init(int size,int blk_num)
{

}

void CFLRU_end()
{

}

int CFLRU_Search()
{

}

int CFLRU_HitCache()
{

}

double CFLRU_AddCacheEntry()
{

}

double CFLRU_DelCacheEntry()
{

}

struct cache_operation CFLRU_Operation={
        init:   CFLRU_init,
        SearchCache:    CFLRU_Search,
        HitCache:   CFLRU_HitCache,
        AddCacheEntry:  CFLRU_AddCacheEntry,
        DelCacheEntry:  CFLRU_DelCacheEntry,
        end:    CFLRU_end
};

struct cache_operation * CFLRU_op_setup()
{
    return &CFLRU_Operation;
}