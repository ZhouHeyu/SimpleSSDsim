#ifndef TYPES
#define TYPES

#include <stdio.h>
#include <stdint.h>

#include <assert.h>
#define ASSERT(expr) assert(expr)
#define EXIT(expr) ASSERT(0)

typedef int_least16_t _s16;
typedef int_least32_t _s32;
typedef uint_least8_t _u8;
typedef uint_least16_t _u16;
typedef uint_least32_t _u32;

typedef uint_fast32_t sect_t;
typedef uint_fast32_t blk_t;

struct ftl_operation {
  int    (*init) (blk_t blk_num, blk_t extra_num);
  size_t (*read) (sect_t lsn, size_t size, int map_flag);
  size_t (*write)(sect_t lsn, size_t size, int map_flag);
  void   (*end)  ();
};
#endif 
