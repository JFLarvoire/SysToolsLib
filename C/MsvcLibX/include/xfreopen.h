/* The GNU CoreUtils library extends the freopen function */
/* msvclibx: Implement using freopen or _setmode */

#include "msvclibx.h" /* Generate a library search record to load MsvcLibX.lib. */

extern FILE *xfreopen(const char *filename, const char *mode, FILE *stream);

