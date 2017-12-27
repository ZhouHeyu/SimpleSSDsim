/* 
 * Contributors: Youngjae Kim (youkim@cse.psu.edu)
 *               Aayush Gupta (axg354@cse.psu.edu)
 *
 * In case if you have any doubts or questions, kindly write to: youkim@cse.psu.edu 
 * 
 * Description: This is a header file for dftl.c.  
 * 
 */

#include  "type.h"

struct ftl_operation * lm_setup();
size_t lm_read(sect_t lsn, sect_t size, int mapdir_flag);
size_t lm_write(sect_t lsn, sect_t size, int mapdir_flag);  
void lm_end();
int lm_init(blk_t blk_num, blk_t extra_num);   

struct seq_log_blk get_new_SW_blk();
struct LogMap get_SW_blk_from_PMT();

void merge_switch(int log_pbn, int data_pbn);
void merge_partial(int log_pbn, int data_pbn, int fpc, int req_lsn);
void merge_full_SW(int req_lsn);  
void merge_full(int pmt_index);  

int getRWblk();
int getFirstRWblk();
int getLastlpnfromPMT();
size_t writeToLogBlock(sect_t lsn, int lbn, int lpn);
int getPbnFromBMT(int lbn);

