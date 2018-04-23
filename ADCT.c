//
// Created by zj on 18-1-31.
//

#include "ADCT.h"
#include "global.h"
#include "Interface.h"
#include <math.h>
#include "Cache.h"

int ADCT_find_cache_max(const int *arr, int arrMaxSize)
{
    int i;
    int temp=-1;
    int cache_max=-1;
    for ( i = 0; i <arrMaxSize ; ++i) {
        if (arr[i]!=-1){
            if (ADCTNandPage[arr[i]].cache_age>temp){
                temp=ADCTNandPage[arr[i]].cache_age;
                cache_max=arr[i];
            }
        }
    }
    return cache_max;
}

//找到数组中索引对应的LPN的age最大或最小,返回的是LPN号
int ADCT_find_cache_min(const int *arr, int arrMaxSize)
{
    int i;
    int temp=99999999;
    int cache_min=-1;
    for (i = 0; i <arrMaxSize ; ++i) {
        //attention arr[i]=-1 ,the NandPage[-1] is error
        if(arr[i]!=-1){
            if(ADCTNandPage[arr[i]].cache_age<temp){
                temp=ADCTNandPage[arr[i]].cache_age;
                cache_min=arr[i];
            }
        }
    }
    return cache_min;
}


//重置对应的NandPage的状态为初始状态的值
void ResetNandPageStat(int LPN)
{
    //先做一个错误，是否存在LPN超出NandPage的大小
    if(LPN>ADCTPageNum)
    {
        printf("error happened in ResetPageStat function\n");
        printf("the LPN:%d over the database volume\n",LPN);
        printf("the NandPage Num is %d\n",ADCTPageNum);
        assert(0);
    }
    ADCTNandPage[LPN].cache_age=0;
    ADCTNandPage[LPN].cache_status=CACHE_INVALID;
    ADCTNandPage[LPN].cache_update=0;
}

void ADCT_Stat_Reset()
{
    //重置相应的周期统计变量
    CDHit_CWH=0;
    CDHit_CRH=0;
    CDHit_DRH=0;
    CDHit_DWH=0;
    TCount=1;
    cycle_physical_write=0;
    cycle_physical_read=0;
    cycle_flash_write_delay=0.0;
    cycle_flash_read_delay=0.0;
}

//根据冷热区分的BoundAge选择部分的热脏页保留,冷脏页剔除
//参数说明，KeepCluster和VictimCluster都是在dlru-cache-arr的位置索引,BoudAge
//函数返回的是保持脏页的个数
int FindKeepCluser(int Dlist[],int DirtyNum,int *KeepCluster,int *VictimCluster,int BoundAge)
{
    int K_index,V_index,Index,L,j;
    //先计算Dlist中的有效数据项
    L=calculate_arr_positive_num(Dlist,PAGE_NUM_PER_BLK);
    //错误检测
    if(L!=DirtyNum){
        printf("error happend in FindKeepCluster:\n");
        printf("Dlist-num is %d\t DirtyNum is %d\n",L,DirtyNum);
        assert(0);
    }
    for ( Index = 0,j=0,K_index=0,V_index=0; Index <PAGE_NUM_PER_BLK&&j<L ; ++Index) {
//        记住Dlist存的是dlru_cache_arr的位置索引
        if (Dlist[Index]!=-1){
            j++;
            if(ADCTNandPage[ADCT_dlru_cache_arr[Dlist[Index]]].cache_age>=BoundAge){
                KeepCluster[K_index]=Dlist[Index];
                K_index++;
            }else{
                VictimCluster[V_index]=Dlist[Index];
                V_index++;
            }

        }
    }
    //做个错误判断
    if(K_index+V_index!=L){
        printf("error happend in FindKeepCluster\n");
        printf("KeepCluster-size:%d+VictimCluster-size:%d!=Dlist-size:%d\n",K_index,V_index,L);
        assert(0);
    }
    //返回需要保留的页数个数
    return K_index;
}


