/* st_*time is 64-bits __time64_t  &  st_size is 32-bits __off32_t */

#undef _USE_32BIT_TIME_T
#define _FILE_OFFSET_BITS 32

#include "lstat.c"
