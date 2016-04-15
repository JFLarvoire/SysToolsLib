#ifndef  _WIN64 /* WIN64 does not support 32-bits time_t */

/* st_*time is 32-bits __time32_t  &  st_size is 64-bits __off64_t */

#define _USE_32BIT_TIME_T
#define _FILE_OFFSET_BITS 64

#include "lstat.c"
#endif