//该函数是调用底层的flash回写函数,将blk块中零散的脏页聚簇连续的回写
//输入的参数BlkNum是要聚簇删除的脏页块号,Alogrithm是否选择启用页填充算法(1:yes,0:no)
double ClusterWriteToFlash(int BlkNum,int Alogrithm)
{
    double delay=0.0,temp_delay=0.0;
    int i;
    int ReqStart,ReqSize=0;
    int offset;
    if(Alogrithm==0) {
//        不启用页填充的算法，只是聚簇回写
//        首先定位到脏页在该块中的第一个起始位置
        ReqStart = BlkNum * PAGE_NUM_PER_BLK;
        for (i = 0; i < PAGE_NUM_PER_BLK; ++i) {
            if (ADCTNandPage[ReqStart].cache_status == CACHE_INDLRU) {
                offset = ReqStart;
                ReqSize = 1;
                break;
            }
            ReqStart++;
        }
//      开始连续重构回写
        for (i = ReqStart+1; i < (BlkNum + 1) * PAGE_NUM_PER_BLK; ++i) {
            //出现中断,考虑将积累的连续请求一次回写(前提有请求,reqSize!=0)
            if (ADCTNandPage[i].cache_status != CACHE_INDLRU) {
                if (ReqSize != 0) {
                    physical_write += ReqSize;
                    cycle_physical_write += ReqSize;
//              注意代码内嵌的时候调用这里的函数
                    temp_delay=callFsim(offset*4,ReqSize*4,0);
                    cycle_flash_write_delay += temp_delay;
                    delay += temp_delay;
//              同时注意删除后重置对应的标记
                    ReqSize = 0;
                }
            }

            if (ADCTNandPage[i].cache_status == CACHE_INDLRU) {
                //说明之前刚刚聚簇回写过一次
                if (ReqSize == 0) {
                    offset = i;
                    ReqSize = 1;
                } else {
                    ReqSize++;
                }

            }
        }//end-for

        /***************debug********************/
        if(ReqSize!=0){
            physical_write+=ReqSize;
            cycle_physical_write+=ReqSize;
            temp_delay=callFsim(offset*4,ReqSize*4,0);
            cycle_flash_write_delay+=temp_delay;
            delay+=temp_delay;
            ReqSize=0;
        }

    } else{
        //启用页填充的算法
        ReqStart=BlkNum*PAGE_NUM_PER_BLK;
//        //依次读取数据页到缓冲区
//        for ( i = ReqStart; i <PAGE_NUM_PER_BLK+ReqStart ; ++i) {
//            if(ADCTNandPage[i].cache_status==CACHE_INVALID){
//                physical_read++;
//                cycle_physical_read++;
////               temp_delay=FLASH_READ_DELAY;
//                temp_delay=callFsim(i*4,4,1);
//                cycle_flash_read_delay+=temp_delay;
//                delay+=temp_delay;
//            }
//        }

//       为了避免底层的nand_read的报错,直接粗略估计读时延
        int extra_read=PAGE_NUM_PER_BLK-BlkTable[BlkNum].BlkSize;
        physical_read+=extra_read;
        cycle_physical_read+=extra_read;
        cycle_flash_read_delay+=extra_read*WRITE_DELAY;
        delay+=extra_read*WRITE_DELAY;
//        整块回写
        physical_write+=PAGE_NUM_PER_BLK;
        cycle_physical_write+=PAGE_NUM_PER_BLK;
        temp_delay=callFsim(ReqStart*4,PAGE_NUM_PER_BLK*4,0);
        cycle_flash_read_delay+=temp_delay;
        delay+=temp_delay;
    }
    return delay;
}


int MoveDLRUToCLRU(int BlkNum,const int *Keeplist,int KeepSize,const int *VictimCluster,int VictimSize)
{
    int free_pos=-1,clru_index;
    int VictimIndex;
    int tempKeep,tempLPN,tempVictim;
    int i;
//    保留部分的热数据页到clru
    for ( i = 0; i <KeepSize ; ++i) {
        tempKeep=Keeplist[i];
        tempLPN=ADCT_dlru_cache_arr[tempKeep];
        //修改对应的NandPage的状态
        ADCTNandPage[tempLPN].cache_status=CACHE_INCLRU;
        ADCTNandPage[tempLPN].cache_update=0;
//          将其加入到clru_cache中
        free_pos=find_free_pos(ADCT_clru_cache_arr,ADCT_MAX_CACHE_SIZE);
        if(free_pos==-1){
            printf("error happend in MoveDLRUToCLRU:can not find free pos for tempLPN %d\n",tempLPN);
            assert(0);
        }
//        将脏页变为干净页的LPN存入到clru-cache
        ADCT_clru_cache_arr[free_pos]=tempLPN;
        ADCT_CLRU_CACHE_SIZE++;
        clru_index=free_pos;
//        更新Clist（块索引的）
        free_pos=find_free_pos(BlkTable[BlkNum].Clist,PAGE_NUM_PER_BLK);
        if(free_pos==-1){
            printf("error happend in MoveDLRUToCLRU:can not find free pos for clru_index in Clist %d\n",clru_index);
            assert(0);
        }

        BlkTable[BlkNum].Clist[free_pos]=clru_index;
        BlkTable[BlkNum].CleanNum++;

        /****************错误检测********************/
        if(BlkTable[BlkNum].CleanNum!=calculate_arr_positive_num(BlkTable[BlkNum].Clist,PAGE_NUM_PER_BLK)){
            printf("error happend in MoveDLRUToCLRU:Clist size is error\n");
            printf("BlkTable[%d]-CleanNum is %d\t Clist-num is %d\n",BlkNum,BlkTable[BlkNum].CleanNum,calculate_arr_positive_num(BlkTable[BlkNum].Clist,PAGE_NUM_PER_BLK));
            assert(0);
        }

//      删除dlru中关于脏页的LPN
        ADCT_dlru_cache_arr[tempKeep]=-1;
        ADCT_DLRU_CACHE_SIZE--;
//      删除Dlist中的索引
        VictimIndex=search_table(BlkTable[BlkNum].Dlist,PAGE_NUM_PER_BLK,tempKeep);
        if(VictimIndex==-1){
            printf("error happened in MoveDLRUToCLRU:can not find tempKeep %d In BlkTable[%d]-Dlist",tempKeep,BlkNum);
            assert(0);
        }
        BlkTable[BlkNum].Dlist[VictimIndex]=-1;
        BlkTable[BlkNum].DirtyNum--;

        /****************错误检测********************/
        if(BlkTable[BlkNum].DirtyNum!=calculate_arr_positive_num(BlkTable[BlkNum].Dlist,PAGE_NUM_PER_BLK)){
            printf("error happend in MoveDLRUToCLRU:Dlist size is error and Save HotDirty\n");
            printf("BlkTable[%d]-DirtyNum is %d\t Dlist-num is %d\n",BlkNum,BlkTable[BlkNum].DirtyNum,calculate_arr_positive_num(BlkTable[BlkNum].Dlist,PAGE_NUM_PER_BLK));
            assert(0);
        }
//
    }

//    删除冷的数据页包含其索引
    for ( i = 0; i <VictimSize ; ++i) {
//        注意tempVictim都只是dlru-cache-arr的位置索引
        tempVictim=VictimCluster[i];
        tempLPN=ADCT_dlru_cache_arr[tempVictim];
//        重置NandPage的状态
        ResetNandPageStat(tempLPN);
//        直接删除dlru中关于LPN
        ADCT_dlru_cache_arr[tempVictim]=-1;
        ADCT_DLRU_CACHE_SIZE--;
//       删除Dlist中的索引
        VictimIndex=search_table(BlkTable[BlkNum].Dlist,PAGE_NUM_PER_BLK,tempVictim);
        if(VictimIndex==-1){
            printf("error happened in MoveDLRUToCLRU:can not find tempVictim %d In BlkTable[%d]-Dlist",tempVictim,BlkNum);
            assert(0);
        }
        BlkTable[BlkNum].Dlist[VictimIndex]=-1;
        BlkTable[BlkNum].DirtyNum--;
//      同时注意更新对应的块的当前大小
        BlkTable[BlkNum].BlkSize--;

        /****************错误检测********************/
        if(BlkTable[BlkNum].DirtyNum!=calculate_arr_positive_num(BlkTable[BlkNum].Dlist,PAGE_NUM_PER_BLK)){
            printf("error happend in MoveDLRUToCLRU:Dlist size is error and Delete cold dirty\n");
            printf("BlkTable[%d]-DirtyNum is %d\t Dlist-num is %d\n",BlkNum,BlkTable[BlkNum].DirtyNum,calculate_arr_positive_num(BlkTable[BlkNum].Dlist,PAGE_NUM_PER_BLK));
            assert(0);
        }


    }
//    最后做一个错误检测，可以删除,后续的代码调试可以加入
    if(ADCT_CLRU_CACHE_SIZE!=calculate_arr_positive_num(ADCT_clru_cache_arr,ADCT_MAX_CACHE_SIZE)||ADCT_DLRU_CACHE_SIZE!=calculate_arr_positive_num(ADCT_dlru_cache_arr,ADCT_MAX_CACHE_SIZE)){
        printf("error happend in MoveDLRUToCLRU:\n");
        printf("clru-num is %d\t CACHE_CLRU_NUM_ENTRIES %d\n",calculate_arr_positive_num(ADCT_clru_cache_arr,ADCT_MAX_CACHE_SIZE),ADCT_CLRU_CACHE_SIZE);
        printf("dlru-num is %d\t CACHE_DLRU_NUM_ENTRIES %d\n",calculate_arr_positive_num(ADCT_dlru_cache_arr,ADCT_MAX_CACHE_SIZE),ADCT_DLRU_CACHE_SIZE);
        assert(0);
    }


    return 0;
}

