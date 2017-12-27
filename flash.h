/* 
 * Contributors: Youngjae Kim (youkim@cse.psu.edu)
 *               Aayush Gupta (axg354@cse.psu.edu)
 *
 * In case if you have any doubts or questions, kindly write to: youkim@cse.psu.edu 
 *
 * Description: This is a header file for flash.c.
 *
 * Acknowledgement: We thank Jeong Uk Kang by sharing the initial version 
 * of sector-level FTL source code. 
 *
 */

#ifndef SSD_LAYOUT 
#define SSD_LAYOUT 

#include  "type.h"

#define LB_SIZE_512  1
#define LB_SIZE_1024 2
#define LB_SIZE_2048 4

#define SECT_NUM_PER_PAGE  4
#define PAGE_NUM_PER_BLK  64
#define SECT_NUM_PER_BLK  (SECT_NUM_PER_PAGE * PAGE_NUM_PER_BLK)
#define SECT_SIZE_B 512

#define SECT_BITS       2
#define PAGE_BITS       6
#define PAGE_SECT_BITS  8
#define BLK_BITS       24

#define NAND_STATE_FREE    -1
#define NAND_STATE_INVALID -2

#define SECT_MASK_IN_SECT 0x0003
#define PAGE_MASK_IN_SECT 0x00FC
#define PAGE_SECT_MASK_IN_SECT 0x00FF
#define BLK_MASK_IN_SECT  0xFFFFFF00
#define PAGE_BITS_IN_PAGE 0x003F
#define BLK_MASK_IN_PAGE  0x3FFFFFC0

#define PAGE_SIZE_B (SECT_SIZE_B * SECT_NUM_PER_PAGE)
#define PAGE_SIZE_KB (PAGE_SIZE_B / 1024)
#define BLK_SIZE_B  (PAGE_SIZE_B * PAGE_NUM_PER_BLK)
#define BLK_SIZE_KB (BLK_SIZE_B / 1024)

#define BLK_NO_SECT(sect)  (((sect) & BLK_MASK_IN_SECT) >> (PAGE_BITS + SECT_BITS))
#define PAGE_NO_SECT(sect) (((sect) & PAGE_MASK_IN_SECT) >> SECT_BITS)
#define SECT_NO_SECT(sect) ((sect) & SECT_MASK_IN_SECT)
#define BLK_PAGE_NO_SECT(sect) ((sect) >> SECT_BITS)
#define PAGE_SECT_NO_SECT(sect) ((sect) & PAGE_SECT_MASK_IN_SECT)
#define BLK_NO_PAGE(page)  (((page) & BLK_MASK_IN_PAGE) >> PAGE_BITS)
#define PAGE_NO_PAGE(page) ((page) & PAGE_MASK_IN_PAGE)
#define SECTOR(blk, page) (((blk) << PAGE_SECT_BITS) | (page))

#define BLK_MASK_SECT 0x3FFFFF00
#define PGE_MASK_SECT 0x000000FC
#define OFF_MASK_SECT 0x00000003
#define IND_MASK_SECT (PGE_MASK_SECT | OFF_MASK_SECT)
#define BLK_BITS_SECT 22
#define PGE_BITS_SECT  6
#define OFF_BITS_SECT  2
#define IND_BITS_SECT (PGE_BITS_SECT + OFF_BITS_SECT)
#define BLK_F_SECT(sect) (((sect) & BLK_MASK_SECT) >> IND_BITS_SECT)
#define PGE_F_SECT(sect) (((sect) & PGE_MASK_SECT) >> OFF_BITS_SECT)
#define OFF_F_SECT(sect) (((sect) & OFF_MASK_SECT))
#define PNI_F_SECT(sect) (((sect) & (~OFF_MASK_SECT)) >> OFF_BITS_SECT)
#define IND_F_SECT(sect) (((sect) & IND_MASK_SECT))
#define IS_SAME_BLK(s1, s2) (((s1) & BLK_MASK_SECT) == ((s2) & BLK_MASK_SECT))
#define IS_SAME_PAGE(s1, s2) (((s1) & (~OFF_MASK_SECT)) == ((s2) & (~OFF_MASK_SECT)))

struct blk_state {
   int free;
   int ec;
   int update_ec;
};
struct sect_state {
  _u32 free  :  1;
  _u32 valid :  1;
  _u32 lsn   : 30;
};

struct nand_blk_info {
  struct blk_state state;                   // Erase Conunter
  struct sect_state sect[SECT_NUM_PER_BLK]; // Logical Sector Number
  _s32 fpc : 10; // free page counter
  _s32 ipc : 10; // invalide page counter
  _s32 lwn : 12; // last written page number
  int page_status[PAGE_NUM_PER_BLK];
};

extern _u32 nand_blk_num;
extern _u8  pb_size;
extern struct nand_blk_info *nand_blk;
extern FILE *fp_erase;
int nand_init (_u32 blk_num, _u8 min_free_blk_num);
void nand_end ();
_u8 nand_page_read (_u32 psn, _u32 *lsns, _u8 isGC);
_u8 nand_page_write (_u32 psn, _u32 *lsns, _u8 isGC, int map_flag);
void nand_erase (_u32 blk_no);
void nand_invalidate (_u32 psn, _u32 lsn);
_u32 nand_get_free_blk(int);
void nand_stat(int);
void nand_stat_reset();
void nand_stat_print(FILE *outFP);
int nand_oob_read(_u32 psn);

_u32 free_blk_num;
_u32 free_blk_idx;

_u32 stat_read_num, stat_write_num, stat_erase_num;
_u32 stat_gc_read_num, stat_gc_write_num;
_u32 stat_oob_read_num, stat_oob_write_num;
#endif 

#define WEAR_LEVEL_THRESHOLD   35 

void flush(int); 
