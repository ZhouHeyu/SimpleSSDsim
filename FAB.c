//
// Created by zj on 18-1-16.
//FAB只处理写请求,针对读请求没有命中直接交由底层的FLASH的处理
//

#include "FAB.h"
#include <stdlib.h>
#include "global.h"
#include "BlkList.h"
#include "flash.h"
#include "Interface.h"


//根据请求的LPNz找到对应的块节点的指针
//函数也可以通过Hit查看对应的LPN是否存在缓冲区中
pBlkNode FindHitBlkNode(pBlkNode pHead,int LPN,int *Hit)
{
    int tempBlk;
    int i,flag=-1;
    pBlkNode pBlk=NULL;
    tempBlk=LPN/PAGE_NUM_PER_BLK;
    pBlk=SearchBlkList(pHead,tempBlk);
    if(pBlk==NULL){
        (*Hit)=-1;
        return pBlk;
    }else{
        for (int i = 0; i <PAGE_NUM_PER_BLK ; ++i) {
            if(pBlk->list[i]==LPN){
                flag=1;
                break;
            }
        }
    }
    (*Hit)= flag == 1 ? 1 : 0;
    return  pBlk;
}


//外部调用的函数
int FAB_init(int size,int blk_num)
{
    FAB_CACHE_SIZE=0;
    FAB_BLK_NUM=0;
    FAB_MAX_CACHE_SIZE=size;
//   创建块索引链表的头部节点
    FAB_Head=CreateBlkList();

//
    return 0;
}

void FAB_end()
{
//   释放对应的头结点内存
    FreeBlkList(FAB_Head);
}

//返回有没有命中,,命中返回flag=1,没有命中返回-1
int FAB_Search(int LPN,int operation)
{
    int  flag=-1;
    pBlkNode pHitNode;
    pHitNode=FindHitBlkNode(FAB_Head,LPN,&flag);

    return flag;
}

int FAB_HitCache(int LPN,int operation,int index)
{
    int flag=-1,tempBlk;
    pBlkNode pHitNode;
    pHitNode=FindHitBlkNode(FAB_Head,LPN,&flag);
//    错误检测
    if(pHitNode==NULL){
        fprintf(stderr,"error happened in FAB_HitCache\n");
        fprintf(stderr,"pHitNode  is NULL!!\n");
        assert(0);
    }
    tempBlk=pHitNode->BlkNum;
//  将命中节点移动到队列的MRU位置
    BlkMoveToMRU(FAB_Head,pHitNode);
//   统计相应的命中情况
    buffer_hit_cnt++;
    if(operation==0){
        buffer_write_hit++;
        cache_write_num++;
    }else{
        buffer_read_hit++;
        cache_read_num++;
    }

    return 0;
}

//关于FAB是一个只针对写请求的缓冲区,因此在AddCache中针对读请求,我们直接将请求交由底层的flash处理
double FAB_AddCacheEntry(int LPN,int operation)
{
    double delay=0.0;
    int tempBlk;
    int i,free_pos;
    tempBlk=LPN/PAGE_NUM_PER_BLK;
    pBlkNode pt;

    buffer_miss_cnt++;
    if(operation!=0){
//      如果是读miss则直接交由flash处理
        buffer_read_miss++;
        delay+=callFsim(LPN*4,4,1);
        physical_read++;
        return delay;
    }

//  针对没有写请求未命中,需要区分是否存在这个块

    return delay;
}


//其实有必要在置换页的时候输入相应的请求页,便于区分两者关系
double FAB_DelCacheEntry(int LPN,int operation)
{
    double delay=0.0;
//    缓冲区未溢出没有必要执行置换算法
    if(FAB_CACHE_SIZE<FAB_MAX_CACHE_SIZE){
        return delay;
    }
//  选择最大的块进行删除



    return delay;
}


struct cache_operation FAB_Operation={
        init:   FAB_init,
        SearchCache:    FAB_Search,
        HitCache:   FAB_HitCache,
        AddCacheEntry:  FAB_AddCacheEntry,
        DelCacheEntry:  FAB_DelCacheEntry,
        end:    FAB_end
};

struct cache_operation * FAB_op_setup()
{
    return &FAB_Operation;
}