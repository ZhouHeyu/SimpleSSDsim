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

void HitCLRU(int LPN,int operation)
{
    int HitIndex,tempBlkNum;
    int victim=-1;
    int free_pos=-1,NewIndex;
    int i;
    //首先更新对应的NandPage的age状态,之后别忘了更新其他的状态
    ADCTNandPage[LPN].cache_age=ADCTNandPage[cache_max_index].cache_age+1;
    cache_max_index=LPN;
    //从CLRU的数组中找到存放LPN的位置
    HitIndex=search_table(clru_cache_arr,ADCT_MAX_CACHE_SIZE,LPN);
    //错误判读
    if(HitIndex==-1){
        printf("error happend in HitCLRU :can not Find LPN %d in clru_cache_arr\n",LPN);
        assert(0);
    }

    //根据命中的类型（写需要移动到DLRU队列中去，更复杂）
    if(operation!=0){
        //读命中,不需要移动数据项
        CDHit_CRH++;
        buffer_read_hit++;
        cache_read_num++;
    }else{
        //写命中，需要移动数据项
        CDHit_CWH++;
        buffer_write_hit++;
        cache_write_num++;
        //删除clru_cache中的数据
        clru_cache_arr[HitIndex]=-1;
        ADCT_CLRU_CACHE_SIZE--;
        //将其存入DLRU队列
        free_pos=find_free_pos(dlru_cache_arr,ADCT_MAX_CACHE_SIZE);
        if(free_pos==-1){
            printf("error happen in HitCLRU, can not find free_pos in dlr-cache-arr\n");
            exit(-1);
        }
        dlru_cache_arr[free_pos]=LPN;
        ADCT_DLRU_CACHE_SIZE++;
        //更新NandPage的标志位
        ADCTNandPage[LPN].cache_status=CACHE_INDLRU;
        ADCTNandPage[LPN].cache_update=1;

        //更新块索引的数据
        tempBlkNum=LPN/PAGE_NUM_PER_BLK;
        //删除对应clist上的索引(HitIndex)，将新的dlru位置索引(free_pos)加入dlist
        victim=search_table(BlkTable[tempBlkNum].Clist,PAGE_NUM_PER_BLK,HitIndex);
        //错误检测
        if(victim==-1){
            printf("error happend in HitInCLRU can not find HitIndex:%d In BLkTable[%d]-Clist\n",HitIndex,tempBlkNum);
            printf("the Clist Num is %d\t CleanNum is %d\n",CL,BlkTable[tempBlkNum].CleanNum);
            //依次打印输出当前的Clist的队列的值
            for(i=0;i<PAGE_NUM_PER_BLK;i++){
                if(BlkTable[tempBlkNum].Clist[i]!=-1){
                    printf("%d\t",BlkTable[tempBlkNum].Clist[i]);
                }
            }
            assert(0);
        }
        BlkTable[tempBlkNum].Clist[victim]=-1;
        BlkTable[tempBlkNum].CleanNum--;

        NewIndex=free_pos;//NewIndex是之前dlru_cache_arr的位置索引
        free_pos=find_free_pos(BlkTable[tempBlkNum].Dlist,PAGE_NUM_PER_BLK);
        //错误检测
        if(free_pos==-1){
            printf("error happend in HitCLRU can not find free_pos in BLKTable[%d]-Dlist for NewIndex %d\n",tempBlkNum,NewIndex);
            printf("the Dlist Num is %d\t DirtyNum is %d\n",DL,BlkTable[tempBlkNum].DirtyNum);
            //依次打印输出当前的Dlist的队列的值
            for(i=0;i<PAGE_NUM_PER_BLK;i++){
                if(BlkTable[tempBlkNum].Dlist[i]!=-1){
                    printf("%d\n",BlkTable[tempBlkNum].Dlist[i]);
                }
            }
            exit(-1);
        }
        BlkTable[tempBlkNum].Dlist[free_pos]=NewIndex;
        BlkTable[tempBlkNum].DirtyNum++;
        //错误检测需要的值
        int CL=calculate_arr_positive_num(BlkTable[tempBlkNum].Clist,PAGE_NUM_PER_BLK);
        int DL=calculate_arr_positive_num(BlkTable[tempBlkNum].Dlist,PAGE_NUM_PER_BLK);
//        做一个错误检测判断CL和DL是否一直一致
        if(CL!=BlkTable[tempBlkNum].CleanNum||DL!=BlkTable[tempBlkNum].DirtyNum){
            printf("error happend in HitCLRU,Clist or Dlist size is error\n");
            printf("the Dlist Num is %d\t DirtyNum is %d\n",DL,BlkTable[tempBlkNum].DirtyNum);
            printf("the Clist Num is %d\t CleanNum is %d\n",CL,BlkTable[tempBlkNum].CleanNum);
            exit(-1);
        }
    }

}


//命中DLRU的操作
void HitDLRU(int LPN,int operation)
{
    //更新对应的NandPage的状态标识
    ADCTNandPage[LPN].cache_age=ADCTNandPage[cache_max_index].cache_age+1;
    cache_max_index=LPN;
    if(operation==0){
        CDHit_DWH++;
        cache_write_num++;
        buffer_write_hit++;
    }else{
        CDHit_DRH++;
        cache_read_num++;
        buffer_read_hit++;
    }
}