//定义自适应的读写缓冲区阈值调整函数,函数返回的是更新后的Tau值
int ADCT_UpdateTau(int lastTau)
{
    int Tau,D_Tau,TempTau,MinTau,MaxTau;//表示脏队列的目标长度
    //四射五入
    double B_CLRU,B_DLRU;//表示各自队列的单位收益
    MinTau=(int)(MinTauRatio*ADCT_MAX_CACHE_SIZE+0.5);
    MaxTau=(int)(MaxTauRatio*ADCT_MAX_CACHE_SIZE+0.5);

    D_Tau=ADCT_MAX_CACHE_SIZE-lastTau;

    //如果从底层得到周期的读写时延
    double ave_flash_read_delay,ave_flash_write_delay;
    if(cycle_physical_read==0){
        ave_flash_read_delay=0.0;
    }else{
        ave_flash_read_delay=cycle_flash_read_delay/cycle_physical_read;
    }
    if(cycle_physical_write==0){
        ave_flash_write_delay=0.0;
    }else{
        ave_flash_write_delay=cycle_flash_write_delay/cycle_physical_write;
    }

    B_CLRU=(CDHit_CRH*ave_flash_read_delay+CDHit_CWH*ave_flash_write_delay)/lastTau;
    B_DLRU=(CDHit_DRH*ave_flash_read_delay+CDHit_DWH*ave_flash_write_delay)/D_Tau;

    //四舍五入
    TempTau=(int)((B_CLRU/(B_CLRU+B_DLRU)*ADCT_MAX_CACHE_SIZE)+0.5);
    TempTau=max(TempTau,MinTau);
    TempTau=min(TempTau,MaxTau);
    Tau=TempTau;
//    printf("Temp is %d\n",TempTau);

    //  更新对应的统计回写(flash)
    int temp_write;
    ADCT_BW=cycle_physical_write;
    ADCT_FW=flash_write_num-last_flash_write;
    last_flash_write=flash_write_num;
    //重置相应的周期统计变量
    ADCT_Stat_Reset();

    return Tau;
}

