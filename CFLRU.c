//
// Created by zj on 18-1-14.
//

#include "CFLRU.h"
#include <stdlib.h>
#include "global.h"
#include "flash.h"
#include "Interface.h"


//该函数主要查找队列尾部的优先置换区w中是否存在干净页
//输入参数是优先置换区的大小，输出参数是干净页在队列中的位置，不存在返回NULL
pNode IsCLeanInWindow(pNode pHead,int w)
{
    int index;
    pNode pt, ps=NULL;
    //错误判断
    if(CFLRU_Cache_Num_Entry!=CFLRU_Cache_Max_Entry){
        printf("error happend in IsCLeanInWindow:\n");
        printf("CFLRU_CACHE_SZIE is %d\t buf_size is %d\n",CFLRU_Cache_Num_Entry,CFLRU_Cache_Max_Entry);
        assert(0);
    }
    //首先指向队列的尾部（LRU）
    pt=pHead->Pre;
    for ( index = 0; index <w ; index++) {
        if (pt->isD==0){
            ps=pt;
            break;
        }
        pt=pt->Pre;
    }
    //    错误检测
    if (CFLRU_Cache_Num_Entry!=GetListLength(pHead)){
        printf("error happend IsCLeanInWindow :\n");
        printf("CFLRU_Cache_Num_Entry is %d\t list-size is %d\t",CFLRU_Cache_Num_Entry,GetListLength(pHead));
        exit(-1);
    }

    return ps;
}



//删除是制定的干净页,返回删除的LPN
int DelCleanLPN(pNode pHead,pNode CVictim)
{
    pNode pt=NULL;
    int DelLPN=-1;
    //切断联系
    pt=CVictim->Pre;
    pt->Next=CVictim->Next;
    CVictim->Next->Pre=pt;
    DelLPN=CVictim->LPN;
    //释放对应的节点,释放CVictim节点的内存
    free(CVictim);
    CFLRU_Cache_Num_Entry--;
    //错误判读
    if(CFLRU_Cache_Num_Entry!=GetListLength(pHead)){
        printf("error happened in DelCleanLPN:\n");
        printf("CFLRU_Cache_Num_Entry is %d\t list-size is %d\t",CFLRU_Cache_Num_Entry,GetListLength(pHead));
        exit(-1);
    }
    return DelLPN;
}

//删除链表的尾部的数据，返回的是删除节点包含的LPN号
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
    CFLRU_Cache_Num_Entry--;
//    错误检测
    if (CFLRU_Cache_Num_Entry!=GetListLength(pHead)){
        printf("error happend in DelLRUList:\n");
        printf("CFLRU_CACHE_SIZE is %d\t list-size is %d\t",CFLRU_Cache_Num_Entry,GetListLength(pHead));
        exit(-1);
    }

    return DelLPN;
}


/***********外部接口函数**************************/


//该函数完成对双链表的创建，窗口大小的设置，最大缓冲区配置
int CFLRU_init(int size,int blk_num)
{
    CFLRU_Cache_Max_Entry=size;
    CFLRU_Cache_Num_Entry=0;
    //如果配置文件中没有输入对应的比例大小，则设置alpha为1/2;在主函数里面设置
    Window_Size=(int)(CFLRU_Cache_Max_Entry*CFLRU_alpha+0.5);
    //创建对应的头结点
    CFLRU_Head=CreateList();
    //针对输入的参数blk_num不加操作
    return 0;
}


void CFLRU_end()
{
    //释放双链表的空间
    FreeList(&CFLRU_Head);
}

