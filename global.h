//
// Created by zhouheyu on 17-12-27.
//

#ifndef SIMULATION_GLOBAL_H
#define SIMULATION_GLOBAL_H
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <time.h>
#include "type.h"


#ifndef TRUE
#define TRUE	1
#endif
#ifndef FALSE
#define FALSE	0
#endif

#ifndef _WIN32
#define	min(x,y)	((x) < (y) ? (x) : (y))

#define	max(x,y)	((x) < (y) ? (y) : (x))
#endif

/* translate from the deprecated but convenient bzero function to memset */
#define bzero(ptr,size)  memset(ptr,0,size)

//定义SSDsim的结构体
typedef struct SSDsim{
    FILE * iotracefile;
    FILE * parfile;
    FILE * outputfile;
    char iotracefilename[256];
    char outputfilename[256];
    char parfilename[256];
    fpos_t iotracefileposition;
    fpos_t outputfileposition;

    double simtime;
    double warmuptime;

    int totalreqs;
    int stop_sim;
    //之后可以拓展定义相关的变量

}SSDsim_t;

SSDsim_t * SSDsim;

//定义一个io请求的结构体
typedef struct ioreq_ev{
    double time;
    int devno;
    int bcount;
    int blkno;
    int operation;
    //之后可以拓展相关的变量
}ioreq_event;

ioreq_event * ioreq;
//关于输入参数的配置
FILE   *outputfile;


//关于底层的配置参数
int flash_numblocks;
int flash_extrblocks;
int cache_type;
int ftl_type;
int cache_size;
//关于CFLRU的参数配置
double CFLRU_alpha;
//关于初始化CASA的Tau值比例,默认是0.5
double CASA_Tau_Ratio;
//关于AD-LRU算法的最小冷区比例,默认是0.2;
double ADLRU_MIN_LC;


//...............这里可以扩展添加之后的参数

//扇区划分
int total_sect_num;
int total_util_sect_num;
int total_extra_sect_num;
//块划分
int total_extr_blk_num;
int total_init_blk_num;

int global_total_blk_num;


//关于nand底层的宏定义
#define READ_DELAY        (0.1309/4)
#define WRITE_DELAY       (0.4059/4)
#define ERASE_DELAY       1.5
#define GC_READ_DELAY  READ_DELAY    // gc read_delay = read delay
#define GC_WRITE_DELAY WRITE_DELAY  // gc write_delay = write delay

#define OOB_READ_DELAY    0.0
#define OOB_WRITE_DELAY   0.0

struct ftl_operation * ftl_op;
//ZhouJie
struct cache_operation *cache_op;


#define PAGE_READ     0
#define PAGE_WRITE    1
#define OOB_READ      2
#define OOB_WRITE     3
#define BLOCK_ERASE   4
#define GC_PAGE_READ  5
#define GC_PAGE_WRITE 6

//nand操作过程中的统计变量
int flash_read_num;
int flash_write_num;
int flash_gc_read_num;
int flash_gc_write_num;
int flash_erase_num;
int flash_oob_read_num;
int flash_oob_write_num;

int map_flash_read_num;
int map_flash_write_num;
int map_flash_gc_read_num;
int map_flash_gc_write_num;
int map_flash_erase_num;
int map_flash_oob_read_num;
int map_flash_oob_write_num;

//涉及到混合FTL操作的交换合并，全合并，部分合并的统计变量
extern int merge_switch_num;
extern int merge_partial_num;
extern int merge_full_num;

int write_count;
int read_count;

//代码运行需要输出的全部统计信息


#endif //SIMULATION_GLOBAL_H
