#ifndef FUZZY__H
# define FUZZY__H


#include <stdlib.h>
#include <string.h>
#include <stddef.h>
#include <string.h>
#define NUL '\000'
#define MAX_FUZZY_MATCHES	256
#define FUZZY_MATCH_RECURSION_LIMIT	10
#define HT_INIT_SIZE 16
#define NUMBUFLEN 65
#define MAX_FUZZY_MATCHES	256

/*
 * Boolean constants
 */
#ifndef TRUE
# define FALSE	0	    // note: this is an int, not a long!
# define TRUE	1
#endif

#define ALLOC_CLEAR_MULT(type, count)  (type *)alloc_clear(sizeof(type) * (count))

#define FOR_ALL_LIST_ITEMS(l, li) \
    for ((li) = (l) == NULL ? NULL : (l)->lv_first; (li) != NULL; (li) = (li)->li_next)
#define STRCMP(d, s)	    strcmp((char *)(d), (char *)(s))

#ifndef mch_memmove
# define mch_memmove(to, from, len) memmove((char*)(to), (char*)(from), (size_t)(len))
#endif

#define VIM_ISWHITE(x)  ((x) == ' ' || (x) == '\t')

#define TOUPPER_ASC(c)	(((c) < 'a' || (c) > 'z') ? (c) : (c) - ('a' - 'A'))
#define TOLOWER_ASC(c)	(((c) < 'A' || (c) > 'Z') ? (c) : (c) + ('a' - 'A'))

#define ARRAY_LENGTH(a) (sizeof(a) / sizeof((a)[0]))

/*
 * In a hashtab item "hi_key" points to "di_key" in a dictitem.
 * This avoids adding a pointer to the hashtab item.
 * DI2HIKEY() converts a dictitem pointer to a hashitem key pointer.
 * HIKEY2DI() converts a hashitem key pointer to a dictitem pointer.
 * HI2DI() converts a hashitem pointer to a dictitem pointer.
 */
#define DI2HIKEY(di) ((di)->di_key)
#define HIKEY2DI(p)  ((dictitem_T *)((p) - offsetof(dictitem_T, di_key)))
#define HI2DI(hi)     HIKEY2DI((hi)->hi_key)
#define STRLEN(s)	    strlen((char *)(s))

typedef unsigned int int_u;
typedef long long varnumber_T;
typedef unsigned char char_u;
typedef double float_T;
// On rare systems "char" is unsigned, sometimes we really want a signed 8-bit
// value.
typedef signed char int8_T;
typedef unsigned long long_u;

typedef struct listvar_S list_T;
typedef struct dictvar_S dict_T;
typedef struct listitem_S listitem_T;
// Struct used by those that are using an item in a list.
typedef struct listwatch_S listwatch_T;
// A type specification.
typedef struct type_S type_T;
/*
 * Item for a hashtable.  "hi_key" can be one of three values:
 * NULL:	   Never been used
 * HI_KEY_REMOVED: Entry was removed
 * Otherwise:	   Used item, pointer to the actual key; this usually is
 *		   inside the item, subtract an offset to locate the item.
 *		   This reduces the size of hashitem by 1/3.
 */
typedef struct hashitem_S {
  long_u hi_hash; // cached hash number of hi_key
  char_u *hi_key;
} hashitem_T;

typedef enum {
  VAR_UNKNOWN = 0, // not set, any type or "void" allowed
  VAR_ANY,         // used for "any" type
  VAR_VOID,        // no value (function not returning anything)
  VAR_BOOL,        // "v_number" is used: VVAL_TRUE or VVAL_FALSE
  VAR_NUMBER,      // "v_number" is used
  VAR_FLOAT,       // "v_float" is used
  VAR_STRING,      // "v_string" is used
  VAR_LIST,        // "v_list" is used
  VAR_DICT,        // "v_dict" is used
} vartype_T;

