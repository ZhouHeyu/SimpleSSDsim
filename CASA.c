//
// Created by zj on 18-1-15.
//

#include "CASA.h"
#include <stdlib.h>
#include "global.h"
#include "List.h"
#include "flash.h"
#include "Interface.h"


//如果命中的是CLRU队列增大Tau,反之减少Tau
int AdjustCASATau(int HitType)
{
    double ReadCost,WriteCost;
    double Temp;
    int TauAdujust;
    int DL,CL;
    ReadCost=CASA_FLASH_READ_DELAY/(CASA_FLASH_READ_DELAY+CASA_FLASH_WRITE_DELAY);
    WriteCost=CASA_FLASH_WRITE_DELAY/(CASA_FLASH_READ_DELAY+CASA_FLASH_WRITE_DELAY);
//    做一个简单的数据处理
    DL=DLRU_CACHE_SIZE;
    CL=CLRU_CACHE_SIZE;

    if(HitType==0){
        Temp=((double)DL/CL)*(WriteCost/ReadCost);
//       四舍五入计算
        TauAdujust=(int)(Temp+0.5);
        CASA_Tau=CASA_Tau+TauAdujust;
//        做一个越界的操作纠正
        if(CASA_Tau>=CASA_CACHE_Max_SIZE){
            CASA_Tau=CASA_CACHE_Max_SIZE-1;
        }
    }else {
//        命中的是DLRU的话减少Tau的值
        Temp=((double)CL/DL)*(WriteCost/ReadCost);
        TauAdujust=(int)(Temp+0.5);
        CASA_Tau=CASA_Tau-TauAdujust;
//        做越界的操作纠正
        if(CASA_Tau<=0){
            CASA_Tau=1;
        }
    }
        return 0;
}



//初始化相关的变量
int CASA_init(int cache_size,int blk_num)
{
    CASA_CACHE_Max_SIZE=cache_size;
    CASA_Tau=(int)(CASA_Tau_Ratio*CASA_CACHE_Max_SIZE+0.5);
    CLRU_Head=CreateList();
    if(CLRU_Head==NULL){
        fprintf(stderr,"error happaned in CASA_Init function\n");
        fprintf(stderr,"malloc for CLRU_Head failed\n");
        assert(0);
    }
    CLRU_CACHE_SIZE=0;
    DLRU_Head=CreateList();
    if(DLRU_Head==NULL){
        fprintf(stderr,"error happaned in CASA_Init function\n");
        fprintf(stderr,"malloc for DLRU_Head failed\n");
        assert(0);
    }
    DLRU_CACHE_SIZE=0;
//    第一读写的延迟是底层的flash的单次读写延迟,之后实时更新
    CASA_FLASH_WRITE_DELAY=WRITE_DELAY;
    CASA_FLASH_READ_DELAY=READ_DELAY;
    return 0;
}
//结束释放内存


void CASA_end()
{
    FreeList(&CLRU_Head);
    FreeList(&DLRU_Head);
}

//变量缓冲区,返回索引的结果,返回-1未命中,0命中的CLRU,1命中的是DLRU
int CASA_Search(int LPN,int operation)
{
    int index=-1;
    pNode Ps=NULL;
    Ps=FindLPNinList(CLRU_Head,LPN);
    if(Ps==NULL){
        Ps=FindLPNinList(DLRU_Head,LPN);
        if(Ps!=NULL){
            //命中的是DLRU
            index=1;
        }
    }else{
//        命中的是CLRU
        index=0;
    }

    return index;
}

//命中缓冲区的操作，这里第三个参数变为命中类型使用
int CASA_HitCache(int LPN,int operation,int HitIndex)
{
//    区分命中的操作是命中CLRU还是命中DLRU
    pNode Ps=NULL;
//    启用自适应的Tau值调整
    if(HitIndex==-1){
        fprintf(stderr,"error happened in CASA_HitCache\n");
        fprintf(stderr,"HitIndex is -1!!\n");
        assert(0);
    }
    AdjustCASATau(HitIndex);
//    命中的移动操作,写命中都是将命中的数据页移动到DLRU队列中
//    根据命中的情况,在对应的队列中找到对应的Ps
    if(HitIndex==0){
        Ps=FindLPNinList(CLRU_Head,LPN);
    }else{
        Ps=FindLPNinList(DLRU_Head,LPN);
    }
//错误检测
    if(Ps==NULL){
        fprintf(stderr,"error happened in CASA_HitCache\n");
        fprintf(stderr,"Can not find LPN %d in CacheList\n",LPN);
        assert(0);
    }
//              命中操作
    buffer_hit_cnt++;
    if(HitIndex==0){
        //命中的是CLRU但是是写命中,需要移动到DLRU
        if(operation==0){
//            命中统计
            buffer_write_hit++;
            cache_write_num++;
            Ps->isD=1;
//           命中的是CLRU,CLRU长度会减少
            MoveToMRU(DLRU_Head,Ps);
            DLRU_CACHE_SIZE++;
            CLRU_CACHE_SIZE--;
//           做一个长度的检测
            if(DLRU_CACHE_SIZE!=GetListLength(DLRU_Head)||CLRU_CACHE_SIZE!=GetListLength(CLRU_Head)){
                fprintf(stderr,"error happened in CASA_HitCache\n");
                fprintf(stderr,"DLRU_CACHE_SIZE is %d\t DLRU-List size is %d\n",DLRU_CACHE_SIZE,GetListLength(DLRU_Head));
                fprintf(stderr,"CLRU_CACHE_SIZE is %d\t CLRU-List size is %d\n",CLRU_CACHE_SIZE,GetListLength(CLRU_Head));
                assert(0);
            }

        }else{
//            命中的是CLRU，但是命中的是读命中
            buffer_read_hit++;
            cache_read_num++;
//            将命中移动到CLRU的头部
            MoveToMRU(CLRU_Head,Ps);
//             做一个长度的检测
            if(DLRU_CACHE_SIZE!=GetListLength(DLRU_Head)||CLRU_CACHE_SIZE!=GetListLength(CLRU_Head)){
                fprintf(stderr,"error happened in CASA_HitCache\n");
                fprintf(stderr,"DLRU_CACHE_SIZE is %d\t DLRU-List size is %d\n",DLRU_CACHE_SIZE,GetListLength(DLRU_Head));
                fprintf(stderr,"CLRU_CACHE_SIZE is %d\t CLRU-List size is %d\n",CLRU_CACHE_SIZE,GetListLength(CLRU_Head));
                assert(0);
            }
        }

    }else{
//        命中的是DLRU，都移动到DLRU队列的MRU位置
        if(operation==0){
            buffer_write_hit++;
            cache_write_num++;
        }else{
            buffer_read_hit++;
            cache_read_num++;
        }
        MoveToMRU(DLRU_Head,Ps);

    }
//      命中操作，数据页迁移完成

    return 0;

}



