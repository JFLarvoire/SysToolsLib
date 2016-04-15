/* st_*time is 64-bits __time64_t  &  st_size is 64-bits __off64_t */

#undef _USE_32BIT_TIME_T
#define _FILE_OFFSET_BITS 64

#include "lstat.c"
