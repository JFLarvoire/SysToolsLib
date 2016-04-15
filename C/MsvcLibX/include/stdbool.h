/* Standard boolean definitions */
/* msvclibx: Set standard values. May need to be adapted for C++? */

#ifndef _STDBOOL_H
#define _STDBOOL_H

/* #define bool int /* Spec says _Bool */
typedef int bool;
#define true 1
#define false 0
#define __bool_true_false_are_defined 1

#endif