//未命中加载新的数据到缓冲区的操作,函数最后加载数据的时延delay
double  CASA_AddCacheEntry(int LPN,int operation)
{
    double delay=0.0;
    pNode p_new=NULL;
    p_new=(pNode)malloc(sizeof(Node));

    if(p_new==NULL){
        fprintf(stderr,"error happened in CASA_AddCacheEntry:\n");
        fprintf(stderr,"malloc for p_new failed\n");
        assert(0);
    }
    p_new->LPN=LPN;
    buffer_miss_cnt++;
    if(operation==0){
        buffer_write_miss++;
        cache_write_num++;
        p_new->isD=1;
        AddNewToMRU(DLRU_Head,p_new);
        DLRU_CACHE_SIZE++;
//        错误检测
        if(DLRU_CACHE_SIZE!=GetListLength(DLRU_Head)){
            fprintf(stderr,"error happened in CASA_AddCacheEntry:\n");
            fprintf(stderr,"DLRU_CACHE_SIZE is %d\t DLRU-List size is %d\n",DLRU_CACHE_SIZE,GetListLength(DLRU_Head));
            assert(0);
        }

    }else{
        buffer_read_miss++;
        cache_read_num++;
        p_new->isD=0;
        AddNewToMRU(CLRU_Head,p_new);
        CLRU_CACHE_SIZE++;
//        错误检测
        if(CLRU_CACHE_SIZE!=GetListLength(CLRU_Head)){
            fprintf(stderr,"error happened in CASA_AddCacheEntry:\n");
            fprintf(stderr,"CLRU_CACHE_SIZE is %d\t CLRU-List size is %d\n",CLRU_CACHE_SIZE,GetListLength(CLRU_Head));
            assert(0);
        }
    }
//    从底层读取数据页到缓冲区,更新Tau所需要的读写时延
    CASA_FLASH_READ_DELAY=callFsim(LPN*4,4,1);
    delay+=CASA_FLASH_READ_DELAY;
    physical_read++;

    return delay;
}

//缓冲区满的时候,删除脏页可能引发新的时延delay
double CASA_DelCacheEntry()
{
    double delay=0.0;
    pNode pVictim=NULL;
    int DelLPN=-1;
    if(CLRU_CACHE_SIZE+DLRU_CACHE_SIZE<CASA_CACHE_Max_SIZE){
//        如果缓冲区没有溢出，则没必要执行置换操作
        return delay;
    }

//    根据Tau值和CLRU的长度选择剔除的队列
    if(CLRU_CACHE_SIZE>=CASA_Tau){
//        选择CLRU剔除
//        删除链表的LRU位置的数据页
        DeleteLRU(CLRU_Head,&DelLPN);
        CLRU_CACHE_SIZE--;
//        长度错误检测
        if(CLRU_CACHE_SIZE!=GetListLength(CLRU_Head)){
            fprintf(stderr,"error happened in CASA_DelCacheEntry:\n");
            fprintf(stderr,"CLRU_CACHE_SIZE is %d\t CLRU-List size is %d\n",CLRU_CACHE_SIZE,GetListLength(CLRU_Head));
            assert(0);
        }
//        干净页不需要回写
    }else{
//        选择DLRU剔除
        DeleteLRU(DLRU_Head,&DelLPN);
        DLRU_CACHE_SIZE--;
//             长度错误检测
        if(DLRU_CACHE_SIZE!=GetListLength(DLRU_Head)){
            fprintf(stderr,"error happened in CASA_DelCacheEntry:\n");
            fprintf(stderr,"DLRU_CACHE_SIZE is %d\t DLRU-List size is %d\n",DLRU_CACHE_SIZE,GetListLength(DLRU_Head));
            assert(0);
        }
//        判断DelLPN是否正常
        if(DelLPN<0){
            fprintf(stderr,"error happened in CASA_DelCacheEntry:\n");
            fprintf(stderr,"DelLPN is %d  !!!error \n",DelLPN);
            assert(0);
        }
//        脏页回写
        physical_write++;
        CASA_FLASH_WRITE_DELAY=callFsim(DelLPN*4,4,0);
        delay+=CASA_FLASH_WRITE_DELAY;
    }


    return delay;
}



struct cache_operation CASA_Operation={
        init:   CASA_init,
        SearchCache:    CASA_Search,
        HitCache:   CASA_HitCache,
        AddCacheEntry:  CASA_AddCacheEntry,
        DelCacheEntry:  CASA_DelCacheEntry,
        end:    CASA_end
};

struct cache_operation * CASA_op_setup()
{
    return &CASA_Operation;
}