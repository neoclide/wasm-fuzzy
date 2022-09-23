#include "alloc.h"
#include "fuzzy.h"
#include "mbyte.h"
#include "strings.h"

typedef struct {
  int idx; // used for stable sort
  listitem_T *item;
  int score;
  list_T *lmatchpos;
} fuzzyItem_T;

// bonus for adjacent matches; this is higher than SEPARATOR_BONUS so that
// matching a whole word is preferred.
#define SEQUENTIAL_BONUS 40
// bonus if match occurs after a path separator
#define PATH_SEPARATOR_BONUS 30
// bonus if match occurs after a word separator
#define WORD_SEPARATOR_BONUS 25
// bonus if match is uppercase and prev is lower
#define CAMEL_BONUS 30
// bonus if the first letter is matched
#define FIRST_LETTER_BONUS 15
// penalty applied for every letter in str before the first match
#define LEADING_LETTER_PENALTY (-5)
// maximum penalty for leading letters
#define MAX_LEADING_LETTER_PENALTY (-15)
// penalty for every letter that doesn't match
#define UNMATCHED_LETTER_PENALTY (-1)
// penalty for gap in matching positions (-2 * k)
#define GAP_PENALTY (-2)
// Score for a string that doesn't fuzzy match the pattern
#define SCORE_NONE (-9999)

#define FUZZY_MATCH_RECURSION_LIMIT 10

/*
 * Compute a score for a fuzzy matched string. The matching character locations
 * are in 'matches'.
 */
static int fuzzy_match_compute_score(char_u *str, int strSz, int_u *matches,
                                     int numMatches) {
  int score;
  int penalty;
  int unmatched;
  int i;
  char_u *p = str;
  int_u sidx = 0;

  // Initialize score
  score = 100;

  // Apply leading letter penalty
  penalty = LEADING_LETTER_PENALTY * matches[0];
  if (penalty < MAX_LEADING_LETTER_PENALTY)
    penalty = MAX_LEADING_LETTER_PENALTY;
  score += penalty;

  // Apply unmatched penalty
  unmatched = strSz - numMatches;
  score += UNMATCHED_LETTER_PENALTY * unmatched;

  // Apply ordering bonuses
  for (i = 0; i < numMatches; ++i) {
    int_u currIdx = matches[i];

    if (i > 0) {
      int_u prevIdx = matches[i - 1];

      // Sequential
      if (currIdx == (prevIdx + 1))
        score += SEQUENTIAL_BONUS;
      else
        score += GAP_PENALTY * (currIdx - prevIdx);
    }

    // Check for bonuses based on neighbor character value
    if (currIdx > 0) {
      // Camel case
      int neighbor = ' ';
      int curr;

      while (sidx < currIdx) {
        neighbor = (*utf_ptr2char)(p);
        p += (*utf_ptr2len)(p);
        sidx++;
      }
      curr = (*utf_ptr2char)(p);

      if (vim_islower(neighbor) && vim_isupper(curr))
        score += CAMEL_BONUS;

      // Bonus if the match follows a separator character
      if (neighbor == '/' || neighbor == '\\')
        score += PATH_SEPARATOR_BONUS;
      else if (neighbor == ' ' || neighbor == '_')
        score += WORD_SEPARATOR_BONUS;
    } else {
      // First letter
      score += FIRST_LETTER_BONUS;
    }
  }
  return score;
}

/*
 * Perform a recursive search for fuzzy matching 'fuzpat' in 'str'.
 * Return the number of matching characters.
 */