void HitCLRU(int LPN,int operation)
{
    int HitIndex,tempBlkNum;
    int victim=-1;
    int free_pos=-1,NewIndex;
    int i,CL,DL;
    //首先更新对应的NandPage的age状态,之后别忘了更新其他的状态
    ADCTNandPage[LPN].cache_age=ADCTNandPage[cache_max_index].cache_age+1;
    cache_max_index=LPN;
    //从CLRU的数组中找到存放LPN的位置
    HitIndex=search_table(ADCT_clru_cache_arr,ADCT_MAX_CACHE_SIZE,LPN);
    //错误判读
    if(HitIndex==-1){
        printf("error happend in HitCLRU :can not Find LPN %d in ADCT_clru_cache_arr\n",LPN);
        assert(0);
    }

    //根据命中的类型（写需要移动到DLRU队列中去，更复杂）
    if(operation!=0){
        //读命中,不需要移动数据项
        CDHit_CRH++;
        buffer_read_hit++;
        cache_read_num++;
    }else{
        //写命中，需要移动数据项
        CDHit_CWH++;
        buffer_write_hit++;
        cache_write_num++;
        //删除clru_cache中的数据
        ADCT_clru_cache_arr[HitIndex]=-1;
        ADCT_CLRU_CACHE_SIZE--;
        //将其存入DLRU队列
        free_pos=find_free_pos(ADCT_dlru_cache_arr,ADCT_MAX_CACHE_SIZE);
        if(free_pos==-1){
            printf("error happen in HitCLRU, can not find free_pos in dlr-cache-arr\n");
            exit(-1);
        }
        ADCT_dlru_cache_arr[free_pos]=LPN;
        ADCT_DLRU_CACHE_SIZE++;
        //更新NandPage的标志位
        ADCTNandPage[LPN].cache_status=CACHE_INDLRU;
        ADCTNandPage[LPN].cache_update=1;

        //更新块索引的数据
        tempBlkNum=LPN/PAGE_NUM_PER_BLK;
        //删除对应clist上的索引(HitIndex)，将新的dlru位置索引(free_pos)加入dlist
        victim=search_table(BlkTable[tempBlkNum].Clist,PAGE_NUM_PER_BLK,HitIndex);
        //错误检测
        if(victim==-1){
            printf("error happend in HitInCLRU can not find HitIndex:%d In BLkTable[%d]-Clist\n",HitIndex,tempBlkNum);
            CL=calculate_arr_positive_num(BlkTable[tempBlkNum].Clist,PAGE_NUM_PER_BLK);
            printf("the Clist Num is %d\t CleanNum is %d\n",CL,BlkTable[tempBlkNum].CleanNum);
            //依次打印输出当前的Clist的队列的值
            for(i=0;i<PAGE_NUM_PER_BLK;i++){
                if(BlkTable[tempBlkNum].Clist[i]!=-1){
                    printf("%d\t",BlkTable[tempBlkNum].Clist[i]);
                }
            }
            assert(0);
        }
        BlkTable[tempBlkNum].Clist[victim]=-1;
        BlkTable[tempBlkNum].CleanNum--;

        NewIndex=free_pos;//NewIndex是之前dlru_cache_arr的位置索引
        free_pos=find_free_pos(BlkTable[tempBlkNum].Dlist,PAGE_NUM_PER_BLK);
        //错误检测
        if(free_pos==-1){
            printf("error happend in HitCLRU can not find free_pos in BLKTable[%d]-Dlist for NewIndex %d\n",tempBlkNum,NewIndex);
            DL=calculate_arr_positive_num(BlkTable[tempBlkNum].Dlist,PAGE_NUM_PER_BLK);
            printf("the Dlist Num is %d\t DirtyNum is %d\n",DL,BlkTable[tempBlkNum].DirtyNum);
            //依次打印输出当前的Dlist的队列的值
            for(i=0;i<PAGE_NUM_PER_BLK;i++){
                if(BlkTable[tempBlkNum].Dlist[i]!=-1){
                    printf("%d\n",BlkTable[tempBlkNum].Dlist[i]);
                }
            }
            exit(-1);
        }
        BlkTable[tempBlkNum].Dlist[free_pos]=NewIndex;
        BlkTable[tempBlkNum].DirtyNum++;
        //错误检测需要的值
         CL=calculate_arr_positive_num(BlkTable[tempBlkNum].Clist,PAGE_NUM_PER_BLK);
         DL=calculate_arr_positive_num(BlkTable[tempBlkNum].Dlist,PAGE_NUM_PER_BLK);
//        做一个错误检测判断CL和DL是否一直一致
        if(CL!=BlkTable[tempBlkNum].CleanNum||DL!=BlkTable[tempBlkNum].DirtyNum){
            printf("error happend in HitCLRU,Clist or Dlist size is error\n");
            printf("the Dlist Num is %d\t DirtyNum is %d\n",DL,BlkTable[tempBlkNum].DirtyNum);
            printf("the Clist Num is %d\t CleanNum is %d\n",CL,BlkTable[tempBlkNum].CleanNum);
            exit(-1);
        }
    }

}


//命中DLRU的操作
void HitDLRU(int LPN,int operation)
{
    //更新对应的NandPage的状态标识
    ADCTNandPage[LPN].cache_age=ADCTNandPage[cache_max_index].cache_age+1;
    cache_max_index=LPN;
    if(operation==0){
        CDHit_DWH++;
        cache_write_num++;
        buffer_write_hit++;
    }else{
        CDHit_DRH++;
        cache_read_num++;
        buffer_read_hit++;
    }
}




