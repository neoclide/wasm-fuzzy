#include "fuzzy.h"
#include "search.h"

__attribute__((export_name("fuzzyMatch"))) int
fuzzyMatch(char_u *str, char_u *pattern, int_u *matches, int matchseq) {
  int score = 0;
  int res = 0;
  res = fuzzy_match(str, pattern, matchseq, &score, matches, MAX_FUZZY_MATCHES);
  if (!res)
    return 0;
  return score;
}