static int fuzzy_match_recursive(char_u *fuzpat, char_u *str, int_u strIdx,
                                 int *outScore, char_u *strBegin, int strLen,
                                 int_u *srcMatches, int_u *matches,
                                 int maxMatches, int nextMatch,
                                 int *recursionCount) {
  // Recursion params
  int recursiveMatch = FALSE;
  int_u bestRecursiveMatches[MAX_FUZZY_MATCHES];
  int bestRecursiveScore = 0;
  int first_match;
  int matched;

  // Count recursions
  ++*recursionCount;
  if (*recursionCount >= FUZZY_MATCH_RECURSION_LIMIT)
    return 0;

  // Detect end of strings
  if (*fuzpat == NUL || *str == NUL)
    return 0;

  // Loop through fuzpat and str looking for a match
  first_match = TRUE;
  while (*fuzpat != NUL && *str != NUL) {
    int c1;
    int c2;

    c1 = utf_ptr2char(fuzpat);
    c2 = utf_ptr2char(str);

    // Found match
    if (vim_tolower(c1) == vim_tolower(c2)) {
      int_u recursiveMatches[MAX_FUZZY_MATCHES];
      int recursiveScore = 0;
      char_u *next_char;

      // Supplied matches buffer was too short
      if (nextMatch >= maxMatches)
        return 0;

      // "Copy-on-Write" srcMatches into matches
      if (first_match && srcMatches) {
        memcpy(matches, srcMatches, nextMatch * sizeof(srcMatches[0]));
        first_match = FALSE;
      }

      // Recursive call that "skips" this match
      next_char = str + (*utf_ptr2len)(str);
      if (fuzzy_match_recursive(fuzpat, next_char, strIdx + 1, &recursiveScore,
                                strBegin, strLen, matches, recursiveMatches,
                                ARRAY_LENGTH(recursiveMatches), nextMatch,
                                recursionCount)) {
        // Pick best recursive score
        if (!recursiveMatch || recursiveScore > bestRecursiveScore) {
          memcpy(bestRecursiveMatches, recursiveMatches,
                 MAX_FUZZY_MATCHES * sizeof(recursiveMatches[0]));
          bestRecursiveScore = recursiveScore;
        }
        recursiveMatch = TRUE;
      }

      // Advance
      matches[nextMatch++] = strIdx;
      fuzpat += (*utf_ptr2len)(fuzpat);
    }
    str += (*utf_ptr2len)(str);
    strIdx++;
  }

  // Determine if full fuzpat was matched
  matched = *fuzpat == NUL ? TRUE : FALSE;

  // Calculate score
  if (matched)
    *outScore = fuzzy_match_compute_score(strBegin, strLen, matches, nextMatch);

  // Return best result
  if (recursiveMatch && (!matched || bestRecursiveScore > *outScore)) {
    // Recursive score is better than "this"
    memcpy(matches, bestRecursiveMatches, maxMatches * sizeof(matches[0]));
    *outScore = bestRecursiveScore;
    return nextMatch;
  } else if (matched)
    return nextMatch; // "this" score is better than recursive

  return 0; // no match
}

/*
 * fuzzy_match()
 *
 * Performs exhaustive search via recursion to find all possible matches and
 * match with highest score.
 * Scores values have no intrinsic meaning.  Possible score range is not
 * normalized and varies with pattern.
 * Recursion is limited internally (default=10) to prevent degenerate cases
 * (pat_arg="aaaaaa" str="aaaaaaaaaaaaaaaaaaaaaaaaaaaaaa").
 * Uses char_u for match indices. Therefore patterns are limited to
 * MAX_FUZZY_MATCHES characters.
 * If 'matchseq' is TRUE, then for multi-word search strings, match all the
 * words in sequence.
 *
 * Returns TRUE if 'pat_arg' matches 'str'. Also returns the match score in
 * 'outScore' and the matching character positions in 'matches'.
 */
int fuzzy_match(char_u *str, char_u *pat_arg, int matchseq, int *outScore,
                int_u *matches, int maxMatches) {
  int recursionCount = 0;
  int len = mb_charlen(str);
  char_u *save_pat;
  char_u *pat;
  char_u *p;
  int complete = FALSE;
  int score = 0;
  int numMatches = 0;
  int matchCount;

  *outScore = 0;

  save_pat = vim_strsave(pat_arg);
  if (save_pat == NULL)
    return FALSE;
  pat = save_pat;
  p = pat;

  // Try matching each word in 'pat_arg' in 'str'
  while (TRUE) {
    if (matchseq)
      complete = TRUE;
    else {
      // Extract one word from the pattern (separated by space)
      p = skipwhite(p);
      if (*p == NUL)
        break;
      pat = p;
      while (*p != NUL && !VIM_ISWHITE(utf_ptr2char(p))) {
        p += (*utf_ptr2len)(p);
      }
      if (*p == NUL) // processed all the words
        complete = TRUE;
      *p = NUL;
    }

    score = 0;
    recursionCount = 0;
    matchCount = fuzzy_match_recursive(
        pat, str, 0, &score, str, len, NULL, matches + numMatches,
        maxMatches - numMatches, 0, &recursionCount);
    if (matchCount == 0) {
      numMatches = 0;
      break;
    }

    // Accumulate the match score and the number of matches
    *outScore += score;
    numMatches += matchCount;

    if (complete)
      break;

    // try matching the next word
    ++p;
  }

  vim_free(save_pat);
  return numMatches != 0;
}
