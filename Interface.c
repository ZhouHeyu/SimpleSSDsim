//
// Created by zhouheyu on 17-12-27.
//
#include "global.h"
#include "Interface.h"
#include "flash.h"
#include "fast.h"
#include "LRU.h"
#include "CFLRU.h"
#include "CASA.h"
#include "FAB.h"
#include "ADLRU.h"
#include "LRUWSR.h"
#include "CCFLRU.h"
#include "BPLRU.h"

int old_merge_switch_num = 0;
int old_merge_partial_num = 0;
int old_merge_full_num= 0;
int old_flash_gc_read_num = 0;
int old_flash_erase_num = 0;
int req_count_num = 1;
int rqst_cnt;
int flag1 = 1;
int count = 0;

//二级映射地址的大小
int page_num_for_2nd_map_table;

#define MAP_REAL_MAX_ENTRIES 6552// real map table size in bytes
#define MAP_GHOST_MAX_ENTRIES 1640// ghost_num is no of entries chk if this is ok

int ghost_arr[MAP_GHOST_MAX_ENTRIES];
int real_arr[MAP_REAL_MAX_ENTRIES];
/***********************************************************************
  Variables for statistics
 ***********************************************************************/
unsigned int cnt_read = 0;
unsigned int cnt_write = 0;
unsigned int cnt_delete = 0;
unsigned int cnt_evict_from_flash = 0;
unsigned int cnt_evict_into_disk = 0;
unsigned int cnt_fetch_miss_from_disk = 0;
unsigned int cnt_fetch_miss_into_flash = 0;

double sum_of_queue_time = 0.0;
double sum_of_service_time = 0.0;
double sum_of_response_time = 0.0;
unsigned int total_num_of_req = 0;

/***********************************************************************
  Mapping table
 ***********************************************************************/
int real_min = -1;
int real_max = 0;

// Interface between disksim & fsim

void reset_flash_stat()
{
    flash_read_num = 0;
    flash_write_num = 0;
    flash_gc_read_num = 0;
    flash_gc_write_num = 0;
    flash_erase_num = 0;
    flash_oob_read_num = 0;
    flash_oob_write_num = 0;
}

FILE *fp_flash_stat;
FILE *fp_gc;
FILE *fp_gc_timeseries;
double gc_di =0 ,gc_ti=0;

double calculate_delay_flash()
{
    double delay;
    double read_delay, write_delay;
    double erase_delay;
    double gc_read_delay, gc_write_delay;
    double oob_write_delay, oob_read_delay;

    oob_read_delay  = (double)OOB_READ_DELAY  * flash_oob_read_num;
    oob_write_delay = (double)OOB_WRITE_DELAY * flash_oob_write_num;

    read_delay     = (double)READ_DELAY  * flash_read_num;
    write_delay    = (double)WRITE_DELAY * flash_write_num;
    erase_delay    = (double)ERASE_DELAY * flash_erase_num;

    gc_read_delay  = (double)GC_READ_DELAY  * flash_gc_read_num;
    gc_write_delay = (double)GC_WRITE_DELAY * flash_gc_write_num;


    delay = read_delay + write_delay + erase_delay + gc_read_delay + gc_write_delay +
            oob_read_delay + oob_write_delay;

//    统计垃圾回收的时间开销
//    if( flash_gc_read_num > 0 || flash_gc_write_num > 0 || flash_erase_num > 0 ) {
//        gc_ti += delay;
//    }
//    else {
//        gc_di += delay;
//    }
//
//    if(warm_done == 1){
//        fprintf(fp_gc_timeseries, "%d\t%d\t%d\t%d\t%d\t%d\n",
//                req_count_num, merge_switch_num - old_merge_switch_num,
//                merge_partial_num - old_merge_partial_num,
//                merge_full_num - old_merge_full_num,
//                flash_gc_read_num,
//                flash_erase_num);
//
//        old_merge_switch_num = merge_switch_num;
//        old_merge_partial_num = merge_partial_num;
//        old_merge_full_num = merge_full_num;
//        req_count_num++;
//    }

    reset_flash_stat();

    return delay;
}


/***********************************************************************
  Initialize Flash Drive
  ***********************************************************************/
