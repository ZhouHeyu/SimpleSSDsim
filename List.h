//
// Created by zhouheyu on 18-1-16.
//主要将关于双链表操作的函数集中写到这个List文件中，实现函数的复用,关于链表的节点也在这个头文件爱你中进行定义
//

#ifndef SIMULATION_LIST_H
#define SIMULATION_LIST_H

//定义节点,双链表需要复用的节点定义
typedef struct Node
{
    int LPN;
    struct Node *Pre;
    struct Node *Next;
    int isD;
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
int MoveTOLRU(pNode pHead,pNode Hit);

#endif //SIMULATION_LIST_H
