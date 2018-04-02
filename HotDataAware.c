//
// Created by ymb on 18/3/30.
//

#include "HotDataAware.h"
#include <stdlib.h>
#include "Interface.h"
#include "global.h"



void Delete_list(pNode CD_list, pNode Victim)
{
    int DelLPN=Victim->LPN;
    pNode temp;
    temp=Victim->Pre;
    temp->Next=Victim->Next;
    Victim->Next->Pre=temp;
//    free(Victim);
//----------------YMB-----------------------
//    Victim->Pre->Next=Victim->Next;
//    Victim->Next->Pre=Victim->Pre;
//    hash[0][DelLPN%hash_size]=0;
//    hash[1][DelLPN%hash_size]=0;
//    hash[2][DelLPN%hash_size]=0;
    hash_table[DelLPN%hash_size].visit_count=0;
    hash_table[DelLPN%hash_size].last_visit_time=0;
    hash_table[DelLPN%hash_size].buf_state=0;

}

//flag=1 hot;flag=0 cold
//int HotDateAware(int LPN, int operation)
//{
//    int flag=0;
//    if(hash_table[LPN%hash_size].visit_count!=0){   // count != 0 说明存在缓冲区
////        Hp=(hash[0][LPN%hash_size])/((double)(t_sys-hash[1][LPN%hash_size]));
//        Hp=hash_table[LPN%hash_size].visit_count/(double)(t_sys-hash_table[LPN%hash_size].last_visit_time);
//        if(Hp>H_th1) flag=1;
//        else{
//            /*--------------------H_th2 的局部性热值判断-----------------*/
//            flag=0;
//        }
//    }
//    else{
//        if(operation==0)
//            flag=1;  //未命中缓冲区的写操作为热数据
//        else
//            flag=0;
//    }
//    return flag;
//}


int HotDateAware(int LPN, int operation)
{
    int flag=0;   //冷热标识  1:hot  0:cold
    int hash_count=0;  //H_th2  中记录存在哈希表中数据个数
    int hash_state1_count=0;
    int i;
    if (hash_table[LPN%hash_size].visit_count!=0){
      // count != 0 说明存在缓冲区
        Hp=(hash_table[LPN%hash_size].visit_count)/((double)(t_sys-hash_table[LPN%hash_size].last_visit_time));
        if(Hp>H_th1) flag=1;
        else{
            /*--------------------H_th2 的局部性热值判断-----------------*/
            for( i=LPN-range_k;i<LPN+range_k;i++)
            {
                if(hash_table[i%hash_size].visit_count!=0){
                    hash_count++;
                    if(hash_table[i%hash_size].buf_state==1)
                        hash_state1_count++;
                }
            }
            if(hash_count!=0&&(double)hash_state1_count/hash_count>H_th2)
                flag=1;
            else
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
   //flag=1, 存在缓冲区;＝0不存在缓冲区
    hash_table[index].visit_count+=1;
    hash_table[index].last_visit_time=t_sys; //t_update
    hash_table[index].buf_state=flag;
    int i;
    if(flag==1)              //如果为一，表示有写入，则进行判断是否到达半衰
    {
        if(t_sys%half_time==0)  //=0 arvie half time
        {
            for( i=0;i<hash_size;i++)
            {
                //t_update与系统时间差超过half_time
                if((hash_table[i].last_visit_time-t_sys)>=half_time)
                {
                    hash_table[i].visit_count=hash_table[i].visit_count/2;
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
    int i;
    HotDataAware_Cache_Num_Entry=0;
    HotDataAware_Cache_Max_Entry=cache_size;
    hash_size=HASH_SIZE;
    //C_list=CreateList();
    //D_list=CreateList();
    HotDataAware_Tau=cache_size/2;//队列阈值
    hash_table=(struct hash_entry*)malloc(sizeof(struct hash_entry)*hash_size);
    if(hash_table==NULL){
        fprintf(stderr,"malloc for hash table is failed!");
        assert(0);
    }
    for( i=0;i<hash_size;i++){
            hash_table[i].buf_state=0;
            hash_table[i].last_visit_time=0;
            hash_table[i].visit_count=0;
    }

//   sys_operation time
    t_sys=0;
    //hash_size=cache_size*2;
    Hp=0;//热度值
    H_th1=HDA_Th1;//热度阈值
    H_th2=HDA_Th2;//局部热度阈值
    //hash[3][hash_size];// 0:存count  1:存t_update  2:存Buf_State
    range_k=Range_K;

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

//   可以尝试修改的地方
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
            HotDataAware_Tau=HotDataAware_Tau-(WRITE_DELAY/(WRITE_DELAY+READ_DELAY))*((double)CL/DL);
    }
    else{  // read
        if(CL!=0 && DL!=0)
            HotDataAware_Tau=HotDataAware_Tau+(READ_DELAY/(WRITE_DELAY+READ_DELAY))*((double)DL/CL);
    }

//   加入实时输出,查看Tau值的变化波动
    fprintf(stdout,"Tau is %lf\n",HotDataAware_Tau);

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

    buffer_miss_cnt++;
    /*-----------------启动热数据识别------------*/
    flag=HotDateAware(LPN,operation);
    /*-----------------------------------------*/
    if(operation==0){
        buffer_write_miss++;
        cache_write_num++;
        p_new->isD=1;
        if(flag==1) {
            physical_read++;
            delay+=callFsim(LPN*4,4,1);
            AddNewToMRU(CD_list, p_new);
            DL++;
            HotDataAware_Cache_Num_Entry++;
            Update_Hash(LPN, 1);
        }
        else
            physical_write++;
            delay+=callFsim(LPN*4,4,0);
            Update_Hash(LPN,0);
    }else{
        buffer_read_miss++;
        cache_read_num++;
        physical_read++;
        delay+=callFsim(LPN*4,4,1);
        p_new->isD=-1;
        if(flag==1){   //冷热数据标识1热0冷
            AddNewToMRU(CD_list,p_new);
            CL++;
            HotDataAware_Cache_Num_Entry++;
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
    if (CL + DL < HotDataAware_Cache_Max_Entry) {
        return delay;
    }

    //干净队列和脏队列的选择
    if (HotDataAware_Tau < CL)
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
    HotDataAware_Cache_Num_Entry--;
    delay += callFsim(DelLPN * 4, 4, 0);
    physical_write++;
    free(Victim);

//    test-debug
    if(HotDataAware_Cache_Num_Entry!=(CL+DL)){
        fprintf(stderr,"errro happend in CCFLRU_DelCacheEntry:");
        fprintf(stderr,"CCFLRU_Cache_Num_Entry is %d\t CL is %d\t DL is %d\n",HotDataAware_Cache_Num_Entry,CL,DL);
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
    if(hash_table!=NULL){
        free(hash_table);
    }
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