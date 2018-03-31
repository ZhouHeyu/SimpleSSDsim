//
// Created by ymb on 18/3/30.
//

#include "HotDateAware.h"
#include <stdlib.h>
#include "Interface.h"
#include "global.h"



void Delete_list(pNode CD_list, pNode Victim)
{
    int DelLPN=Victim->LPN;
    Victim->Pre->Next=Victim->Next;
    Victim->Next->Pre=Victim->Pre;
    hash[0][DelLPN%hash_size]=0;
    hash[1][DelLPN%hash_size]=0;
    hash[2][DelLPN%hash_size]=0;
}


int HotDateAware(int LPN, int operation)
{
    int flag=0;
    if(hash[0][LPN%hash_size]!=0){   // count != 0 说明存在缓冲区
        Hp=(hash[0][LPN%hash_size])/((double)(t_sys-hash[1][LPN%hash_size]));
        if(Hp>H_th1) flag=1;
        else{
            /*--------------------H_th2 的局部性热值判断-----------------*/
            flag=0;
        }
    }
    else{
        if(operation==0)
            flag=1;  //未命中缓冲区的写操作为热数据
        else
            flag=0;
    }
    return flag;
}

void Update_Hash(int LPN,int flag)
{
    int index=LPN%hash_size;
    hash[0][index]+=1;
    hash[1][index]=t_sys;     // t_update
    hash[2][index]=flag;     //flag=1, 存在缓冲区;＝0不存在缓冲区
    if(flag==1)              //如果为一，表示有写入，则进行判断是否到达半衰
    {
        if(t_sys%half_time==0)  //=0 arvie half time
        {
            for(int i=0;i<hash_size;i++)
            {
                if((hash[1][i]-t_sys)>=half_time) //t_update与系统时间差超过half_time
                {
                    hash[0][i]=hash[0][i]/2;
                }
            }
        }
    }
}


int HotDateAware_init(int cache_size,int blk_num)
{
    CD_list=CreateList();
    CL=0;
    DL=0;
    HotDateAware_Cache_Num_Entry=0;
    HotDateAware_Cache_Max_Entry=cache_size;
    //C_list=CreateList();
    //D_list=CreateList();
    Tau=cache_size/2;//队列阈值
    t_sys=0;
    //hash_size=cache_size*2;
    Hp=0;//热度值
    H_th1=0.5;//热度阈值
    H_th2=0.5;//局部热度阈值
    //hash[3][hash_size];// 0:存count  1:存t_update  2:存Buf_State
    half_time=1000;
    return 0;
}

int HotDateAware_Search(int LPN,int operation)
{
    int type=-1;
    pNode pt=NULL;
    pt=FindLPNinList(CD_list,LPN);
    if(pt==NULL){
        type=-1;
    }else{
        type=1;
    }
    if (operation==0)  // write
    {
        t_sys++;
    }
    return type;
}


int HotDateAware_HitCache(int LPN,int operation,int HitIndex)
{
    if(HitIndex==-1){
        fprintf(stderr,"error happened in HotDateAware_HitCache, because of HitIndex\n");
        fprintf(stderr,"HitIndex is -1!!\n");
        assert(0);
    }
    pNode pHit=NULL;
    int CL=GetCListLength(CD_list);
    int DL=GetDListLength(CD_list);
    buffer_hit_cnt++;
    if (operation==0)//写命中
    {
        buffer_write_hit++;
        cache_write_num++;
        pHit=FindLPNinList(CD_list,LPN);
        if (pHit != NULL){    //如果CD_list找到，就将数据提到MRU
            pHit->isD==1;     // 设为脏页
            MoveToMRU(CD_list,pHit);
        }
        else{                               //
            fprintf(stderr,"error happened in HotDateAware_HitCache. Beacuse can't find LPN in CD_list\n");
            assert(0);
        }
    }
    else{   // read
        buffer_read_hit++;
        cache_read_num++;
        pHit=FindLPNinList(CD_list,LPN);
        if(pHit!=NULL){   // hit
            MoveToMRU(CD_list,pHit);
        }
        else{  // not hit in CD_list, that means error
            fprintf(stderr,"error happened in HotDateAware_HitCache. Beacuse can't find LPN in CD_list\n");
            assert(0);
        }
    }
    // Tau更新
    if (operation==0){  //write
        if(CL!=0 && DL!=0)
            Tau=Tau-(WRITE_DELAY/(WRITE_DELAY+READ_DELAY))*((double)CL/DL);
    }
    else{  // read
        if(CL!=0 && DL!=0)
            Tau=Tau+(READ_DELAY/(WRITE_DELAY+READ_DELAY))*((double)DL/CL);
    }
    Update_Hash(LPN,1);
    return 0;
}

