#include "fuzzy.h"

extern int fuzzy_match(char_u *str, char_u *pat_arg, int matchseq, int *outScore,
                int_u *matches, int maxMatches);
