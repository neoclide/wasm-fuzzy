#include "fuzzy.h"
#include <stdlib.h>

/*
 * Allocate memory and set all bytes to zero.
 */
void *alloc_clear(size_t size) {
  void *p;
  p = malloc(size);
  return p;
}

/*
 * The normal way to allocate memory.  This handles an out-of-memory situation
 * as well as possible, still returns NULL when we're completely out.
 */
void *alloc(size_t size) { return malloc(size); }

/*
 * Replacement for free() that ignores NULL pointers.
 * Also skip free() when exiting for sure, this helps when we caught a deadly
 * signal that was caused by a crash in free().
 * If you want to set NULL after calling this function, you should use
 * VIM_CLEAR() instead.
 */
void vim_free(void *x) {
  if (x != NULL) {
#ifdef MEM_PROFILE
    mem_pre_free(&x);
#endif
    free(x);
  }
}