double HotDateAware_AddCacheEntry(int LPN ,int operation)
{
    int flag=0;//热数据识别表示 1:热 0:冷
    double delay=0.0;
    pNode p_new=NULL;
    p_new=(pNode)malloc(sizeof(Node));
    if(p_new==NULL){
        fprintf(stderr,"error happened in CCFLRU_AddCacheEntry:\n");
        fprintf(stderr,"malloc for new node is failded\n");
        assert(0);
    }
    p_new->LPN=LPN;
    //p_new->isD=1;
    physical_read++;
    delay+=callFsim(LPN*4,4,1);
    buffer_miss_cnt++;
    /*-----------------启动热数据识别------------*/
    flag=HotDateAware(LPN,operation);
    /*-----------------------------------------*/
    if(operation==0){
        buffer_write_miss++;
        cache_write_num++;
        p_new->isD=1;
        if(flag==1) {
            AddNewToMRU(CD_list, p_new);
            DL++;
            HotDateAware_Cache_Num_Entry++;
            Update_Hash(LPN, 1);
        }
        else
            Update_Hash(LPN,0);
    }else{
        buffer_read_miss++;
        cache_read_num++;
        p_new->isD=-1;
        if(flag==1){   //冷热数据标识1热0冷
            AddNewToMRU(CD_list,p_new);
            CL++;
            HotDateAware_Cache_Num_Entry++;
            Update_Hash(LPN,1);
        }
        else Update_Hash(LPN,0);
    }
//    test debug;
    if(CL!=GetCListLength(CD_list)||DL!=GetDListLength(CD_list)){
        fprintf(stderr,"error happened in CCFLRU_AddCacheEntry:\n");
        fprintf(stderr,"CL is %d\t DL is %d\n",CL,DL);
        fprintf(stderr,"CL-list size is %d\t DL-list size is %d\n",GetCListLength(CD_list),GetDListLength(CD_list));
        assert(0);
    }
    return delay;
}


double HotDateAware_DelCacheEntry(int ReqLPN,int ReqOperation) {
    int Tau_flag = 0;  //0:干净页剔除  1:脏页剔除
    int DelLPN=0;
    double delay = 0.0;
    pNode Victim = NULL;
//     not full don't need replace
    if (CL + DL < HotDateAware_Cache_Max_Entry) {
        return delay;
    }

    //干净队列和脏队列的选择
    if (Tau < CL)
        Tau_flag = 0;  //?????????????????????
    else
        Tau_flag = 1;
    Victim = CD_list->Pre;
    while (1) {
//      Victim may be clean page --> Move to CCL MRU
        if (Tau_flag == 1) {  //Dirty
            if (Victim->isD == 1) {
                Delete_list(CD_list,Victim);
                DL--;
                break;
            }
        }
        else {
            if (Victim->isD == -1) {
                Delete_list(CD_list, Victim);
                CL--;
                break;
            }
        }
        Victim=Victim->Pre;
    }
//        Del Mix list LRU Node
    DelLPN=Victim->LPN;
    HotDateAware_Cache_Num_Entry--;
    delay += callFsim(DelLPN * 4, 4, 0);
    physical_write++;
    free(Victim);

//    test-debug
    if(HotDateAware_Cache_Num_Entry!=(CL+DL)){
        fprintf(stderr,"errro happend in CCFLRU_DelCacheEntry:");
        fprintf(stderr,"CCFLRU_Cache_Num_Entry is %d\t CL is %d\t DL is %d\n",HotDateAware_Cache_Num_Entry,CL,DL);
        assert(0);
    }
    if(CL!=GetCListLength(CD_list)||DL!=GetDListLength(CD_list)){
        fprintf(stderr,"errro happend in HotDateAware_DelCacheEntry:");
        fprintf(stderr,"CL_Size is %d\t DL_size is %d\n",CL,DL);
        fprintf(stderr,"CL-list size is %d\t DL-list size is %d\n",GetCListLength(CD_list),GetDListLength(CD_list));
        assert(0);
    }

    return delay;
}
void HotDateAware_end()
{
    FreeList(CD_list);
}
struct cache_operation HotDateAware_Operation={
        init:   HotDateAware_init,
        SearchCache:    HotDateAware_Search,
        HitCache:   HotDateAware_HitCache,
        AddCacheEntry:  HotDateAware_AddCacheEntry,
        DelCacheEntry:  HotDateAware_DelCacheEntry,
        end:    HotDateAware_end
};

struct cache_operation * HotDateAware_op_setup()
{
    return &HotDateAware_Operation;
}