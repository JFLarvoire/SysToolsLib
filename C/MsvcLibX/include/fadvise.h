/* Give hints to the kernel about future uses of a file */
/* msvclibx: Define constants, and replace functions by void macros */

enum fadvice_t { 
  FADVISE_NORMAL,
  FADVISE_SEQUENTIAL,
  FADVISE_NOREUSE,
  FADVISE_DONTNEED, 
  FADVISE_WILLNEED,
  FADVISE_RANDOM 
};
 
/* void	fdadvise(int fd, off_t offset, off_t len, fadvice_t advice) */
#define fdadvise(fd, offset, len, advice)

/* void fadvise(FILE *fp, fadvice_t advice) */
#define fadvise(fp, advice)
