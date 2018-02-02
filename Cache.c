//
// Created by zj on 18-1-8.
//

#include "Cache.h"
#include "stdlib.h"
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
    fprintf(outFP,"All buffer req count(#) %d\n",buffer_cnt);
    fprintf(outFP,"Buffer_Miss_Count (#) %d\t",buffer_miss_cnt);
    fprintf(outFP,"Hit rate is (#)%f\n",(double)(buffer_cnt-buffer_miss_cnt)/buffer_cnt);
    fprintf(outFP,"Read Hit count(#) is %d\t",buffer_read_hit);
    fprintf(outFP,"Write Hit count is (#)%d\n",buffer_write_hit);
    fprintf(outFP,"Read Miss count is (#)%d\t",buffer_read_miss);
    fprintf(outFP,"Write Miss count is (#)%d\n",buffer_write_miss);
    fprintf(outFP,"Physical Read Count (#)is %d\t",physical_read);
    fprintf(outFP,"Physical Write Count(#) is %d\n",physical_write);
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
    assert(0);
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