//删除CLRU的lru位置的数据项
void DelLPNInCLRU()
{
    int MinAgeLPN,Victim;
    //BlkTable删除需要的中间变量
    int ClistVictim,ClistIndex;
    int tempBlkNum;

    //注意这里返回的是LPN号,不是位置索引
    MinAgeLPN= ADCT_find_cache_min(ADCT_clru_cache_arr, ADCT_MAX_CACHE_SIZE);
    //重置NandPage相关的状态
    ResetNandPageStat(MinAgeLPN);
    Victim=search_table(ADCT_clru_cache_arr,ADCT_MAX_CACHE_SIZE,MinAgeLPN);
    //错误检测
    if(Victim==-1){
        printf("error happend in DelLPNInCLRU: can not find MinAgeLPN %d In clru-cache-arr\n",MinAgeLPN);
        assert(0);
    }
    //删除clru中的数据
    ADCT_clru_cache_arr[Victim]=-1;
    ADCT_CLRU_CACHE_SIZE--;
    //删除对应的LPN的块索引
    tempBlkNum=MinAgeLPN/PAGE_NUM_PER_BLK;
//    遍历找到对应删除的MinAgeLPN(LPN)-->Victim(ClistVictim)在Clist上的位置(ClistIndex)
    ClistVictim=Victim;
    ClistIndex=search_table(BlkTable[tempBlkNum].Clist,PAGE_NUM_PER_BLK,ClistVictim);
    //错误检测
    if(ClistIndex==-1){
        printf("error happend in DelLPNInCLRU: can not find ClistVictim %d In BlkTable[%d]-Clist\n",ClistVictim,tempBlkNum);
        assert(0);
    }
    //删除对应的数据项
    BlkTable[tempBlkNum].Clist[ClistIndex]=-1;
    //对应的统计量的修改
    BlkTable[tempBlkNum].BlkSize--;
    BlkTable[tempBlkNum].CleanNum--;
    //错误检测
    if(BlkTable[tempBlkNum].CleanNum+BlkTable[tempBlkNum].DirtyNum!=BlkTable[tempBlkNum].BlkSize){
        printf("error happend in DelLPNinCLRU: BlkTableSize is error\n");
        printf("BlkTable[%d]-CleanNum is %d\t DirtyNum is %d\t BlkSize is %d\n",tempBlkNum,BlkTable[tempBlkNum].CleanNum,BlkTable[tempBlkNum].DirtyNum,BlkTable[tempBlkNum].BlkSize);
        printf("Clist-num is %d\t Dlist num is %d\n",calculate_arr_positive_num(BlkTable[tempBlkNum].Clist,PAGE_NUM_PER_BLK),calculate_arr_positive_num(BlkTable[tempBlkNum].Dlist,PAGE_NUM_PER_BLK));
        assert(0);
    }
    //错误检测
    if(BlkTable[tempBlkNum].CleanNum!=calculate_arr_positive_num(BlkTable[tempBlkNum].Clist,PAGE_NUM_PER_BLK)){
        printf("error happend in DelLPNinCLRU:Clist size is error\n");
        printf("BlkTable[%d]-CleanNum is %d\t Clist-num is %d\n",tempBlkNum,BlkTable[tempBlkNum].CleanNum,calculate_arr_positive_num(BlkTable[tempBlkNum].Clist,PAGE_NUM_PER_BLK));
        assert(0);
    }
}




