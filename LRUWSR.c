//
// Created by mao on 18-1-17.
//

#include "LRUWSR.h"
#include <stdlib.h>
#include "global.h"
#include "flash.h"
#include "Interface.h"


//无论热区还是冷区,选择剔除的时候都是优先置换干净页,之后基于二次机会遍历选择脏页
//函数返回的是需要剔除页的节点指针
pNode FindVictimList(pNode pHead)
{
    pNode Victim=NULL;
    //debug test
    if(IsEmptyList(pHead)!=0){
        fprintf(stderr,"error happened in FindVictimList ");
        fprintf(stderr,"the list is empty!\n");
        assert(0);
    }
    //debug test
    //先遍历寻找队列中尾部的干净页优先提出
    Victim=IsCleanNodeInList(pHead);
    if(Victim==NULL){
//        不存在干净页则选择脏页,基于二次机会找到脏页
        Victim=FindColdNodeInList(pHead);
    }

    return Victim;
}

int DelLRUList(pNode pHead)
{
    int DelLPN=-1;
    pNode pt=pHead->Pre;
    if(pt==pHead){
        printf("error happend in DelLRUList\n");
        printf("List is empty！！\n");
        exit(-1);
    }
    //将尾部衔接
    pt->Pre->Next=pHead;
    pHead->Pre=pt->Pre;
    //
    DelLPN=pt->LPN;
    if(DelLPN==-1){
        printf("error happend in DelLRUList:\n");
        printf("DelLPN == -1\n");
        exit(-1);
    }
    //释放删除点pt的内存
    free(pt);
    LRUWSR_Cache_Num_Entry--;
//    错误检测
    if (LRUWSR_Cache_Num_Entry!=GetListLength(pHead)){
        printf("error happend in DelLRUList:\n");
        printf("LRUWSR_CACHE_SIZE is %d\t list-size is %d\t",LRUWSR_Cache_Num_Entry,GetListLength(pHead));
        exit(-1);
    }

    return DelLPN;
}



//该函数完成对双链表的创建，窗口大小的设置，最大缓冲区配置
int LRUWSR_init(int size,int blk_num)
{
    LRUWSR_Cache_Max_Entry=size;
    LRUWSR_Cache_Num_Entry=0;

    //创建对应的头结点
    LRUWSR_Head=CreateList();
    //针对输入的参数blk_num不加操作
    return 0;
}




void LRUWSR_end()
{
    //释放双链表的空间
    FreeList(&LRUWSR_Head);
}



//函数返回命中LPN在链表中的位置index,如果没有命中则返回-1
int LRUWSR_Search(int LPN,int operation)
{
    int index=-1;
    pNode pt=LRUWSR_Head->Next;
    int count=0;
    //索引从1开始
    while(pt!=LRUWSR_Head){
        count++;
        if(pt->LPN==LPN){
            index=count;
            break;
        }
        pt=pt->Next;
    }

    return index;
}


//输入参数包括请求命中的LPN号，操作类型和命中的位置（在双链表中按顺序排列的位置）
int LRUWSR_HitCache(int LPN,int operation ,int index)
{
    pNode HitNode=NULL;
    HitNode=FindIndexNode(LRUWSR_Head,index);
    if(HitNode==NULL){
        fprintf(stderr,"error happened in LRUWSR_Hit:\n");
        fprintf(stderr,"HitNode is NULL!!\n");
        assert(0);
    }
//    debug
    if(HitNode->LPN!=LPN){
        fprintf(stderr,"error happened in LRUWSR_Hit:\n");
        fprintf(stderr,"HitNode->LPN is %d\t req-LPN is %d\n",HitNode->LPN,LPN);
        assert(0);
    }
//   统计命中的操作
    buffer_hit_cnt++;
    if(operation==0){
        HitNode->isD=1;
        buffer_write_hit++;
        cache_write_num++;
    }else{
        buffer_read_hit++;
        cache_read_num++;
    }
//   将命中的移动到MRU位置
    MoveToMRU(LRUWSR_Head,HitNode);
    HitNode->isCold=0;           //非冷页
    return 0;

}



double LRUWSR_AddCacheEntry(int LPN,int operation)
{
    double delay=0.0;
    pNode p_new=NULL;
    p_new=(struct Node*)malloc(sizeof(struct Node));
    if(p_new==NULL){
        printf("error happened in LRUWSR_AddCacheEntry:\n");
        printf("malloc for New node is error\n");
        assert(0);
    }
    buffer_miss_cnt++;
    //根据请求初始化对应节点的参数
    if (operation==0){               //写操作
        buffer_write_miss++;
        cache_write_num++;
        p_new->isD=1;
    } else{                          //读操作
        buffer_read_miss++;
        cache_read_num++;
        p_new->isD=0;
    }
    p_new->LPN=LPN;
    //插入头部
    AddNewToMRU(LRUWSR_Head,p_new);
//    增加相应链表的长度
    LRUWSR_Cache_Num_Entry++;
    p_new->isCold=0;              //非冷页  新增！！！！
    delay+=callFsim(LPN*4,4,1);
    physical_read++;
//错误判断
    if(LRUWSR_Cache_Num_Entry!=GetListLength(LRUWSR_Head)){
        printf("error happened in LRUWSR_AddCacheEntry:\n");
        printf("LRUWSR_Cache_Num_Entry is %d\t list-size is %d\t",LRUWSR_Cache_Num_Entry,GetListLength(LRUWSR_Head));
        exit(-1);
    }
    return delay;
}




double LRUWSR_DelCacheEntry(int ReqLPN,int ReqOperation)
{
    double delay=0.0;
    int DelLPN=-1;
    pNode pVictim = NULL;
//    如果缓冲区没有满,没有必要进行删除操作
    if(LRUWSR_Cache_Num_Entry<LRUWSR_Cache_Max_Entry){
        return delay;
    }

//   首先选择链表中的干净页进行删除.其次选择脏页
    pVictim=FindVictimList(LRUWSR_Head);
    DelLPN=pVictim->LPN;
//  如果选择的是干净页,则直接删除即可,脏页则需要回写操作
    if(pVictim->isD!=0){
        delay+=callFsim(DelLPN*4,4,0);
        physical_write++;
    }

    DelVictimNodeInList(LRUWSR_Head,pVictim);
    LRUWSR_Cache_Num_Entry--;
//    test debug
    if(LRUWSR_Cache_Num_Entry!=GetListLength(LRUWSR_Head)){
        printf("error happened in LRUWSR_DelCacheEntry:\n");
        printf("LRUWSR_Cache_Num_Entry is %d\t list-size is %d\t",LRUWSR_Cache_Num_Entry,GetListLength(LRUWSR_Head));
        exit(-1);
    }

    return delay;
}


struct cache_operation LRUWSR_Operation={
        init:   LRUWSR_init,
        SearchCache:    LRUWSR_Search,
        HitCache:   LRUWSR_HitCache,
        AddCacheEntry:  LRUWSR_AddCacheEntry,
        DelCacheEntry:  LRUWSR_DelCacheEntry,
        end:    LRUWSR_end
};

struct cache_operation * LRUWSR_op_setup()
{
    return &LRUWSR_Operation;
}