//函数返回命中LPN在链表中的位置index,如果没有命中则返回-1
int CFLRU_Search(int LPN,int operation)
{
    int index=-1;
    pNode pt=CFLRU_Head->Next;
    int count=0;
    //索引从1开始
    while(pt!=CFLRU_Head){
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
int CFLRU_HitCache(int LPN,int operation ,int index)
{
    pNode HitNode=NULL;
    HitNode=FindIndexNode(CFLRU_Head,index);
    if(HitNode==NULL){
        fprintf(stderr,"error happened in CFLRU_Hit:\n");
        fprintf(stderr,"HitNode is NULL!!\n");
        assert(0);
    }
//    debug
    if(HitNode->LPN!=LPN){
        fprintf(stderr,"error happened in CFLRU_Hit:\n");
        fprintf(stderr,"HitNode->LPN is %d\t req-LPN is %d\n",HitNode->LPN,LPN);
        assert(0);
    }
//   统计命中的操作
    buffer_hit_cnt++;
    if(operation==0){
        HitNode->isD=0;
        buffer_write_hit++;
        cache_write_num++;
    }else{
        buffer_read_hit++;
        cache_read_num++;
    }
//   将命中的移动到MRU位置
    MoveToMRU(CFLRU_Head,HitNode);

    return 0;

}

double CFLRU_AddCacheEntry(int LPN,int operation)
{
    double delay=0.0;
    pNode p_new=NULL;
    p_new=(struct Node*)malloc(sizeof(struct Node));
    if(p_new==NULL){
        printf("error happened in CFLRU_AddCacheEntry:\n");
        printf("malloc for New node is error\n");
        assert(0);
    }
    buffer_miss_cnt++;
    //根据请求初始化对应节点的参数
    if (operation==0){
        buffer_write_miss++;
        cache_write_num++;
        p_new->isD=1;
    } else{
        buffer_read_miss++;
        cache_read_num++;
        p_new->isD=0;
    }
    p_new->LPN=LPN;
    //插入头部
    p_new->Next=CFLRU_Head->Next;
    p_new->Pre=CFLRU_Head;
    //链接
    CFLRU_Head->Next->Pre=p_new;
    CFLRU_Head->Next=p_new;
//    增加相应链表的长度
    CFLRU_Cache_Num_Entry++;
    delay+=callFsim(LPN*4,4,1);
    physical_read++;
//错误判断
    if(CFLRU_Cache_Num_Entry!=GetListLength(CFLRU_Head)){
        printf("error happened in CFLRU_AddCacheEntry:\n");
        printf("CFLRU_Cache_Num_Entry is %d\t list-size is %d\t",CFLRU_Cache_Num_Entry,GetListLength(CFLRU_Head));
        exit(-1);
    }
    return delay;
}

double CFLRU_DelCacheEntry()
{
    double delay=0.0;
    int DelLPN=-1;
    pNode pC=NULL;
//    如果缓冲区没有满,没有必要进行删除操作
    if(CFLRU_Cache_Num_Entry<CFLRU_Cache_Max_Entry){
        return delay;
    }

//    首先查看w窗口内是否存在想要的干净页替换
    pC=IsCLeanInWindow(CFLRU_Head,Window_Size);
    if(pC!=NULL){
        //表示存在可以置换的干净页,不需要回写操作
        DelLPN=DelCleanLPN(CFLRU_Head,pC);
        //错误判断
        if(DelLPN==-1){
            printf("error happend in CFLRU_DelCacheEntry:\n");
            printf("DelClean LPN is -1!!!\n");
//            exit(-1);
            assert(0);
        }
    }else{
        //表示不存在可以置换的干净页，选择尾部的脏页进行回写
        DelLPN=DelLRUList(CFLRU_Head);
//        之后调用callFsim函数
        physical_write++;
        delay+=callFsim(DelLPN*4,4,0);
//        delay+=FLASH_WRITE_DELAY;
    }
    return delay;


}

struct cache_operation CFLRU_Operation={
        init:   CFLRU_init,
        SearchCache:    CFLRU_Search,
        HitCache:   CFLRU_HitCache,
        AddCacheEntry:  CFLRU_AddCacheEntry,
        DelCacheEntry:  CFLRU_DelCacheEntry,
        end:    CFLRU_end
};

struct cache_operation * CFLRU_op_setup()
{
    return &CFLRU_Operation;
}