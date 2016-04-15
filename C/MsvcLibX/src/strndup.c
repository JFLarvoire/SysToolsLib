#define _CRT_SECURE_NO_WARNINGS 1 /* Avoid Visual C++ security warnings */

#include <string.h>
#include <stdlib.h>

/* Duplicate a string, up to at most size characters */
char *strndup(const char *s, size_t size) {
  size_t l;
  char *s2;
  l = strlen(s);
  if (l > size) l=size;
  s2 = malloc(l+1);
  if (s2) {
    strncpy(s2, s, l);
    s2[l] = '\0';
  }
  return s2;
}