void initFlash()
{
    blk_t total_blk_num;
    blk_t total_util_blk_num;
    blk_t total_extr_blk_num;

    // total number of sectors
    total_util_sect_num  = flash_numblocks;
    total_extra_sect_num = flash_extrblocks;
    total_sect_num = total_util_sect_num + total_extra_sect_num;

    // total number of blocks
    total_blk_num      = total_sect_num / SECT_NUM_PER_BLK;     // total block number
    total_util_blk_num = total_util_sect_num / SECT_NUM_PER_BLK;    // total unique block number

    global_total_blk_num = total_util_blk_num;

    total_extr_blk_num = total_blk_num - total_util_blk_num;        // total extra block number

    ASSERT(total_extr_blk_num != 0);

    if (nand_init(total_blk_num, 3) < 0) {
        EXIT(-4);
    }
//    选择相应的FTL算法
    switch(ftl_type){

//        // pagemap
//        case 1: ftl_op = pm_setup(); break;
//            // blockmap
//            //case 2: ftl_op = bm_setup(); break;
//            // o-pagemap
//        case 3: ftl_op = opm_setup(); break;
//            // fast
        case 4: ftl_op = lm_setup(); break;

        default: break;
    }
//    选择相应的缓冲区算法
    switch(cache_type){
//        LRU
        case 1: cache_op=LRU_op_setup();break;
//        CFLRU
        case 2: cache_op=CFLRU_op_setup();break;
//        AD-LRU
        case 3: cache_op=ADLRU_op_setup();break;
//        CASA
        case 4: cache_op=CASA_op_setup();break;
//        LRU-WSR
        case 5: cache_op=LRUWSR_op_setup();break;
//         CCF-LRU
        case 6:cache_op=CCFLRU_op_setup();break;
//            块级的FAB算法
        case 7:cache_op=FAB_op_setup();break;
//            BPLRU算法
//        case 8:cache_op=BPLRU_op_setup();break;

    }

    ftl_op->init(total_util_blk_num, total_extr_blk_num);
    cache_op->init(cache_size,total_util_blk_num);
    nand_stat_reset();
    //在这里可以申请分配对应的缓冲区的内存段
    Buffer_Stat_Reset();
}

//初始化对应的要释放内存段
void endFlash()
{
    Buffer_Stat_Print(outputfile);
    nand_stat_print(outputfile);
    ftl_op->end();
    cache_op->end();
    nand_end();
    //这里也可以添加相应的缓冲区申请的内存段的释放
}

//统计相应块的磨损状况
void printWearout()
{
    int i;
    FILE *fp = fopen("wearout", "w");

    for(i = 0; i<nand_blk_num; i++)
    {
        fprintf(fp, "%d %d\n", i, nand_blk[i].state.ec);
    }

    fclose(fp);
}


//根据相应的算法选择对应的ftl_op->write/read
/***********************************************************************
  Send request (lsn, sector_cnt, operation flag)
  ***********************************************************************/
//这里的操作的是扇区号(tart_blk_no, block_cnt)
void send_flash_request(int start_blk_no, int block_cnt, int operation, int mapdir_flag)
{
    int size;
    //size_t (*op_func)(sect_t lsn, size_t size);
    size_t (*op_func)(sect_t lsn, size_t size, int mapdir_flag);

    if((start_blk_no + block_cnt) >= total_util_sect_num){
        printf("start_blk_no: %d, block_cnt: %d, total_util_sect_num: %d\n",
               start_blk_no, block_cnt, total_util_sect_num);
        exit(0);
    }

    switch(operation){

        //write
        case 0:

            op_func = ftl_op->write;
            while (block_cnt> 0) {
                //回写这里也是一个扇区一个扇区的回写
                size = op_func(start_blk_no, block_cnt, mapdir_flag);
                start_blk_no += size;
                block_cnt-=size;
            }
            break;
            //read
        case 1:


            op_func = ftl_op->read;
            while (block_cnt> 0) {
                size = op_func(start_blk_no, block_cnt, mapdir_flag);
                start_blk_no += size;
                block_cnt-=size;
            }
            break;

        default:
            break;
    }
}



void find_real_max()
{
    int i;

    for(i=0;i < MAP_REAL_MAX_ENTRIES; i++) {
        if(opagemap[real_arr[i]].map_age > opagemap[real_max].map_age) {
            real_max = real_arr[i];
        }
    }
}

