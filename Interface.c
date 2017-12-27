//
// Created by zhouheyu on 17-12-27.
//
#include "global.h"
#include "Interface.h"
#include "flash.h"
#include "fast.h"
int old_merge_switch_num = 0;
int old_merge_partial_num = 0;
int old_merge_full_num= 0;
int old_flash_gc_read_num = 0;
int old_flash_erase_num = 0;
int req_count_num = 1;
int cache_hit, rqst_cnt;
int flag1 = 1;
int count = 0;

//二级映射地址的大小
int page_num_for_2nd_map_table;

#define MAP_REAL_MAX_ENTRIES 6552// real map table size in bytes
#define MAP_GHOST_MAX_ENTRIES 1640// ghost_num is no of entries chk if this is ok

#define CACHE_MAX_ENTRIES 300
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

    ftl_op->init(total_util_blk_num, total_extr_blk_num);

    nand_stat_reset();
    //在这里可以申请分配对应的缓冲区的内存段

}

//初始化对应的要释放内存段
void endFlash()
{
    nand_stat_print(outputfile);
    ftl_op->end;
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


double callFsim(unsigned int secno, int scount, int operation)
{
    double delay;
    int bcount;
    unsigned int blkno; // pageno for page based FTL
    int cnt;


    blkno = secno / 4;
    blkno += page_num_for_2nd_map_table;
    bcount = (secno + scount -1)/4 - (secno)/4 + 1;


    cnt = bcount;


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

