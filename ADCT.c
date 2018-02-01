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
    int i,j;
    ADCT_MAX_CACHE_SIZE=size;

    ADCT_CLRU_CACHE_SIZE=0;
    clru_cache_arr=(int *)malloc(sizeof(int)*size);
    if(clru_cache_arr==NULL){
        fprintf(stderr,"malloc for clru_cache_arr is failed!\n");
        assert(0);
    }
//  快速初始化
    memset(clru_cache_arr,0xFF, sizeof(int)*size);

    ADCT_DLRU_CACHE_SIZE=0;
    dlru_cache_arr=(int *)malloc(sizeof(int)*size);
    if(dlru_cache_arr==NULL){
        fprintf(stderr,"malloc for dlru_cache_arr is failed!\n");
        assert(0);
    }
    memset(dlru_cache_arr,0xFF, sizeof(int)*size);

//   初始化对应的页状态信息
    ADCTPageNum=DataBlk_Num*PAGE_NUM_PER_BLK;
    ADCTNandPage=(struct CachePageEntry *)malloc(sizeof(struct CachePageEntry)*ADCTPageNum);
    if(ADCTNandPage==NULL){
        printf("the create ADCT Nandpage Memeory is failed\n");
        assert(0);
    }
// 初始化内存
    for ( i = 0; i <ADCTPageNum ; ++i) {
        ADCTNandPage[i].cache_status=0;
        ADCTNandPage[i].cache_update=0;
        ADCTNandPage[i].cache_age=0;
    }

    BlkTableNum=DataBlk_Num;
    BlkTable=( struct BlkTable_entry *)malloc(sizeof(struct  BlkTable_entry)*BlkTableNum);
    if(BlkTable==NULL){
        printf("the create ADCT Blktable is failed\n");
        assert(0);
    }
//  初始化函数
    for ( i = 0; i <BlkTableNum; ++i) {
        BlkTable[i].BlkSize=0;
        BlkTable[i].CleanNum=0;
        BlkTable[i].DirtyNum=0;
        for ( j = 0; j <PAGE_NUM_PER_BLK ; ++j) {
            BlkTable[i].Clist[j]=-1;
            BlkTable[i].Dlist[j]=-1;
        }
    }
//  设置最小的界限
    MinTauRatio=0.1;
    MaxTauRatio=0.9;
    ADCT_Tau=size/2;

    return 0;
}

void  ADCT_end()
{
    if(dlru_cache_arr==NULL) {
        free(dlru_cache_arr);
    }
    if(clru_cache_arr==NULL){
        free(clru_cache_arr);
    }
    if(BlkTable){
        free(BlkTable);
    }
    if(ADCTNandPage==NULL){
        free(ADCTNandPage);
    }
}


//返回的是type=-1表示为命中，0表示命中读缓冲区，1表示命中写缓冲区
int ADCT_Search(int LPN,int operation)
{
    int type;
//   首先换算对应的块是不是在
    if(ADCTNandPage[LPN].cache_status==CLRU_VALID){
        type=0;
    }else if(ADCTNandPage[LPN].cache_status==DLRU_VALID){
        type=1;
    }else{
        type=-1;
    }

    return type;
}

//Hit_index表示命中的队列类型
int ADCT_HitCache (int LPN,int operation,int Hit_kindex)
{
    if(Hit_kindex==0){
//      命中CLRU的操作
    }else if(Hit_kindex==1){

    }else{
        fprintf(stderr,"ADCT Hit Cache error !!\n");
        assert(0);
    }
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
