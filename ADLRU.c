//
// Created by mao on 18-1-16.
//

#include "ADLRU.h"
#include <stdlib.h>
#include "global.h"
#include "List.h"
#include "flash.h"
#include "Interface.h"


int ADLRU_init(int cache_size,int blk_num)
{
    ADLRU_CACHE_MAX_SIZE=cache_size;
    COLD_MIN=(int)(ADLRU_MIN_LC*ADLRU_CACHE_MAX_SIZE+0.5);
    ColdLRU_Head=CreateList();

    if(ColdLRU_Head==NULL){
        fprintf(stderr,"error happaned in ADLRU_Init function\n");
        fprintf(stderr,"malloc for ColdLRU_Head failed\n");
        assert(0);
    }
    COLD_CACHE_SIZE=0;
    HotLRU_Head=CreateList();
    if(HotLRU_Head==NULL){
        fprintf(stderr,"error happaned in ADLRU_Init function\n");
        fprintf(stderr,"malloc for HotLRU_Head failed\n");
        assert(0);
    }
    HOT_CACHE_SIZE=0;

    return 0;
}


//结束释放内存
void ADLRU_end()
{
    FreeList(&ColdLRU_Head);
    FreeList(&HotLRU_Head);
}

///热的index==1 冷的index==0,没有命中则是-1
int ADLRU_Search(int LPN,int operation)
{
    int index=-1;
    pNode Ps=NULL;
    Ps=FindLPNinList(ColdLRU_Head,LPN);
    if(Ps==NULL){
        Ps=FindLPNinList(HotLRU_Head,LPN);
        if(Ps!=NULL){
            index=1;
        }
    }else{
        index=0;
    }

    return index;
}



int ADLRU_HitCache(int LPN,int operation,int HitIndex)
{
//    区分命中的操作是命中HotLRU还是命中ColdLRU
    pNode Ps=NULL;

    if(HitIndex==-1){
        fprintf(stderr,"error happened in ADLRU_HitCache\n");
        fprintf(stderr,"HitIndex is -1!!\n");
        assert(0);
    }

//    根据命中的情况,在对应的队列中找到对应的Ps
    if(HitIndex==0)
    {
//        hit cold region
        Ps=FindLPNinList(ColdLRU_Head,LPN);
    }
    else
    {
//        hit hot region
        Ps=FindLPNinList(HotLRU_Head,LPN);
    }
//      debug test
    if(Ps==NULL){
        fprintf(stderr,"error happened in ADLRU_HitCache\n");
        fprintf(stderr,"Can not find LPN %d in CacheList\n",LPN);
        assert(0);
    }
//    针对不同的读写命中修改统计信息和对应的标识位
    buffer_hit_cnt++;
    if(operation==0){
        buffer_write_hit++;
        cache_write_num++;
        Ps->isD=1;
    }else{
        buffer_read_hit++;
        cache_read_num++;
    }
//   再次命中的时候,冷位标识符置位0
    Ps->isCold=0;
//  冷区命中的数据页移动到热区中去,命中热区的也移动到热区的MRU位置
    MoveToMRU(HotLRU_Head,Ps);
    if(HitIndex==0){
//        冷区中的数据页移动到热区中去,修改对应的长度统计信息
        HOT_CACHE_SIZE++;
        COLD_CACHE_SIZE--;
    }

//    debug test
    if(COLD_CACHE_SIZE!=GetListLength(ColdLRU_Head)||HOT_CACHE_SIZE!=GetListLength(HotLRU_Head)){
        fprintf(stderr,"error happened in ADLRU_HitCache\n");
        fprintf(stderr,"COLD_CACHE_SIZE is %d\t cold-list size is %d\n",COLD_CACHE_SIZE,GetListLength(ColdLRU_Head));
        fprintf(stderr,"HOT_CACHE_SIZE is %d\t hot-list size is %d\n",HOT_CACHE_SIZE,GetListLength(HotLRU_Head));
    }
//      命中操作，数据页迁移完成

    return 0;

}