//删除DLRU中的数据项，聚簇回写，涉及底层的物理读写，返回操作时延
double DelLPNInDLRU()
{
    double delay=0.0,temp_delay=0.0;
    int i,j,k,InsertFLag;
    int MinAgeIndex=-1,MinAge=2147483647;
    //要删除的LPN(尾部的LPN)
    int LRUVictim,tempBlkNum;
    //聚簇要删除的冷页和要保留的热脏页(dlru_cache_arr的位置索引)
    int VictimCluster[PAGE_NUM_PER_BLK],KeepCluser[PAGE_NUM_PER_BLK];
    //要剔除的脏页总数,保留的页数（转移到CLRU的个数）
    int DelSize,KeepSize,VictimSize;
    int Bound,BoundAgeIndex,BoundAge,BoundAgeLPN;
    //Bound是当前dlru队列的冷热界限，[1,2,3,4,5],如果1,2,3位置为热，3就是Bound
    //之后将DLRU中的LPN根据其age的大小，降序排列，DescendIndex是其在dlru-cache-arr中的位置索引，不是LPN
    //同理BoundAgeIndex也是dlru-cache-arr的位置索引，BoundAge是其age值
    int DescendIndex[ADCT_DLRU_CACHE_SIZE];
    //初始化，-1的索引是无效值(ADCT_dlru_cache_arr[-1]是不存在的)
    for ( i = 0; i <ADCT_DLRU_CACHE_SIZE ; ++i) {
        DescendIndex[i]=-1;
    }
    //初始化VictimCluster和KeepCluser
    for ( i = 0; i <PAGE_NUM_PER_BLK ; ++i) {
        VictimCluster[i]=-1;
        KeepCluser[i]=-1;
    }

    /*************变量初始化结束************************/
    //错误判断
    if(ADCT_DLRU_CACHE_SIZE<=0){
        printf("error happend in DelLPNInDLRU:CACHE_DLRU_NUM_ENTRIES==0\n");
        exit(1);
    }
    if(ADCT_DLRU_CACHE_SIZE!=calculate_arr_positive_num(ADCT_dlru_cache_arr,ADCT_MAX_CACHE_SIZE))
    {
        printf("error happend in DelLPNInDLRU:dlru size is error\n");
        printf("CACHE_DLRU_NUM_ENTRIES is %d\t dlru num is %d\n",ADCT_DLRU_CACHE_SIZE,calculate_arr_positive_num(ADCT_dlru_cache_arr,ADCT_MAX_CACHE_SIZE));
        exit(1);
    }
    /*****************找到DLRU中age降序排列的前a%的分界的BoudAgeIndex**************************/
    for ( i = 0,j=0; i <ADCT_MAX_CACHE_SIZE&&j<ADCT_DLRU_CACHE_SIZE ; ++i) {
        if(ADCT_dlru_cache_arr[i]>=0){
            if(ADCTNandPage[ADCT_dlru_cache_arr[i]].cache_age<MinAge){
                MinAge=ADCTNandPage[ADCT_dlru_cache_arr[i]].cache_age;
                //MinAgeIndex就是要删除的LPN在dlru_cache_arr的位置
                MinAgeIndex=i;
            }
            //同时读入有效的LPN到DescendIndex数组中，按照降序排列
            InsertFLag=0;
            for ( k = 0; k <j ; ++k) {
                if(ADCTNandPage[ADCT_dlru_cache_arr[DescendIndex[k]]].cache_age<ADCTNandPage[ADCT_dlru_cache_arr[i]].cache_age){
                    //如果出现插入的新索引的age比该位置的大，则返回这个插入的位置，同时，将这之后的数据都向后移动一位
                    InsertArr(DescendIndex,ADCT_DLRU_CACHE_SIZE,i,k);
                    InsertFLag=1;
                    break;
                }
            }
            //如果上述的Age都比i的大,插入尾部(j),注意j是当前的空闲的位置
            if(InsertFLag==0){
                DescendIndex[j]=i;
            }
            //错误检测
            if(DescendIndex[j]==-1){
                printf("error happend in DelLPNInDLRU：order DesenedIndex\n");
                printf("Insert Index to DescendIndex exist error:\n");
                assert(0);
            }
            j++;
        }
    }
    //经过上述的排序，可以得到对应的DescendIndex的降序排序（age）
    Bound=(int)(ADCT_HotTh*ADCT_DLRU_CACHE_SIZE+0.5);//四舍五入
    BoundAgeIndex=DescendIndex[Bound];
    BoundAgeLPN=ADCT_dlru_cache_arr[BoundAgeIndex];
    BoundAge=ADCTNandPage[BoundAgeLPN].cache_age;
    /**********定位到尾部（LRU）的LPN,和对应的块索引***************/
    LRUVictim=ADCT_dlru_cache_arr[MinAgeIndex];
    tempBlkNum=LRUVictim/PAGE_NUM_PER_BLK;
    KeepSize=FindKeepCluser(BlkTable[tempBlkNum].Dlist,BlkTable[tempBlkNum].DirtyNum,KeepCluser,VictimCluster,BoundAge);

//    debug
    if(MinAgeIndex==-1){
        fprintf(stderr,"当前最小操作时间%d\n",DescendIndex[ADCT_DLRU_CACHE_SIZE-1]);
        fprintf(stderr,"当前初始时MinAge　%d\n",MinAge);
        exit(0);
    }



    if(calculate_arr_positive_num(VictimCluster,PAGE_NUM_PER_BLK)==0){
        printf("error!!!");
    }
//    debug

    DelSize=BlkTable[tempBlkNum].DirtyNum;
    VictimSize=DelSize-KeepSize;

//    嵌入自适应聚簇回写缓冲区算法代码:
    int E=PAGE_NUM_PER_BLK-BlkTable[tempBlkNum].DirtyNum;
    double F=1.0;
    if(ADCT_BW!=0){
         F=(double)ADCT_FW/ADCT_BW;
    }else{
        if(ADCT_FW!=0){F=(double)ADCT_FW/1;}
    }
    double Th=(double)PAGE_NUM_PER_BLK/(1+pow(ADCT_t,2-F));

//    printf("Th is %lf\n",Th);

    if(E<=(int)Th){
        delay=ClusterWriteToFlash(tempBlkNum,1);
    }else{
        delay=ClusterWriteToFlash(tempBlkNum,0);
    }

//    删除全部的脏页进行回写
//    delay=ClusterWriteToFlash(tempBlkNum,1);



    /****************计算回写的时间延迟***0:不启用页填充*********************************/
//    delay=ClusterWriteToFlash(tempBlkNum,0);
    //注意更改NandPage的状态（回写后的）
    //更改对应的块索引
    MoveDLRUToCLRU(tempBlkNum,KeepCluser,KeepSize,VictimCluster,VictimSize);




    return delay;
}



/*************************************************************/

