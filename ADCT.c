//
// Created by zj on 18-1-31.
//

#include "ADCT.h"

int ADCT_init(int size, int DataBlk_Num)
{
    return 0;
}

void  ADCT_end()
{

}

int ADCT_Search(int LPN,int operation)
{
    return 0;
}

int ADCT_HitCache (int LPN,int operation,int Hit_kindex)
{
  return 0;
}

double ADCT_AddCacheEntry(int LPN,int operation)
{
    double delay;
    return delay;
}

double ADCT_DelCacheEntry(int ReqLPN,int ReqOperation)
{
    double delay;
    return delay;
}

struct cache_operation ADCT_Operation={
        init:   ADCT_init,
        SearchCache:    ADCT_Search,
        HitCache:   ADCT_HitCache,
        AddCacheEntry:  ADCT_AddCacheEntry,
        DelCacheEntry:  ADCT_DelCacheEntry,
        end:    ADCT_end
};

struct cache_operation * ADCT_op_setup()
{
    return &ADCT_Operation;
}
