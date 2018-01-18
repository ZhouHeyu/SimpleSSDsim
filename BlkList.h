//
// Created by zj on 18-1-16.
//这个链表主要实现块节点的链表操作
//

#ifndef SIMULATION_BLKLIST_H
#define SIMULATION_BLKLIST_H
#include "type.h"
#include "flash.h"


//定义节点,节点是块的节点
typedef struct BlkNode
{
    int BlkNum;
    struct BlkNode *Pre;
    struct BlkNode *Next;
    int BlkSize;
    int list[PAGE_NUM_PER_BLK];
}BlkNode ,*pBlkNode;

//创建块索引的双向链表
pBlkNode CreateBlkList();
//删除整个链表，释放内存
void FreeBlkList(pBlkNode pHead);
//判断链表是否为空
int IsEmptyBlkList(pBlkNode pHead);
//计算链表长度
int GetBlkListLength(pBlkNode pHead);
//遍历链表，寻找链表中对应的数据，若存在则返回该节点的指针
pBlkNode  SearchBlkList(pBlkNode pHead,int BlkNum);
//将命中的数据块移动队列的头部
int BlkMoveToMRU(pBlkNode pHead,pBlkNode pHit);
//根据请求的LPNz找到对应的块节点的指针
//函数也可以通过Hit查看对应的LPN是否存在缓冲区中
pBlkNode FindHitBlkNode(pBlkNode pHead,int LPN,int *Hit);
#endif //SIMULATION_BLKLIST_H