//第一次加载的数据页无论读写都加载到冷区
double  ADLRU_AddCacheEntry(int LPN,int operation)
{
    double delay=0.0;
    pNode p_new=NULL;
    p_new=(pNode)malloc(sizeof(Node));

    if(p_new==NULL){
        fprintf(stderr,"error happened in ADLRU_AddCacheEntry:\n");
        fprintf(stderr,"malloc for p_new failed\n");
        assert(0);
    }
//    第一次加载的数据页的isCold位置位为0,扫描到的isCold=1的数据页作为冷页剔除
    p_new->LPN=LPN;
    p_new->isCold=0;
//    cache操作的统计
    buffer_miss_cnt++;
    if(operation==0){
        buffer_write_miss++;
        cache_write_num++;
        p_new->isD=1;
    }else{
        buffer_read_miss++;
        cache_read_num++;
        p_new->isD=0;
    }
//    第一次访问的数据都放到冷区中去
    AddNewToMRU(ColdLRU_Head,p_new);
    COLD_CACHE_SIZE++;
//   错误检测
    if(COLD_CACHE_SIZE!=GetListLength(ColdLRU_Head)){
        fprintf(stderr,"error happened in ADLRU_AddCacheEntry:\n");
        fprintf(stderr,"COLD_CACHE_SIZE is %d\t ColdLRU-List size is %d\n",COLD_CACHE_SIZE,GetListLength(ColdLRU_Head));
        assert(0);
    }
    delay+=callFsim(LPN*4,4,1);
    physical_read++;

    return delay;
}


//关于缓冲区的剔除都是基于二次机会的,无条件优先置换对应队列中的干净页
double ADLRU_DelCacheEntry(int ReqLPN,int ReqOperation)
{
    double delay = 0.0;
    int DelLPN = -1;

    pNode pVictim = NULL;
//    如果缓冲区没有满,没有必要进行删除操作
    if (COLD_CACHE_SIZE + HOT_CACHE_SIZE < ADLRU_CACHE_MAX_SIZE) {
//        如果缓冲区没有溢出，则没必要执行置换操作
        return delay;
    }
//  剔除操作的时候,比较选择冷区还是热区的剔除页
    if(COLD_CACHE_SIZE>COLD_MIN){
//        选择冷区中的页进行剔除
        pVictim= FindVictimNode_CleanFirst(ColdLRU_Head);
//        debug
        if(pVictim==NULL){
            fprintf(stderr,"error happend in ADLRU_DelCacheEntry!\n");
            fprintf(stderr,"can not find Victim in COLD-list!\n");
            assert(0);
        }
        DelLPN=pVictim->LPN;
//       如果选择的是干净页,则直接删除即可,脏页则需要回写操作
        if(pVictim->isD!=0){
            delay+=callFsim(DelLPN*4,4,0);
            physical_write++;
        }
        DelVictimNodeInList(ColdLRU_Head,pVictim);
        COLD_CACHE_SIZE--;

    }else{
//        选择热区中的页进行剔除
        pVictim= FindVictimNode_CleanFirst(HotLRU_Head);
        if(pVictim==NULL){
            fprintf(stderr,"error happend in ADLRU_DelCacheEntry!\n");
            fprintf(stderr,"can not find Victim in Hot-list!\n");
            assert(0);
        }
        DelLPN=pVictim->LPN;
//       如果选择的是干净页,则直接删除即可,脏页则需要回写操作
        if(pVictim->isD!=0){
            delay+=callFsim(DelLPN*4,4,0);
            physical_write++;
        }
        DelVictimNodeInList(HotLRU_Head,pVictim);
        HOT_CACHE_SIZE--;
    }
//  错误判断
    if(HOT_CACHE_SIZE !=GetListLength(HotLRU_Head) || COLD_CACHE_SIZE !=GetListLength(ColdLRU_Head)){
        fprintf(stderr,"error happend in ADLRU_DelCacheEntry!\n");
        fprintf(stderr,"HOT_CACHE_SIZE is %d\t Hot-list is %d\n",HOT_CACHE_SIZE,GetListLength(HotLRU_Head));
        fprintf(stderr,"COLD_CACHE_SIZE is %d\t Cold-list is %d\n",COLD_CACHE_SIZE,GetListLength(ColdLRU_Head));
        assert(0);
    }

    return delay;
}



struct cache_operation ADLRU_Operation={
        init:   ADLRU_init,
        SearchCache:    ADLRU_Search,
        HitCache:   ADLRU_HitCache,
        AddCacheEntry:  ADLRU_AddCacheEntry,
        DelCacheEntry:  ADLRU_DelCacheEntry,
        end:    ADLRU_end
};



struct cache_operation * ADLRU_op_setup()
{
    return &ADLRU_Operation;
}