//删除CLRU的lru位置的数据项
void DelLPNInCLRU()
{
    int MinAgeLPN,Victim;
    //BlkTable删除需要的中间变量
    int ClistVictim,ClistIndex;
    int tempBlkNum;

    //注意这里返回的是LPN号,不是位置索引
    MinAgeLPN=my_find_cache_min(clru_cache_arr,CACHE_MAX_ENTRIES);
    //重置NandPage相关的状态
    ResetNandPageStat(MinAgeLPN);
    Victim=search_table(clru_cache_arr,CACHE_MAX_ENTRIES,MinAgeLPN);
    //错误检测
    if(Victim==-1){
        printf("error happend in DelLPNInCLRU: can not find MinAgeLPN %d In clru-cache-arr\n",MinAgeLPN);
        exit(1);
    }
    //删除clru中的数据
    clru_cache_arr[Victim]=-1;
    CACHE_CLRU_NUM_ENTRIES--;
    //删除对应的LPN的块索引
    tempBlkNum=MinAgeLPN/PAGE_NUM_PER_BLK;
//    遍历找到对应删除的MinAgeLPN(LPN)-->Victim(ClistVictim)在Clist上的位置(ClistIndex)
    ClistVictim=Victim;
    ClistIndex=search_table(BlkTable[tempBlkNum].Clist,PAGE_NUM_PER_BLK,ClistVictim);
    //错误检测
    if(ClistIndex==-1){
        printf("error happend in DelLPNInCLRU: can not find ClistVictim %d In BlkTable[%d]-Clist\n",ClistVictim,tempBlkNum);
        exit(1);
    }
    //删除对应的数据项
    BlkTable[tempBlkNum].Clist[ClistIndex]=-1;
    //对应的统计量的修改
    BlkTable[tempBlkNum].BlkSize--;
    BlkTable[tempBlkNum].CleanNum--;
    //错误检测
    if(BlkTable[tempBlkNum].CleanNum+BlkTable[tempBlkNum].DirtyNum!=BlkTable[tempBlkNum].BlkSize){
        printf("error happend in DelLPNinCLRU: BlkTableSize is error\n");
        printf("BlkTable[%d]-CleanNum is %d\t DirtyNum is %d\t BlkSize is %d\n",tempBlkNum,BlkTable[tempBlkNum].CleanNum,BlkTable[tempBlkNum].DirtyNum,BlkTable[tempBlkNum].BlkSize);
        printf("Clist-num is %d\t Dlist num is %d\n",calculate_arr_positive_num(BlkTable[tempBlkNum].Clist,PAGE_NUM_PER_BLK),calculate_arr_positive_num(BlkTable[tempBlkNum].Dlist,PAGE_NUM_PER_BLK));
        exit(-1);
    }
    //错误检测
    if(BlkTable[tempBlkNum].CleanNum!=calculate_arr_positive_num(BlkTable[tempBlkNum].Clist,PAGE_NUM_PER_BLK)){
        printf("error happend in DelLPNinCLRU:Clist size is error\n");
        printf("BlkTable[%d]-CleanNum is %d\t Clist-num is %d\n",tempBlkNum,BlkTable[tempBlkNum].CleanNum,calculate_arr_positive_num(BlkTable[tempBlkNum].Clist,PAGE_NUM_PER_BLK));
        exit(-1);
    }
}

/*************************************************************/

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
    if(ADCTNandPage[LPN].cache_status==CACHE_INCLRU){
        type=0;
    }else if(ADCTNandPage[LPN].cache_status==CACHE_INDLRU){
        type=1;
    }else{
        type=-1;
    }

    return type;
}



//Hit_index表示命中的队列类型
int ADCT_HitCache (int LPN,int operation,int Hit_kindex)
{
//  统计命中次数
    buffer_hit_cnt++;
    if(Hit_kindex==0){
//      命中CLRU的操作
        HitCLRU(LPN,operation);
    }else if(Hit_kindex==1){
        HitDLRU(LPN,operation);
    }else{
        fprintf(stderr,"ADCT Hit Cache error !!\n");
        assert(0);
    }
  return 0;
}



double ADCT_AddCacheEntry(int LPN,int operation)
{
    double delay=0.0;
    return delay;
}


//判断当前缓冲区是否溢出
double ADCT_DelCacheEntry(int ReqLPN,int ReqOperation)
{
    double delay=0.0;
    if(ADCT_CLRU_CACHE_SIZE+ADCT_DLRU_CACHE_SIZE<ADCT_MAX_CACHE_SIZE){
        return delay;
    }

//  debug test
    if(ADCT_CLRU_CACHE_SIZE!=calculate_arr_positive_num(clru_cache_arr,ADCT_MAX_CACHE_SIZE)||ADCT_DLRU_CACHE_SIZE!=calculate_arr_positive_num(dlru_cache_arr,ADCT_MAX_CACHE_SIZE)){
        printf("ADCT_CLRU_CACHE_SIZE is %d\t clru-arr num is %d\n",ADCT_CLRU_CACHE_SIZE,calculate_arr_positive_num(clru_cache_arr,ADCT_MAX_CACHE_SIZE));
        printf("ADCT_DLRU_CACHE_SIZE is %d\t dlru-arr num is %d\n",ADCT_DLRU_CACHE_SIZE,calculate_arr_positive_num(dlru_cache_arr,ADCT_MAX_CACHE_SIZE));
        assert(0);
    }
    //根据Tau选择删除CLRU还是DLRU
    if(ADCT_CLRU_CACHE_SIZE>=ADCT_Tau){
//        选择删除CLRU,不涉及读写延迟
        DelLPNInCLRU();
    }else{
//        涉及到回写操作，会有flash_delay的延迟
        delay+=DelLPNInDLRU();
    }


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
