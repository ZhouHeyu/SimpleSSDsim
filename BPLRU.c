//
// Created by zj on 18-1-18.
//需要注意的是BPLRU只是针对写请求处理的,读请求只是查找缓冲区是否存在对应的LPN不存在直接读取flash
//

#include "BPLRU.h"
#include "Interface.h"

//用于页补偿,记录上一次的请求的块号
int LastBlkNum;
//记录相同块的累计
int SameBlkHit;
//判断为顺序写的阈值
int Seq_Threshold;
//判定顺序写模式的标识符,块补偿机制通过这个这个标识符进行启动
int Seq_flag;

int BPLRU_init(int cache_size,int blk_num)
{
    BPLRU_BLK_NUM=0;
    BPLRU_Head=CreateBlkList();
    BPLRU_MAX_CACHE_SIZE=cache_size;
    BPLRU_CACHE_SIZE=0;
    LastBlkNum=-1;
    SameBlkHit=0;
    Seq_flag=0;
//    认为顺序写的判断的阈值
    Seq_Threshold=PAGE_NUM_PER_BLK/2;

    return 0;
}

void BPLRU_end()
{
    FreeBlkList(BPLRU_Head);
}


//函数返回的Hit_flag表示的命中情况,同时处理了全局变量Seq_flag,用来判断当前的请求是否未连续写模式
int BPLRU_Search(int LPN,int operation)
{
    int Hit_Flag=-1;
    int tempBlk;
    tempBlk=LPN/PAGE_NUM_PER_BLK;
//  针对第一次请求的处理
    if(LastBlkNum==-1){
        LastBlkNum=tempBlk;
    }

//    辨别是否未顺序写模式
    if(LastBlkNum==tempBlk){
        SameBlkHit++;
        if(SameBlkHit>=Seq_Threshold){
//         如果连续多次命中同一个块,则判断为顺序写,将标识符置位1
            Seq_flag=1;
        } else{
//           如果非连续命中模式,则判断未非顺序写模式,标识符置位0
            Seq_flag=0;
        }
    }else{
//        非连续写请求
        SameBlkHit=1;
        LastBlkNum=tempBlk;
        Seq_flag=0;
    }


    FindHitBlkNode(BPLRU_Head,LPN,&Hit_Flag);

    return Hit_Flag;
}


//  在BPLRU算法中,只针对写请求处理,读请求命中不做任何的处理
int BPLRU_HitCache(int LPN,int operation,int type)
{
    buffer_hit_cnt++;
    pBlkNode pHit=NULL;
    int flag;
    if(operation!=0){
        buffer_read_hit++;
        cache_read_num++;
        return 0;
    }else{
        buffer_write_hit++;
        cache_write_num++;
    }

//   计算命中的块
    pHit=FindHitBlkNode(BPLRU_Head,LPN,&flag);

//  根据当前的写入模式进行不同的操作
//    顺序写模式
    if(Seq_flag==1){
//      启动块补偿机制,将命中的块移动到LRU位置
        BlkMoveToLRU(BPLRU_Head,pHit);
    }else{
//     非顺序写模式,将命中的块移动到MRU位置
        BlkMoveToMRU(BPLRU_Head,pHit);
    }

    return 0;
}


//  在BPLRU算法中,只针对写请求处理,读请求命中不做任何的处理
double BPLRU_AddCacheEntry(int LPN,int operation)
{
    double delay=0.0;
    pBlkNode pHit=NULL;
    int flag,i,free_pos;

    buffer_miss_cnt++;
    if(operation!=0){
//       如果是读请求,直接交由底层
        buffer_read_miss++;
        physical_read++;
        delay+=callFsim(LPN*4,4,1);
        return delay;
    }else{
        buffer_write_miss++;
        cache_write_num++;
        physical_read++;
        delay+=callFsim(LPN*4,4,1);
    }
    pHit=FindHitBlkNode(BPLRU_Head,LPN,&flag);
//    如果请求的块不存在
    if(pHit==NULL){
        pHit=(pBlkNode)malloc(sizeof(BlkNode));
        if(pHit==NULL){
            fprintf(stderr,"error happened in BPLRU_AddCacheEntry:\n");
            fprintf(stderr,"malloc for new blknode is failed\n");
            assert(0);
        }
//       初始化
        pHit->BlkSize=1;
        pHit->BlkNum=LPN/PAGE_NUM_PER_BLK;
        for ( i = 0; i <PAGE_NUM_PER_BLK ; ++i) {
            pHit->list[i]=-1;
        }
        free_pos=find_free_pos(pHit->list,PAGE_NUM_PER_BLK);
        if(free_pos==-1){
            fprintf(stderr,"error happened in BPLRU_AddCacheEntry:\n");
            fprintf(stderr,"can not find free pos in list\n");
            assert(0);
        }
        pHit->list[free_pos]=LPN;
        BlkAddNewToMRU(BPLRU_Head,pHit);
        BPLRU_CACHE_SIZE++;
        BPLRU_BLK_NUM++;
    }else{
//        请求的块存在,找到块中的空余位置放置新的LPN
        free_pos=find_free_pos(pHit->list,PAGE_NUM_PER_BLK);
        if(free_pos==-1){
            fprintf(stderr,"error happened in BPLRU_AddCacheEntry:\n");
            fprintf(stderr,"can not find free pos in list\n");
            assert(0);
        }
        pHit->list[free_pos]=LPN;
        pHit->BlkSize++;
        BPLRU_CACHE_SIZE++;
        BlkMoveToMRU(BPLRU_Head,pHit);
    }
//    如果当前模式为顺序写模式,需要启用LRU补偿机制
    if(Seq_flag==1){
        BlkMoveToLRU(BPLRU_Head,pHit);
    }

//    test debug
    if(BPLRU_BLK_NUM!=GetBlkListLength(BPLRU_Head)){
        fprintf(stderr,"error happened in BPLRU_AddCacheEntry:\n");
        fprintf(stderr,"BPLRU_BLK_NUM is %d\t BPLRU-list size is %d\n",BPLRU_BLK_NUM,GetBlkListLength(BPLRU_Head));
        assert(0);
    }
    if(pHit->BlkSize!=calculate_arr_positive_num(pHit->list,PAGE_NUM_PER_BLK)){
        fprintf(stderr,"error happened in BPLRU_AddCacheEntry:\n");
        fprintf(stderr,"pHit->BlkSize is %d\tpHit->list size is %d\n",pHit->BlkSize,calculate_arr_positive_num(pHit->list,PAGE_NUM_PER_BLK));
        assert(0);
    }
    if(BPLRU_CACHE_SIZE!=BlkGetCacheSize(BPLRU_Head)){
        fprintf(stderr,"error happened in BPLRU_AddCacheEntry:\n");
        fprintf(stderr,"BPLRU_CACHE_SIZE is %d\t list-cache size is %d\n",BPLRU_CACHE_SIZE,BlkGetCacheSize(BPLRU_Head));
        assert(0);
    }

    return delay;
}


