//
// Created by zhouheyu on 18-1-16.
//主要将关于双链表操作的函数集中写到这个List文件中，实现函数的复用,关于链表的节点也在这个头文件爱你中进行定义
//

#ifndef SIMULATION_LIST_H
#define SIMULATION_LIST_H
#include <stdio.h>
#include <stdlib.h>
#include "type.h"
//定义节点,双链表需要复用的节点定义
typedef struct Node
{
    int LPN;
    struct Node *Pre;
    struct Node *Next;
    int isD;
    int isCold;
}Node ,*pNode;
//头节点
//创建双向链表
pNode CreateList();
//删除整个链表，释放内存（这里有点小问题)
void FreeList(pNode *ppHead);
//判断链表是否为空
int IsEmptyList(pNode pHead);
//返回链表的长度
int GetListLength(pNode pHead);
//从链表中找到特定的LPN值，并返回节点的指针位置,如果不存在返回NULL
pNode FindLPNinList(pNode pHead,int LPN);
//该函数完成指定节点的指针返回,根据指定的位置返回节点的指针
pNode FindIndexNode(pNode pHead,int index);
//向链表中删除节点，删除位置的节点
int DeleteEleList(pNode pHead,int pos);
//将命中的节点移动到指定队列pHead的第一个位置（MRU）
int MoveToMRU(pNode pHead,pNode Hit);
//这个函数返回的是删除页的状态（是否为脏页），关于删除的页编号通过传值参数DelLPN改变
int DeleteLRU(pNode pHead,int *DelLPN);
//将一个全新的节点添加到队列的MRU位置
int AddNewToMRU(pNode pHead,pNode New);
//查看链表中的节点是否存在干净页节点,如果不存在干净页则返回NULL
pNode IsCleanNodeInList(pNode pHead);
//基于二次机会的冷探测机制,找到节点中isCold的节点,并返回该节点
pNode FindColdNodeInList(pNode pHead);
//删除链表中指定Victim的节点,函数返回的是删除节点对应的LPN号
int DelVictimNodeInList(pNode pHead,pNode Victim);
//无论热区还是冷区,选择剔除的时候都是优先置换干净页,之后基于二次机会遍历选择脏页
//函数返回的是需要剔除页的节点指针
pNode FindVictimNode_CleanFirst(pNode pHead);
#endif //SIMULATION_LIST_H
