//
// Created by zj on 18-1-31.
//

#include "ADCT.h"
#include "Cache.h"
//定义自适应的读写缓冲区阈值调整函数,函数返回的是更新后的Tau值
int ADCT_UpdateTau(int lastTau)
{
    int Tau,D_Tau,TempTau,MinTau,MaxTau;//表示脏队列的目标长度
    //四射五入
    double B_CLRU,B_DLRU;//表示各自队列的单位收益
    MinTau=(int)(MinTauRatio*buf_size+0.5);
    MaxTau=(int)(MaxTauRatio*buf_size+0.5);

    D_Tau=buf_size-Tau;

    //如果从底层得到周期的读写时延
    ave_flash_read_delay=cycle_flash_read_delay/cycle_physical_read;
    ave_flash_write_delay=cycle_flash_write_delay/cycle_physical_write;
    B_CLRU=(CDHit_CRH*ave_flash_read_delay+CDHit_CWH*ave_flash_write_delay)/Tau;
    B_DLRU=(CDHit_DRH*ave_flash_read_delay+CDHit_CWH*ave_flash_write_delay)/D_Tau;


    //四舍五入
    TempTau=(int)((B_CLRU/(B_CLRU+B_DLRU)*buf_size)+0.5);
    TempTau=myMax(MinTau,TempTau);
    TempTau=myMin(MaxTau,TempTau);
    Tau=TempTau;


    //重置相应的周期统计变量
    CDHit_CWH=0;
    CDHit_CRH=0;
    CDHit_DRH=0;
    CDHit_DWH=0;
    T_count=1;
    cycle_physical_write=0;
    cycle_physical_read=0;
    cycle_flash_write_delay=0.0;
    cycle_flash_read_delay=0.0;

    return Tau;
}

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