void find_real_min()
{

    int i,index;
    int temp = 99999999;

    for(i=0; i < MAP_REAL_MAX_ENTRIES; i++) {
        if(opagemap[real_arr[i]].map_age <= temp) {
            real_min = real_arr[i];
            temp = opagemap[real_arr[i]].map_age;
            index = i;
        }
    }
}

int find_min_ghost_entry()
{
    int i;

    int ghost_min = 0;
    int temp = 99999999;

    for(i=0; i < MAP_GHOST_MAX_ENTRIES; i++) {
        if( opagemap[ghost_arr[i]].map_age <= temp) {
            ghost_min = ghost_arr[i];
            temp = opagemap[ghost_arr[i]].map_age;
        }
    }
    return ghost_min;
}


void init_arr()
{

    int i;
    for( i = 0; i < MAP_REAL_MAX_ENTRIES; i++) {
        real_arr[i] = -1;
    }
    for( i = 0; i < MAP_GHOST_MAX_ENTRIES; i++) {
        ghost_arr[i] = -1;
    }
}

int search_table(int *arr, int size, int val)
{
    int i;
    for(i =0 ; i < size; i++) {
        if(arr[i] == val) {
            return i;
        }
    }

    printf("shouldnt come here for search_table()=%d,%d",val,size);
    for( i = 0; i < size; i++) {
        if(arr[i] != -1) {
            printf("arr[%d]=%d ",i,arr[i]);
        }
    }
    exit(1);
    return -1;
}


//针对arr数组中存放-1标识为无效数据
int find_free_pos( int *arr, int size)
{
    int i;
    for(i = 0 ; i < size; i++) {
        if(arr[i] == -1) {
            return i;
        }
    }
//    printf("shouldnt come here for find_free_pos()");
//    exit(1);
    return -1;
}

//将数据插入到(int)arr数组指定的位置pos，pos之后的数据往后挪动一位
int InsertArr(int *arr,int size,int data,int pos)
{
    int j;
    //首先做一个错误检测
    if(pos>=size&&pos<0){
        printf("error happend in InsertArr: Insert-pos:%d over bound:0-%d",pos,size-1);
        exit(-1);
    }
    for ( j = size-1; j >pos ; j--) {
        arr[j]=arr[j-1];
    }
    //在pos的位置插入数据
    arr[pos]=data;
    return 0;
}

double callFsim(unsigned int secno, int scount, int operation)
{
    double delay;
    int bcount;
    unsigned int blkno; // pageno for page based FTL
    int cnt;


    blkno = secno / 4;
    bcount = (secno + scount -1)/4 - (secno)/4 + 1;


    cnt = bcount;
    reset_flash_stat();

    switch(operation)
    {
        //write/read
        case 0:
        case 1:

            while(cnt > 0)
            {
                cnt--;
                // FAST scheme
                 if(ftl_type == 4){

                    if(operation == 0){
                        write_count++;
                    }
                    else read_count++;

                    send_flash_request(blkno*4, 4, operation, 1); //cache_min is a page for page baseed FTL
                    blkno++;
                }
            }
            break;
    }

    delay = calculate_delay_flash();

    return delay;

}

int ZJ_flag=0;

double CacheManage(unsigned int secno,int scount,int operation)
{
    double delay,flash_delay=0.0,cache_delay=0.0;
    int bcount;
    unsigned int blkno;
    int HitIndex;
    int cnt=0;
    //页对齐操作
    blkno=secno/4;
    bcount=(secno+scount-1)/4-(secno)/4+1;
    //重置cache_stat相关状态
    reset_cache_stat();
    cnt=bcount;
    while(cnt>0){
        buffer_cnt++;
        HitIndex=cache_op->SearchCache(blkno,operation);
        if(HitIndex==-1){
//            未命中缓冲区
            flash_delay+=cache_op->DelCacheEntry(blkno,operation);
            flash_delay+=cache_op->AddCacheEntry(blkno,operation);
        }else{
//            命中缓冲区
            cache_op->HitCache(blkno,operation,HitIndex);
        }
        blkno++;
        cnt--;
    }
    cache_delay=calculate_delay_cache();
    delay=cache_delay+flash_delay;
    return delay;
}
