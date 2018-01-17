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


//返回当前的缓冲区的大小
int FABGetCacheSize(pBlkNode pHead)
{
    int length=0;
    pBlkNode  pt=pHead->Next;
    while (pt !=pHead)
    {
        length+=pt->BlkSize;
        pt=pt->Next;
    }
    return length;
}

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
        for ( i = 0; i <PAGE_NUM_PER_BLK ; ++i) {
            if(pBlk->list[i]==LPN){
                flag=1;
                break;
            }
        }
    }
    (*Hit)= flag == 1 ? 1 : 0;
    return  pBlk;
}

//找到块节点链表中的块最大的块节点,第二参数是禁止删除的块节点的标号
pBlkNode FindMaxSizeBlk(pBlkNode pHead, int ReqBlkNum)
{
    pBlkNode pMax=NULL,pt=pHead->Next;
    int MaxSize=0;
    while(pt!=pHead){
//        还需要满足该块不是请求的块
        if(pt->BlkSize>MaxSize && pt->BlkNum!=ReqBlkNum){
            MaxSize=pt->BlkSize;
            pMax=pt;
        }
        pt=pt->Next;
    }
//    返回满足要求的最大块
    return pMax;
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
    pBlkNode pBlk,ps;

    buffer_miss_cnt++;
    if(operation!=0){
//      如果是读miss则直接交由flash处理
        buffer_read_miss++;
        delay+=callFsim(LPN*4,4,1);
        physical_read++;
        return delay;
    }

//  针对没有写请求未命中,需要区分是否存在这个块
    pBlk=SearchBlkList(FAB_Head,tempBlk);
//    如果之前不存在该块
    if(pBlk==NULL){
        pBlk=(pBlkNode)malloc(sizeof(BlkNode));
        if(pBlk==NULL){
            fprintf(stderr,"error happend in FAB_AddCacheEntry:\n");
            fprintf(stderr,"malloc for New Blk failed\n");
            assert(0);
        }
        pBlk->BlkNum=tempBlk;
        pBlk->BlkSize=1;
        for ( i = 0; i <PAGE_NUM_PER_BLK ; ++i) {
            pBlk->list[i]=-1;
        }
        physical_read++;
        delay=callFsim(LPN*4,4,1);
        pBlk->list[0]=LPN;
        //将新的块插入到队列的头部
        ps=FAB_Head->Next;
        pBlk->Next=ps;
        pBlk->Pre=FAB_Head;
        ps->Pre=pBlk;
        FAB_Head->Next=ps;
        FAB_CACHE_SIZE++;
        FAB_BLK_NUM++;
//       错误检测
        if(FAB_BLK_NUM!=GetBlkListLength(FAB_Head)){
            fprintf(stderr,"error happend in FAB_AddCacheEntry:\n");
            fprintf(stderr," FAB_BLK_NUM is %d\t,CacheHead-list size is %d\n",FAB_BLK_NUM,GetBlkListLength(FAB_Head));
            assert(0);
        }
    }else{
        //错误检测
        if(pBlk->BlkNum!=tempBlk){
            fprintf(stderr,"error happend in FAB_AddCacheEntry:\n");
            fprintf(stderr,"pblk-BlkNum is %d\t tempBlk is %d\n",pBlk->BlkNum,tempBlk);
            assert(0);
        }
//          将命中的块移动到头部
        BlkMoveToMRU(FAB_Head,pBlk);
        free_pos=find_free_pos(pBlk->list,PAGE_NUM_PER_BLK);
        //test
        if(free_pos==-1){
            fprintf(stderr,"error happend in FAB_AddCacheEntry:\n");
            fprintf(stderr,"can not find free pos for LPN %d in pblk-list\n",LPN);
            assert(0);
        }
        physical_read++;
        pBlk->list[free_pos]=LPN;
        pBlk->BlkSize++;
        delay+=callFsim(LPN*4,4,1);
        FAB_CACHE_SIZE++;
        //错误检测
        if(FAB_BLK_NUM!=GetBlkListLength(FAB_Head)){
            printf("error happend in AddNewToBuffer  1-2-2\n");
            printf("BlkNodeSize is %d\t,CacheHead-list size is %d\n",FAB_BLK_NUM,GetBlkListLength(FAB_Head));
            exit(-1);
        }
    }

    return delay;
}


