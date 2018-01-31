//
// Created by zj on 18-1-31.
//

#include "ADCT.h"
#include "global.h"
#include "Cache.h"

//设置对应的命中情况变量
int CDHit_CWH;
int CDHit_CRH;
int CDHit_DRH;
int CDHit_DWH;
int TCount;
//设置周期物理读写的次数和读写时延
int cycle_physical_write;
int cycle_physical_read;
double cycle_flash_read_delay;
double cycle_flash_write_delay;



//设置相应的块节点的链表操作
pADBNode CreateBIndex()
{
    int i;
    pADBNode pHead=(pADBNode)malloc(sizeof(ADBNode));
    if (NULL==pHead){
        printf("malloc for pBHead failed!\n");
        exit(-1);
    }
    pHead->BlkNum=-1;
    pHead->DirtyNum=0;
    pHead->CleanNum=0;
    pHead->Size=0;
    pHead->Pre=pHead;
    pHead->Next=pHead;
    for ( i = 0; i <PAGE_NUM_PER_BLK ; ++i) {
        pHead->CleanList[i]=-1;
        pHead->DirtyList[i]=-1;
    }
    return pHead;
}

void FreeBList(pADBNode pHead)
{
    pADBNode ps,pt=pHead->Next;
    while(pt!=pHead){
        ps=pt;
        pt=pt->Next;
        free(ps);
    }
    free(pHead);
}

void ADCT_Stat_Reset()
{
    //重置相应的周期统计变量
    CDHit_CWH=0;
    CDHit_CRH=0;
    CDHit_DRH=0;
    CDHit_DWH=0;
    TCount=1;
    cycle_physical_write=0;
    cycle_physical_read=0;
    cycle_flash_write_delay=0.0;
    cycle_flash_read_delay=0.0;
}

//定义自适应的读写缓冲区阈值调整函数,函数返回的是更新后的Tau值
int ADCT_UpdateTau(int lastTau)
{
    int Tau,D_Tau,TempTau,MinTau,MaxTau;//表示脏队列的目标长度
    //四射五入
    double B_CLRU,B_DLRU;//表示各自队列的单位收益
    MinTau=(int)(MinTauRatio*ADCT_MAX_CACHE_SIZE+0.5);
    MaxTau=(int)(MaxTauRatio*ADCT_MAX_CACHE_SIZE+0.5);

    D_Tau=ADCT_MAX_CACHE_SIZE-lastTau;

    //如果从底层得到周期的读写时延
    double ave_flash_read_delay=cycle_flash_read_delay/cycle_physical_read;
    double ave_flash_write_delay=cycle_flash_write_delay/cycle_physical_write;
    B_CLRU=(CDHit_CRH*ave_flash_read_delay+CDHit_CWH*ave_flash_write_delay)/lastTau;
    B_DLRU=(CDHit_DRH*ave_flash_read_delay+CDHit_DWH*ave_flash_write_delay)/D_Tau;

    //四舍五入
    TempTau=(int)((B_CLRU/(B_CLRU+B_DLRU)*ADCT_MAX_CACHE_SIZE)+0.5);
    TempTau=min(TempTau,MinTau);
    TempTau=max(TempTau,MaxTau);
    Tau=TempTau;
    //重置相应的周期统计变量
    ADCT_Stat_Reset();

    return Tau;
}



int ADCT_init(int size, int DataBlk_Num)
{
    ADCT_CLRU_CACHE_SIZE=0;
    ADCT_DLRU_CACHE_SIZE=0;
    ADCT_MAX_CACHE_SIZE=size;
//  设置最小的界限
    MinTauRatio=0.1;
    MaxTauRatio=0.9;
    ADCT_DHead=CreateList();
    ADCT_CHead=CreateList();
    ADCT_BLK_NUM=0;
//   设置对应的头节点链表
    ADCT_BHead=CreateBIndex();
    ADCT_Stat_Reset();
//   设置对应的热数据的判别大小和初始的大小Tau的大小,还有周期循环的ADCT_Cycle
//   在SSDSim的参数默认函数中设置
    ADCT_Tau=size/2;

    return 0;
}

void  ADCT_end()
{
    FreeList(ADCT_CHead);
    FreeList(ADCT_DHead);
    FreeBList(ADCT_BHead);
}


//返回的是type=-1表示为命中，0表示命中读缓冲区，1表示命中写缓冲区
int ADCT_Search(int LPN,int operation)
{
    int type=-1;
//   首先换算对应的块是不是在
    return type;
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
