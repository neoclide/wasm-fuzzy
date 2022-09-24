#ifndef FUZZY__H
# define FUZZY__H

#include <string.h>
#define NUL '\000'
#define MAX_FUZZY_MATCHES	256
#define FUZZY_MATCH_RECURSION_LIMIT	10
#define HT_INIT_SIZE 16
#define NUMBUFLEN 65
#define MAX_FUZZY_MATCHES	256

#ifndef TRUE
# define FALSE	0	    // note: this is an int, not a long!
# define TRUE	1
#endif

#ifndef mch_memmove
# define mch_memmove(to, from, len) memmove((char*)(to), (char*)(from), (size_t)(len))
#endif

#define VIM_ISWHITE(x)  ((x) == ' ' || (x) == '\t')
#define TOUPPER_ASC(c)	(((c) < 'a' || (c) > 'z') ? (c) : (c) - ('a' - 'A'))
#define TOLOWER_ASC(c)	(((c) < 'A' || (c) > 'Z') ? (c) : (c) + ('a' - 'A'))
#define ARRAY_LENGTH(a) (sizeof(a) / sizeof((a)[0]))
#define STRLEN(s)	    strlen((char *)(s))

typedef unsigned int int_u;
typedef long long varnumber_T;
typedef unsigned char char_u;
typedef double float_T;
// On rare systems "char" is unsigned, sometimes we really want a signed 8-bit value.
typedef signed char int8_T;
typedef unsigned long long_u;

#endif // FUZZY__H
