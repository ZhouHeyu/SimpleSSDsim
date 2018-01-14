//
// Created by zj on 18-1-8.
//

#include "Cache.h"
void Buffer_Stat_Reset()
{
    //初始化统计变量
    buffer_cnt=0;
    buffer_hit_cnt=0;
    buffer_miss_cnt=0;
    buffer_write_hit=0;
    buffer_write_miss=0;
    buffer_read_hit=0;
    buffer_read_miss=0;
    physical_read=0;
    physical_write=0;
    reset_cache_stat();
}


void Buffer_Stat_Print(FILE *outFP)
{
    fprintf(outFP, "\n");
    fprintf(outFP, "CACHE STATISTICS\n");
    fprintf(outFP, "------------------------------------------------------------\n");
    fprintf(outFP,"All buffer req count# %d\n",buffer_cnt);
    fprintf(outFP,"Buffer_Miss_Count # %d\t",buffer_miss_cnt);
    fprintf(outFP,"Hit rate is %f\n",(double)(buffer_cnt-buffer_miss_cnt)/buffer_cnt);
    fprintf(outFP,"Read Hit count is %d\t",buffer_read_hit);
    fprintf(outFP,"Write Hit count is %d\n",buffer_write_hit);
    fprintf(outFP,"Read Miss count is %d\t",buffer_read_miss);
    fprintf(outFP,"Write Miss count is %d\n",buffer_write_miss);
    fprintf(outFP,"Physical Read Count is %d\t",physical_read);
    fprintf(outFP,"Physical Write Count is %d\n",physical_write);
    fprintf(outFP, "------------------------------------------------------------\n");
}

//reset cache state
void reset_cache_stat()
{
    cache_read_num = 0;
    cache_write_num = 0;
}

//计算cache的操作时延
double calculate_delay_cache()
{
    double delay;
    double cache_read_delay=0.0,cache_write_delay=0.0;
    cache_read_delay=(double)CACHE_READ_DELAY*cache_read_num;
    cache_write_delay=(double)CACHE_WRITE_DELAY*cache_write_num;
    delay=cache_read_delay+cache_write_delay;
    reset_cache_stat();
    return delay;
}

int calculate_arr_positive_num(int *arr,int size)
{
    int i;
    int count=0;
    for ( i = 0; i <size ; ++i) {
        if(arr[i]>=0){
            count++;
        }
    }
    return count;
}