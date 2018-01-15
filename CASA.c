//
// Created by zj on 18-1-15.
//

#include "CASA.h"
#include <stdlib.h>
#include "global.h"
#include "flash.h"
#include "Interface.h"

//创建双向链表
pNode CreateList()
{
    //分配初始内存
    pNode  pHead=(pNode)malloc(sizeof(Node));
    if(NULL==pHead){
        fprintf(stderr,"error happened in CASA/CreateList\n");
        fprintf(stderr,"malloc for pHead failed!\n");
        exit(-1);
    }
    pHead->LPN=-1;
    pHead->isD=-1;
    pHead->Pre=pHead;
    pHead->Next=pHead;
    return pHead;
}


//删除整个链表，释放内存（这里有点小问题)
void FreeList(pNode *ppHead)
{
    pNode pt=NULL;
    while(*ppHead!=NULL){
        pt=(*ppHead)->Next;
        free(*ppHead);
        if(NULL!=pt)
            pt->Pre=NULL;
        *ppHead=pt;
    }
}


//判断链表是否为空
int IsEmptyList(pNode pHead)
{
    pNode pt=pHead->Next;
    if(pt==pHead)
    {
        return 1;
    }else
    {
        return 0;
    }
}

//返回链表的长度
int GetListLength(pNode pHead)
{
    int length=0;
    pNode pt=pHead->Next;
    while (pt !=pHead)
    {
        length++;
        pt=pt->Next;
    }
    return length;
}

//从链表中找到特定的LPN值，并返回节点的指针位置,如果不存在返回NULL
pNode FindLPNinList(pNode pHead,int LPN)
{
    pNode ps=NULL,pt=pHead->Next;
    int count=0;
    while(pt!=pHead)
    {
        count++;
        if(pt->LPN==LPN){
            ps=pt;
            break;
        }
        pt=pt->Next;
    }
    //调试输出语句遍历循环了多少次
//    printf("the while count is %d\n",count);
    return ps;
}



/***********************链表操作(end)******************************************/

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
        if(CASA_Tau>=CASA_CACHE_SIZE){
            CASA_Tau=CASA_CACHE_SIZE-1;
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


//将命中的Node移动到DLRU的头部
void MoveToDlistMRU(pNode HitNode)
{
    pNode Pt=NULL;
//    将命中的HitNode移出原来的位置
    Pt=HitNode->Next;
    HitNode->Pre->Next=Pt;
    Pt->Pre=HitNode->Pre;
//    将HitNode移动到DLRU的Head的后面位置
    HitNode->Next=DLRU_Head->Next;
    HitNode->Pre=DLRU_Head;
//    将命中的HitNode嵌入到其中
    DLRU_Head->Next->Pre=HitNode;
    DLRU_Head->Next=HitNode;
//  只是实现简单的移动,在函数调用后修改对应长度
}



//初始化相关的变量
int CASA_init(int cache_size,int blk_num)
{
    CASA_CACHE_SIZE=cache_size;
    CASA_Tau=(int)(CASA_Tau_Ratio*CASA_CACHE_SIZE+0.5);
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

//变量缓冲区,返回索引的结果,返回-1未命中,1命中的CLRU,2命中的是DLRU
int CASA_Search(int LPN,int operation)
{
    int index=-1;
    pNode Ps=NULL;
    Ps=FindLPNinList(CLRU_Head,LPN);
    if(Ps==NULL){
        Ps=FindLPNinList(DLRU_Head,LPN);
        if(Ps!=NULL){
            //命中的是DLRU
            index=2;
        }
    }else{
//        命中的是CLRU
        index=1;
    }

    return index;
}

//命中缓冲区的操作
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
        exit(0);
    }
//
    if(HitIndex==0){
        //命中的是CLRU但是是写命中,需要移动到DLRU
        if(operation==0){
            Ps->isD=1;
//           命中的是CLRU,CLRU长度会减少
            MoveToDlistMRU(Ps);
//
        }

    }else{

    }

}

//未命中加载新的数据到缓冲区的操作,函数最后加载数据的时延delay
double  CASA_AddCacheEntry(int LPN,int operation)
{
    double delay=0.0;
    return delay;
}

//缓冲区满的时候,删除脏页可能引发新的时延delay
double CASA_DelCacheEntry()
{
    double delay=0.0;
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