int ADCT_init(int size, int DataBlk_Num)
{
    int i,j;
    ADCT_MAX_CACHE_SIZE=size;

    ADCT_CLRU_CACHE_SIZE=0;
    ADCT_clru_cache_arr=(int *)malloc(sizeof(int)*size);
    if(ADCT_clru_cache_arr==NULL){
        fprintf(stderr,"malloc for ADCT_clru_cache_arr is failed!\n");
        assert(0);
    }
//  快速初始化
    memset(ADCT_clru_cache_arr,0xFF, sizeof(int)*size);

    ADCT_DLRU_CACHE_SIZE=0;
    ADCT_dlru_cache_arr=(int *)malloc(sizeof(int)*size);
    if(ADCT_dlru_cache_arr==NULL){
        fprintf(stderr,"malloc for ADCT_dlru_cache_arr is failed!\n");
        assert(0);
    }
    memset(ADCT_dlru_cache_arr,0xFF, sizeof(int)*size);

//   初始化对应的页状态信息
    ADCTPageNum=DataBlk_Num*PAGE_NUM_PER_BLK;
    ADCTNandPage=(struct CachePageEntry *)malloc(sizeof(struct CachePageEntry)*ADCTPageNum);
    if(ADCTNandPage==NULL){
        printf("the create ADCT Nandpage Memeory is failed\n");
        assert(0);
    }
// 初始化内存
    for ( i = 0; i <ADCTPageNum ; ++i) {
        ADCTNandPage[i].cache_status=0;
        ADCTNandPage[i].cache_update=0;
        ADCTNandPage[i].cache_age=0;
    }

    BlkTableNum=DataBlk_Num;
    BlkTable=( struct BlkTable_entry *)malloc(sizeof(struct  BlkTable_entry)*BlkTableNum);
    if(BlkTable==NULL){
        printf("the create ADCT Blktable is failed\n");
        assert(0);
    }
//  初始化函数
    for ( i = 0; i <BlkTableNum; ++i) {
        BlkTable[i].BlkSize=0;
        BlkTable[i].CleanNum=0;
        BlkTable[i].DirtyNum=0;
        for ( j = 0; j <PAGE_NUM_PER_BLK ; ++j) {
            BlkTable[i].Clist[j]=-1;
            BlkTable[i].Dlist[j]=-1;
        }
    }
//  设置最小的界限
    MinTauRatio=0.1;
    MaxTauRatio=0.9;
    ADCT_Tau=size/2;
//  写入放大系数
    ADCT_FW=0;
    ADCT_BW=0;
    last_flash_write=flash_write_num;
    ADCT_t=2;
    return 0;
}

void  ADCT_end()
{
    if(ADCT_dlru_cache_arr==NULL) {
        free(ADCT_dlru_cache_arr);
    }
    if(ADCT_clru_cache_arr==NULL){
        free(ADCT_clru_cache_arr);
    }
    if(BlkTable){
        free(BlkTable);
    }
    if(ADCTNandPage==NULL){
        free(ADCTNandPage);
    }
}


//返回的是type=-1表示为命中，0表示命中读缓冲区，1表示命中写缓冲区
int ADCT_Search(int LPN,int operation)
{
    int type;
//   首先换算对应的块是不是在
    if(ADCTNandPage[LPN].cache_status==CACHE_INCLRU){
        type=0;
    }else if(ADCTNandPage[LPN].cache_status==CACHE_INDLRU){
        type=1;
    }else{
        type=-1;
    }

//    插入对阈值Tau的调整
    //最后缓冲区操作完成开始检测是否需要更新Tau的值
    if(TCount==ADCTUpdateCycle){
        ADCT_Tau=ADCT_UpdateTau(ADCT_Tau);
    }else {
        TCount++;
    }

    return type;
}



//Hit_index表示命中的队列类型
int ADCT_HitCache (int LPN,int operation,int Hit_kindex)
{
//  统计命中次数
    buffer_hit_cnt++;
    if(Hit_kindex==0){
//      命中CLRU的操作
        HitCLRU(LPN,operation);
    }else if(Hit_kindex==1){
        HitDLRU(LPN,operation);
    }else{
        fprintf(stderr,"ADCT Hit Cache error !!\n");
        assert(0);
    }
  return 0;
}