//其实有必要在置换页的时候输入相应的请求页,便于区分两者关系
double FAB_DelCacheEntry(int ReqLPN,int ReqOperation)
{
    double delay=0.0;
    int i,j,k;
//    受保护的节点是当前请求块所属的节点
    int ReqBlkNum=-1,DelSize,DelBlkNum;
//    需要删除的节点(最大块节点)
    pBlkNode DelBlkNode=NULL,ps;

//    聚簇回写使用到的变量
    int StartLPN,DelArr[PAGE_NUM_PER_BLK],InsertFlag=0;
    int LPNSize=1;

//    缓冲区未溢出没有必要执行置换算法或者是读请求
    if(FAB_CACHE_SIZE<FAB_MAX_CACHE_SIZE || ReqOperation!=0){
        return delay;
    }
//  选择最大的块进行删除,但是不删除当前请求所属的节点
    ReqBlkNum=ReqLPN/PAGE_NUM_PER_BLK;
    DelBlkNode=FindMaxSizeBlk(FAB_Head,ReqBlkNum);
//   错误检测
    if(DelBlkNode==NULL){
        fprintf(stderr,"error happended in FAB_DelCacheEntry!\n");
        fprintf(stderr,"DelBlkNode is NULL\n");
        assert(0);
    }
    DelSize=DelBlkNode->BlkSize;
    DelBlkNum=DelBlkNode->BlkNum;
    physical_write+=DelSize;

//    聚簇回写
    StartLPN=(DelBlkNum+1)*PAGE_NUM_PER_BLK;
    for ( i = 0,j=0; i <PAGE_NUM_PER_BLK&&j<DelSize ; ++i) {
        if(DelBlkNode->list[i]>=0){
            if(DelBlkNode->list[i]<StartLPN){
                StartLPN=DelBlkNode->list[i];
            }
//           同时读入回写的地址,按照升序排列
            InsertFlag=0;
            for ( k = 0; k <j ; ++k) {
                if(DelBlkNode->list[i]<DelArr[k]){
                    InsertArr(DelArr,PAGE_NUM_PER_BLK,DelBlkNode->list[i],k);
                    InsertFlag=1;
                    break;
                }
            }
            if(InsertFlag==0){
                DelArr[j]=DelBlkNode->list[i];
            }
            j++;
        }
    }
    //错误判断
    if(DelSize!=calculate_arr_positive_num(DelArr,PAGE_NUM_PER_BLK)){
        fprintf(stderr,"error happend in FAB_DelCacheEntry\n");
        fprintf(stderr,"DelSize is %d\t,DelArr-size is %d\n",DelSize,calculate_arr_positive_num(DelArr,PAGE_NUM_PER_BLK));
        assert(0);
    }

    //遍历依次回写
    for ( i = 1; i <DelSize ; ++i) {
        if(DelArr[i]-DelArr[i-1]!=1){
            //聚簇回写
            //delay+=LPNSize*FLASH_WRITE_DELAY;
            delay+=callFsim(StartLPN*4,4*LPNSize,0);
            StartLPN=DelArr[i];
            LPNSize=1;
        }else{
            LPNSize++;
        }
    }
    //delete histroy-leave
    if(LPNSize>0){
        //delay+=LPNSize*FLASH_WRITE_DELAY;
        delay+=callFsim(StartLPN*4,4*LPNSize,0);
        LPNSize=0;
    }

//    删除该节点
    ps=DelBlkNode->Pre;
    ps->Next=DelBlkNode->Next;
    DelBlkNode->Next->Pre=ps;
    free(DelBlkNode);
    FAB_BLK_NUM--;
    FAB_CACHE_SIZE=FAB_CACHE_SIZE-DelSize;

//    错误检测
    if(FAB_BLK_NUM!=GetBlkListLength(FAB_Head)){
        fprintf(stderr,"error happend in FAB_DelCacheEntry\n");
        fprintf(stderr,"FAB_BLK_NUM is %d\t FAB_BLK_LIST size is %d\n",FAB_BLK_NUM,GetBlkListLength(FAB_Head));
        assert(0);
    }
    if(FAB_CACHE_SIZE!=FABGetCacheSize(FAB_Head)){
        fprintf(stderr,"error happend in FAB_DelCacheEntry\n");
        fprintf(stderr,"FAB_CACHE_SIZE is %d\t cache-list-size is %d\n",FAB_CACHE_SIZE,FABGetCacheSize(FAB_Head));
        assert(0);
    }

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