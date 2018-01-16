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
    struct Node *Pre;
    struct Node *Next;
    int BlkSize;
    int list[PAGE_NUM_PER_BLK];
}BlkNode ,*pBlkNode;

//创建块索引的双向链表
pBlkNode CreateBlkList();
//判断链表是否为空
int IsEmptyBlkList(pBlkNode pHead);
//计算链表长度

#endif //SIMULATION_BLKLIST_H