double ADCT_AddCacheEntry(int LPN,int operation)
{
    double delay=0.0;
    int free_pos=-1,dlru_index,clru_index;
    int tempBlkNum;

    buffer_miss_cnt++;
    physical_read++;
    cycle_physical_read++;
    if (operation==0){
        buffer_write_miss++;
        cache_write_num++;
//        找到dlru中的空闲位置
        free_pos=find_free_pos(ADCT_dlru_cache_arr,ADCT_MAX_CACHE_SIZE);
        if (free_pos==-1){
            printf("error happend in AddNewToBuffer: can not find free pos for LPN %d in dlru-cache-arr\n",LPN);
            assert(0);
        }
        ADCT_dlru_cache_arr[free_pos]=LPN;
        dlru_index=free_pos;
        ADCT_DLRU_CACHE_SIZE++;
//        更新相应的NandPage的状态
        ADCTNandPage[LPN].cache_update=1;
        ADCTNandPage[LPN].cache_status=CACHE_INDLRU;
        ADCTNandPage[LPN].cache_age=ADCTNandPage[cache_max_index].cache_age+1;
        cache_max_index=LPN;
//        计算读写的时延
        delay=callFsim(LPN*4,4,1);
//        delay=FLASH_READ_DELAY;
        cycle_flash_read_delay+=delay;
//        更新对应的块索引
        tempBlkNum=LPN/PAGE_NUM_PER_BLK;
        BlkTable[tempBlkNum].BlkSize++;
        BlkTable[tempBlkNum].DirtyNum++;
        free_pos=find_free_pos(BlkTable[tempBlkNum].Dlist,PAGE_NUM_PER_BLK);
        if (free_pos==-1){
            printf("error happend in AddNewToBuffer: can not find free pos for dlru_index %d in BlkTable[%d]-Dlist\n",dlru_index,tempBlkNum);
            assert(0);
        }
        BlkTable[tempBlkNum].Dlist[free_pos]=dlru_index;
        /****************错误检测********************/
        if(BlkTable[tempBlkNum].DirtyNum!=calculate_arr_positive_num(BlkTable[tempBlkNum].Dlist,PAGE_NUM_PER_BLK)){
            printf("error happend in  AddNewToBuffer:Dlist size is error\n");
            printf("BlkTable[%d]-DirtyNum is %d\t Dlist-num is %d\n",tempBlkNum,BlkTable[tempBlkNum].DirtyNum,calculate_arr_positive_num(BlkTable[tempBlkNum].Dlist,PAGE_NUM_PER_BLK));
            assert(0);
        }

    }else{
        buffer_read_miss++;
        cache_read_num++;
//       找到clru中空闲的位置
        free_pos=find_free_pos(ADCT_clru_cache_arr,ADCT_MAX_CACHE_SIZE);
        if (free_pos==-1){
            printf("error happend in AddNewToBuffer: can not find free pos for LPN %d in clru-cache-arr\n",LPN);
            assert(0);
        }
        ADCT_clru_cache_arr[free_pos]=LPN;
        clru_index=free_pos;
        ADCT_CLRU_CACHE_SIZE++;
//        更新相应的NandPage的状态
        ADCTNandPage[LPN].cache_update=0;
        ADCTNandPage[LPN].cache_status=CACHE_INCLRU;
        ADCTNandPage[LPN].cache_age=ADCTNandPage[cache_max_index].cache_age+1;
        cache_max_index=LPN;
//        计算读写延迟
        delay=callFsim(LPN*4,4,1);
        cycle_flash_read_delay+=delay;
//       更新对应的块索引
        tempBlkNum=LPN/PAGE_NUM_PER_BLK;
        BlkTable[tempBlkNum].BlkSize++;
        BlkTable[tempBlkNum].CleanNum++;
        free_pos=find_free_pos(BlkTable[tempBlkNum].Clist,PAGE_NUM_PER_BLK);
        if (free_pos==-1){
            printf("error happend in AddNewToBuffer: can not find free pos for clru_index %d in BlkTable[%d]-Clist\n",clru_index,tempBlkNum);
            assert(0);
        }
        BlkTable[tempBlkNum].Clist[free_pos] = clru_index;
        /****************错误检测********************/
        if(BlkTable[tempBlkNum].CleanNum!=calculate_arr_positive_num(BlkTable[tempBlkNum].Clist,PAGE_NUM_PER_BLK)){
            printf("error happend in AddNewToBuffer:Clist size is error\n");
            printf("BlkTable[%d]-CleanNum is %d\t Clist-num is %d\n",tempBlkNum,BlkTable[tempBlkNum].CleanNum,calculate_arr_positive_num(BlkTable[tempBlkNum].Clist,PAGE_NUM_PER_BLK));
            assert(0);
        }

    }

    return delay;
}


//判断当前缓冲区是否溢出
double ADCT_DelCacheEntry(int ReqLPN,int ReqOperation)
{
    double delay=0.0;
    if(ADCT_CLRU_CACHE_SIZE+ADCT_DLRU_CACHE_SIZE<ADCT_MAX_CACHE_SIZE){
        return delay;
    }
//    debug
    if(ADCT_CLRU_CACHE_SIZE+ADCT_DLRU_CACHE_SIZE>ADCT_MAX_CACHE_SIZE){
        printf("error\n");
        exit(0);
    }
//    debug

//  debug test
    if(ADCT_CLRU_CACHE_SIZE!=calculate_arr_positive_num(ADCT_clru_cache_arr,ADCT_MAX_CACHE_SIZE)||ADCT_DLRU_CACHE_SIZE!=calculate_arr_positive_num(ADCT_dlru_cache_arr,ADCT_MAX_CACHE_SIZE)){
        printf("ADCT_CLRU_CACHE_SIZE is %d\t clru-arr num is %d\n",ADCT_CLRU_CACHE_SIZE,calculate_arr_positive_num(ADCT_clru_cache_arr,ADCT_MAX_CACHE_SIZE));
        printf("ADCT_DLRU_CACHE_SIZE is %d\t dlru-arr num is %d\n",ADCT_DLRU_CACHE_SIZE,calculate_arr_positive_num(ADCT_dlru_cache_arr,ADCT_MAX_CACHE_SIZE));
        assert(0);
    }
    //根据Tau选择删除CLRU还是DLRU
    if(ADCT_CLRU_CACHE_SIZE>=ADCT_Tau){
//        选择删除CLRU,不涉及读写延迟
        DelLPNInCLRU();
    }else{
//        涉及到回写操作，会有flash_delay的延迟
        delay+=DelLPNInDLRU();
    }


    return delay;
}

struct cache_operation ADCT_Operation={
        init:   ADCT_init,
        SearchCache:    ADCT_Search,
        HitCache:   ADCT_HitCache,
        AddCacheEntry:  ADCT_AddCacheEntry,
        DelCacheEntry:  ADCT_DelCacheEntry,
        end:    ADCT_end
};

struct cache_operation * ADCT_op_setup()
{
    return &ADCT_Operation;
}
