#include "alloc.h"
#include "fuzzy.h"
#include "mbyte.h"
#include <ctype.h>

/*
 * Copy "string" into newly allocated memory.
 */
char_u *vim_strsave(char_u *string) {
  char_u *p;
  size_t len;

  len = STRLEN(string) + 1;
  p = alloc(len);
  if (p != NULL)
    mch_memmove(p, string, len);
  return p;
}

/*
 * Skip over ' ' and '\t'.
 */
char_u *skipwhite(char_u *q) {
  char_u *p = q;

  while (VIM_ISWHITE(*p))
    ++p;
  return p;
}

int vim_tolower(int c) {
  if (c <= '@')
    return c;
  if (c >= 0x80) {
    return utf_tolower(c);
  }
  return TOLOWER_ASC(c);
}

int vim_islower(int c) {
  if (c <= '@')
    return FALSE;
  if (c >= 0x80) {
    return utf_islower(c);
  }
  return islower(c);
}

int vim_isupper(int c) {
  if (c <= '@')
    return FALSE;
  if (c >= 0x80) {
    return utf_isupper(c);
  }
  return isupper(c);
}