//实现页填充的聚簇回写
double BPLRU_DelCacheEntry(int ReqLPN,int Reqoperation)
{
    double delay=0.0;
    pBlkNode pVictim=NULL;
    int tempBlk;
    int i,flag;
    int DelSize=0,DelBlkNum,curr_LPN,extra_physical_read;
//    缓冲区未满,不执行剔除操作
    if(BPLRU_CACHE_SIZE<BPLRU_MAX_CACHE_SIZE){
        return delay;
    }

//    一般选择LRU位置的块删除
    pVictim=BPLRU_Head->Pre;
    tempBlk=ReqLPN/PAGE_NUM_PER_BLK;
//    做个代码纠正，如果尾部的块为正在访问的数据块是不允许删除该块的，选择上面一个块删除
    if(pVictim->BlkNum==tempBlk){
        pVictim=pVictim->Pre;
    }
//  页填充先读缺少的页到缓冲区
    DelSize=pVictim->BlkSize;
    DelBlkNum=pVictim->BlkNum;
    extra_physical_read=PAGE_NUM_PER_BLK-DelSize;
    physical_read+=extra_physical_read;
//    确定数据页再没缓冲区，没有先读入,读取操作
    for (curr_LPN = PAGE_NUM_PER_BLK*DelBlkNum; curr_LPN < PAGE_NUM_PER_BLK*(DelBlkNum+1); curr_LPN++) {
        flag=0;
        for ( i = 0; i <DelSize ; ++i) {
            if(curr_LPN==pVictim->list[i]){
                flag=1;
                break;
            }
        }
        if(flag==1){
            delay+=callFsim(curr_LPN*4,4,1);
        }
    }
//  整块回写
    physical_write+=PAGE_NUM_PER_BLK;
    delay+=callFsim(PAGE_NUM_PER_BLK*DelBlkNum*4,PAGE_NUM_PER_BLK*4,0);
//  释放相应的节点
    BPLRU_CACHE_SIZE-=BlkDeleteNode(BPLRU_Head,pVictim);
    BPLRU_BLK_NUM--;

//    debug test
    if(BPLRU_BLK_NUM!=GetBlkListLength(BPLRU_Head)){
        fprintf(stderr,"error happened in BPLRU_DelCacheEntry:\n");
        fprintf(stderr,"BPLRU_BLK_NUM is %d\t BPLRU-list size is %d\n",BPLRU_BLK_NUM,GetBlkListLength(BPLRU_Head));
        assert(0);
    }

    if(BPLRU_CACHE_SIZE!=BlkGetCacheSize(BPLRU_Head)){
        fprintf(stderr,"error happened in BPLRU_DelCacheEntry:\n");
        fprintf(stderr,"BPLRU_CACHE_SIZE is %d\t list-cache size is %d\n",BPLRU_CACHE_SIZE,BlkGetCacheSize(BPLRU_Head));
        assert(0);
    }


    return delay;
}

struct cache_operation BPLRU_Operation={
        init:   BPLRU_init,
        SearchCache:    BPLRU_Search,
        HitCache:   BPLRU_HitCache,
        AddCacheEntry:  BPLRU_AddCacheEntry,
        DelCacheEntry:  BPLRU_DelCacheEntry,
        end:    BPLRU_end
};

struct cache_operation * BPLRU_op_setup()
{
    return &BPLRU_Operation;
}