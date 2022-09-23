#include "fuzzy.h"
#include "mbyte.h"
#include "search.h"
#include "strings.h"

__attribute__((used)) int fuzzyMatch(char_u *str, char_u *pattern,
                                     int_u *matches, int matchseq) {
  int score = 0;
  int res = 0;
  res = fuzzy_match(str, pattern, matchseq, &score, matches, MAX_FUZZY_MATCHES);
  if (!res)
    return 0;
  return score;
}
