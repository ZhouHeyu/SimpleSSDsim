//
// Created by zj on 18-1-20.
//

#include "CCFLRU.h"
#include <stdlib.h>
#include "Interface.h"

//找到队列中LRU中的数据页isD为1的节点
pNode FindColdVictim(pNode pHead)
{
    pNode Victim=NULL,ps=NULL;
    Victim=pHead->Pre;
    int count=0,L;
    L=GetListLength(pHead);
    while(Victim->isCold!=1){
        Victim->isCold=1;
        ps=Victim;
        Victim=ps->Pre;
//        new Victim link
        Victim->Next=pHead;
        pHead->Pre=Victim;
//        move old Vitcim to MRU
        ps->Pre=pHead;
        ps->Next=pHead->Next;
        pHead->Next->Pre=ps;
        pHead->Next=ps;
        count++;
        if(count>L+2){
            fprintf(stderr,"error happend in FindColdVictim: loop count over limit!\n");
            assert(0);
        }
    }
//    test debug
    if(Victim!=pHead->Pre){
        fprintf(stderr,"error happend in FindColdVictim: Victim is not LRU page!\n");
        assert(0);
    }
    return Victim;
}

int CCFLRU_init(int cache_size,int blk_num)
{
    CCL_Head=CreateList();
    CCL_Size=0;
    ML_Head=CreateList();
    ML_Size=0;
    CCFLRU_Cache_Max_Entry=cache_size;
    CCFLRU_Cache_Num_Entry=0;
    return 0;
}

void CCFLRU_end()
{
    FreeList(CCL_Head);
    FreeList(ML_Head);
}

//函数返回的-1表示未命中,0表示命中CCL队列,1表示命中ML队列
int CCFLRU_Search(int LPN,int operation)
{
    int type=-1;
    pNode pt=NULL;
    pt=FindLPNinList(CCL_Head,LPN);
    if(pt==NULL){
        pt=FindLPNinList(ML_Head,LPN);
        if(pt!=NULL){
            type=1;
        }
    }else{
        type=0;
    }
    return type;
}

//当数据页变脏或者二次命中都会移动到ML队列的MRU位置
int CCFLRU_HitCache(int LPN,int operation,int HitType)
{
    pNode pHit=NULL;
    if(HitType==0){
        pHit=FindLPNinList(CCL_Head,LPN);
//       命中CCL后面有一个移动到ML的操作
        CCL_Size--;
        ML_Size++;
    }else{
        pHit=FindLPNinList(ML_Head,LPN);
    }
    buffer_hit_cnt++;
    pHit->isCold=0;
    if(operation==0){
        pHit->isD=1;
        buffer_write_hit++;
        cache_write_num++;
    }else {
        buffer_read_hit++;
        cache_read_num++;
    }
    MoveToMRU(ML_Head,pHit);
    return 0;
}


//第一次访问的数据页都标识为冷页,干净页被加载到CCL,脏页被加载到ML
double CCFLRU_AddCacheEntry(int LPN ,int operation)
{
    double delay=0.0;
    pNode p_new=NULL;
    p_new=(pNode)malloc(sizeof(Node));
    if(p_new==NULL){
        fprintf(stderr,"error happened in CCFLRU_AddCacheEntry:\n");
        fprintf(stderr,"malloc for new node is failded\n");
        assert(0);
    }
    p_new->LPN=LPN;
    p_new->isCold=1;
    physical_read++;
    delay+=callFsim(LPN*4,4,1);
    buffer_miss_cnt++;
    if(operation==0){
        buffer_write_miss++;
        cache_write_num++;
        p_new->isD=1;
        AddNewToMRU(ML_Head,p_new);
        ML_Size++;
        CCFLRU_Cache_Num_Entry++;

    }else{
        buffer_read_miss++;
        cache_read_num++;
        p_new->isD=0;
        AddNewToMRU(CCL_Head,p_new);
        CCL_Size++;
        CCFLRU_Cache_Num_Entry++;
    }
//    test debug;
    if(CCL_Size!=GetListLength(CCL_Head)||ML_Size!=GetListLength(ML_Head)){
        fprintf(stderr,"error happened in CCFLRU_AddCacheEntry:\n");
        fprintf(stderr,"CCL_Size is %d\t ML_Size is %d\n",CCL_Size,ML_Size);
        fprintf(stderr,"CCL-list size is %d\t ML-list size is %d\n",GetListLength(CCL_Head),GetListLength(ML_Head));
        assert(0);
    }

    return delay;
}


//优先选择CCL队列中的数据页删除,当CCL中没有数据页的时候选择ML中的数据页删除
//ML中的剔除操作略微复杂:
//hot-drity ---> cold dirty and MRU
//hot-clean --> cold clean to CCL LRU find ML Victim continue
//cold-dirty   --> write to flash and delete
double CCFLRU_DelCacheEntry(int ReqLPN,int ReqOperation)
{
    double delay=0.0;
    pNode Victim=NULL;
    int DelLPN=-1;
//     not full don't need replace
    if(CCL_Size+ML_Size<CCFLRU_Cache_Max_Entry){
        return delay;
    }
//  CCL is empty?
    if(CCL_Size>0){
//      del CCL-list LRU
        DeleteLRU(CCL_Head,&DelLPN);
        CCL_Size--;
        CCFLRU_Cache_Num_Entry--;
//       DelNode is Clean Node dont need write back;
    }else{

        do{
            Victim=FindColdVictim(ML_Head);
//          Victim may be clean page --> Move to CCL MRU
            if(Victim->isD==0){
                MoveToMRU(CCL_Head,Victim);
                CCL_Size++;
                ML_Size--;
            }
        }while(Victim->isD!=1);
//        Del Mix list LRU Node
        DeleteLRU(ML_Head,&DelLPN);
        ML_Size--;
        CCFLRU_Cache_Num_Entry--;
        delay+=callFsim(DelLPN*4,4,0);
        physical_write++;
    }

//    test-debug
    if(CCFLRU_Cache_Num_Entry!=CCL_Size+ML_Size){
        fprintf(stderr,"errro happend in CCFLRU_DelCacheEntry:");
        fprintf(stderr,"CCFLRU_Cache_Num_Entry is %d\t CCL_Size is %d\t ML_Size is %d\n",CCFLRU_Cache_Num_Entry,CCL_Size,ML_Size);
        assert(0);
    }
    if(CCL_Size!=GetListLength(CCL_Head)||ML_Size!=GetListLength(ML_Head)){
        fprintf(stderr,"errro happend in CCFLRU_DelCacheEntry:");
        fprintf(stderr,"CCL_Size is %d\t ML_Size is %d\n",CCL_Size,ML_Size);
        fprintf(stderr,"CCL-list size is %d\t ML-list size is %d\n",GetListLength(CCL_Head),GetListLength(ML_Head));
        assert(0);
    }

    return delay;
}


struct cache_operation CCFLRU_Operation={
        init:   CCFLRU_init,
        SearchCache:    CCFLRU_Search,
        HitCache:   CCFLRU_HitCache,
        AddCacheEntry:  CCFLRU_AddCacheEntry,
        DelCacheEntry:  CCFLRU_DelCacheEntry,
        end:    CCFLRU_end
};

struct cache_operation * CCFLRU_op_setup()
{
    return &CCFLRU_Operation;
}