typedef struct hashtable_S {
  long_u ht_mask;       // mask used for hash value (nr of items in
                        // array is "ht_mask" + 1)
  long_u ht_used;       // number of items used
  long_u ht_filled;     // number of items used + removed
  int ht_changed;       // incremented when adding or removing an item
  int ht_locked;        // counter for hash_lock()
  int ht_error;         // when set growing failed, can't add more
                        // items before growing works
  hashitem_T *ht_array; // points to the array, allocated when it's
                        // not "ht_smallarray"
  hashitem_T ht_smallarray[HT_INIT_SIZE]; // initial array
} hashtab_T;

/*
 * Structure to hold an internal variable without a name.
 */
typedef struct {
  vartype_T v_type;
  char v_lock; // see below: VAR_LOCKED, VAR_FIXED
  union {
    list_T *v_list;       // list value (can be NULL!)
    dict_T *v_dict;       // dict value (can be NULL!)
    varnumber_T v_number; // number value
    float_T v_float;      // floating number value
    char_u *v_string;     // string value (can be NULL!)
  } vval;
} typval_T;

/*
 * Structure to hold info about a list.
 * Order of members is optimized to reduce padding.
 * When created by range() it will at first have special value:
 *  lv_first == &range_list_item;
 * and use lv_start, lv_end, lv_stride.
 */
struct listvar_S {
  listitem_T *lv_first;  // first item, NULL if none, &range_list_item
                         // for a non-materialized list
  listwatch_T *lv_watch; // first watcher, NULL if none
  union {
    struct { // used for non-materialized range list:
             // "lv_first" is &range_list_item
      varnumber_T lv_start;
      varnumber_T lv_end;
      int lv_stride;
    } nonmat;
    struct {                   // used for materialized list
      listitem_T *lv_last;     // last item, NULL if none
      listitem_T *lv_idx_item; // when not NULL item at index "lv_idx"
      int lv_idx;              // cached index of an item
    } mat;
  } lv_u;
  type_T *lv_type;      // current type, allocated by alloc_type()
  list_T *lv_copylist;  // copied list used by deepcopy()
  list_T *lv_used_next; // next list in used lists list
  list_T *lv_used_prev; // previous list in used lists list
  int lv_refcount;      // reference count
  int lv_len;           // number of items
  int lv_with_items;    // number of items following this struct that
                        // should not be freed
  int lv_copyID;        // ID used by deepcopy()
};

struct listitem_S {
  listitem_T *li_next; // next item in list
  listitem_T *li_prev; // previous item in list
  typval_T li_tv;      // type and value of the variable
};

struct listwatch_S {
  listitem_T *lw_item;  // item being watched
  listwatch_T *lw_next; // next watcher
};

struct type_S {
  vartype_T tt_type;
  int8_T tt_argcount;     // for func, incl. vararg, -1 for unknown
  int8_T tt_min_argcount; // number of non-optional arguments
  char_u tt_flags;        // TTFLAG_ values
  type_T *tt_member;      // for list, dict, func return type
  type_T **tt_args;       // func argument types, allocated
};

/*
 * Structure to hold info about a Dictionary.
 */
struct dictvar_S {
  char dv_lock;         // zero, VAR_LOCKED, VAR_FIXED
  char dv_scope;        // zero, VAR_SCOPE, VAR_DEF_SCOPE
  int dv_refcount;      // reference count
  int dv_copyID;        // ID used by deepcopy()
  hashtab_T dv_hashtab; // hashtab that refers to the items
  type_T *dv_type;      // current type, allocated by alloc_type()
  dict_T *dv_copydict;  // copied dict used by deepcopy()
  dict_T *dv_used_next; // next dict in used dicts list
  dict_T *dv_used_prev; // previous dict in used dicts list
};

/*
 * Structure to hold an item of a Dictionary.
 * Also used for a variable.
 * The key is copied into "di_key" to avoid an extra alloc/free for it.
 */
struct dictitem_S {
  typval_T di_tv;   // type and value of the variable
  char_u di_flags;  // DI_FLAGS_ flags (only used for variable)
  char_u di_key[1]; // key (actually longer!)
};
typedef struct dictitem_S dictitem_T;

#endif // FUZZY__H
