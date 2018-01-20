//
// Created by zj on 18-1-16.
//

#include "BlkList.h"
#include <stdlib.h>

pBlkNode CreateBlkList()
{
    int i;
    //分配初始内存
    pBlkNode  pHead=(pBlkNode)malloc(sizeof(BlkNode));
    if(NULL==pHead){
        printf("malloc for pBlkNode Head failed!\n");
        exit(-1);
    }
    pHead->BlkNum=-1;
    pHead->Pre=pHead;
    pHead->Next=pHead;
    pHead->BlkSize=-1;
    for ( i = 0; i <PAGE_NUM_PER_BLK ; ++i) {
        pHead->list[i]=-1;
    }

    return pHead;
}


//删除整个链表，释放内存
void FreeBlkList(pBlkNode pHead)
{
    pBlkNode pt=pHead->Next,ps;
    while (pt!=pHead)
    {
        ps=pt;
        pt=pt->Next;
        free(ps);
    }
    free(pHead);

}

//判断链表是否为空
int IsEmptyBlkList(pBlkNode pHead)
{
    pBlkNode pt=pHead->Next;
    if(pt==pHead)
    {
        return 1;
    }else
    {
        return 0;
    }

}

//计算链表长度
int GetBlkListLength(pBlkNode pHead)
{
    int length=0;
    pBlkNode pt=pHead->Next;
    while (pt !=pHead)
    {
        length++;
        pt=pt->Next;
    }
    return length;
}

//遍历块节点链表,访问的块号是否存在链表中
pBlkNode  SearchBlkList(pBlkNode pHead,int BlkNum)
{
    pBlkNode pt=NULL;
    pBlkNode ps=pHead->Next;
    while(ps!=pHead)
    {
        if(ps->BlkNum==BlkNum)
        {
            pt=ps;
            break;
        }
        ps=ps->Next;
    }
    return pt;
}

//将命中的数据块移动队列的头部
int BlkMoveToMRU(pBlkNode pHead,pBlkNode pHit)
{
    pBlkNode ps;
    ps=pHit->Pre;
    //删除原来位置中的链接关系
    ps->Next=pHit->Next;
    pHit->Next->Pre=ps;
    //插入到对应的位置
    ps=pHead->Next;
    pHit->Next=ps;
    pHit->Pre=pHead;
    pHead->Next=pHit;
    ps->Pre=pHit;
    //错误检测
    return 0;
}

int BlkMoveToLRU(pBlkNode pHead,pBlkNode pHit)
{
    pBlkNode ps;
    ps=pHit->Pre;
    //删除原来位置中的链接关系
    ps->Next=pHit->Next;
    pHit->Next->Pre=ps;
    //插入到对应的位置,嵌入到MRU位置
    pHit->Pre=pHead->Pre;
    pHit->Next=pHead;
    pHead->Pre->Next=pHit;
    pHead->Pre=pHit;
    return 0;
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


int BlkAddNewToMRU(pBlkNode pHead,pBlkNode p_new)
{
    p_new->Pre=pHead;
    p_new->Next=pHead->Next;
    pHead->Next->Pre=p_new;
    pHead->Next=p_new;
    return 0;
}

//返回以块节点组织的cache大小-->当前的缓冲区的大小
int BlkGetCacheSize(pBlkNode pHead)
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


//删除块链表中指定的节点,放回删除节点的包含的页的个数
int BlkDeleteNode(pBlkNode pHead,pBlkNode Victim)
{
    int DelSize=-1;
    DelSize=Victim->BlkSize;
    int flag=0;
    pBlkNode  ps=pHead->Next;
//   debug
    while(ps!=pHead){
        if(ps==Victim) {
            flag=1;
            break;
        }
        ps=ps->Next;
    }
    if(flag==0){
        fprintf(stderr,"blk-list not clude Victim Node!\n");
        assert(0);
    }
//    debug
    ps=Victim->Next;
//     new link
    ps->Pre=Victim->Pre;
    Victim->Pre->Next=ps;
//    释放节点内存
    free(Victim);

    return DelSize;
}
