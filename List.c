//
// Created by zhouheyu on 18-1-16.
//

#include "List.h"


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


//向链表中删除节点，删除位置的节点
int DeleteEleList(pNode pHead,int pos)
{
    pNode pt=pHead,ps=NULL;
    //pos=0就是pHead
    if(pos>0&&pos<GetListLength(pHead)+1){
        while(1)
        {
            pos--;
            if(pos==0)
                break;
            pt=pt->Next;
        }
        //此时的pt是pos的前一个节点
        //ps才是要删除的节点位置
        ps=pt->Next;
        pt->Next=ps->Next;
        ps->Next->Pre=pt;
        //释放ps指向的节点
        free(ps);
        return 1;
    }else{
        printf("delete pos %d is error\n",pos);
        return 0;
    }

}

//该函数完成指定节点的指针返回,根据指定的位置返回节点的指针
pNode FindIndexNode(pNode pHead,int index)
{
    pNode ps=NULL,pt=pHead->Next;
//    index是从1开始计数的
    int count=1;
    while(count<index)
    {
        if(pt==pHead){
            fprintf(stderr,"error happened in FindIndexNode\n");
            fprintf(stderr,"index over the double-list-size\n");
            return NULL;
        }
        pt=pt->Next;
        count++;
    }
    return  pt;
}


//将命中的数据页移动到MRU位置,输入参数是LRU队列和命中数据页的指针
int MoveToMRU(pNode pHead,pNode Hit)
{
    pNode pt=NULL,ps=NULL;
    //首先切断原来位置的前后链接关系
    pt=Hit->Pre;
    pt->Next=Hit->Next;
    Hit->Next->Pre=pt;
    //将命中的数据页潜入到头部
    Hit->Pre=pHead;
    Hit->Next=pHead->Next;
    //链接关系
    pHead->Next->Pre=Hit;
    pHead->Next=Hit;

    return 0;
}


//这个函数返回的是删除页的状态（是否为脏页），关于删除的页编号通过传值参数DelLPN改变
int DeleteLRU(pNode pHead,int *DelLPN)
{
//    删除指定队列的LRU位置的
    int D_flag;
    pNode pVictim=NULL,Ps;
    pVictim=pHead->Pre;
//    错误测试
    if(pVictim==pHead || pVictim==NULL){
        fprintf(stderr,"error happened in DeleteLRU:\n");
        fprintf(stderr,"List Maybe Empty,can not Delte Node\n");
        assert(0);
    }
//  删除尾部的节点，注意再函数调用结束修改对应的长度
    * DelLPN=pVictim->LPN;
    D_flag=pVictim->isD;
//  删除节点
    Ps=pVictim->Pre;
    Ps->Next=pHead;
    pHead->Pre=Ps;
    free(pVictim);
    return D_flag;
}


//将一个全新的节点添加到队列的MRU位置
int AddNewToMRU(pNode pHead,pNode New)
{
//        需要注意的是新添加的节点的前后节点都没有关联任何位置节点
//    Insert
    New->Pre=pHead;
    New->Next=pHead->Next;
//    link
    pHead->Next->Pre=New;
    pHead->Next=New;
    return 0;
}