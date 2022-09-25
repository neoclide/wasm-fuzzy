#include <ctype.h>
#include <stdlib.h>
#include <string.h>

#define NUL '\000'
#define MAX_FUZZY_MATCHES 256
#define FUZZY_MATCH_RECURSION_LIMIT 10
#define HT_INIT_SIZE 16
#define NUMBUFLEN 65
#define MAX_FUZZY_MATCHES 256

#ifndef TRUE
#define FALSE 0 // note: this is an int, not a long!
#define TRUE 1
#endif

#ifndef mch_memmove
#define mch_memmove(to, from, len)                                             \
  memmove((char *)(to), (char *)(from), (size_t)(len))
#endif

#define VIM_ISWHITE(x) ((x) == ' ' || (x) == '\t')
#define TOUPPER_ASC(c) (((c) < 'a' || (c) > 'z') ? (c) : (c) - ('a' - 'A'))
#define TOLOWER_ASC(c) (((c) < 'A' || (c) > 'Z') ? (c) : (c) + ('a' - 'A'))
#define ARRAY_LENGTH(a) (sizeof(a) / sizeof((a)[0]))
#define STRLEN(s) strlen((char *)(s))

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

typedef unsigned int int_u;
typedef long long varnumber_T;
typedef unsigned char char_u;
typedef double float_T;
// On rare systems "char" is unsigned, sometimes we really want a signed 8-bit
// value.
typedef signed char int8_T;
typedef unsigned long long_u;

/*
 * The following tables are built by ../runtime/tools/unicode.vim.
 * They must be in numeric order, because we use binary search.
 * An entry such as {0x41,0x5a,1,32} means that Unicode characters in the
 * range from 0x41 to 0x5a inclusive, stepping by 1, are changed to
 * folded/upper/lower by adding 32.
 */
typedef struct {
  int rangeStart;
  int rangeEnd;
  int step;
  int offset;
} convertStruct;

static convertStruct toLower[] = {
    {0x41, 0x5a, 1, 32},          {0xc0, 0xd6, 1, 32},
    {0xd8, 0xde, 1, 32},          {0x100, 0x12e, 2, 1},
    {0x130, 0x130, -1, -199},     {0x132, 0x136, 2, 1},
    {0x139, 0x147, 2, 1},         {0x14a, 0x176, 2, 1},
    {0x178, 0x178, -1, -121},     {0x179, 0x17d, 2, 1},
    {0x181, 0x181, -1, 210},      {0x182, 0x184, 2, 1},
    {0x186, 0x186, -1, 206},      {0x187, 0x187, -1, 1},
    {0x189, 0x18a, 1, 205},       {0x18b, 0x18b, -1, 1},
    {0x18e, 0x18e, -1, 79},       {0x18f, 0x18f, -1, 202},
    {0x190, 0x190, -1, 203},      {0x191, 0x191, -1, 1},
    {0x193, 0x193, -1, 205},      {0x194, 0x194, -1, 207},
    {0x196, 0x196, -1, 211},      {0x197, 0x197, -1, 209},
    {0x198, 0x198, -1, 1},        {0x19c, 0x19c, -1, 211},
    {0x19d, 0x19d, -1, 213},      {0x19f, 0x19f, -1, 214},
    {0x1a0, 0x1a4, 2, 1},         {0x1a6, 0x1a6, -1, 218},
    {0x1a7, 0x1a7, -1, 1},        {0x1a9, 0x1a9, -1, 218},
    {0x1ac, 0x1ac, -1, 1},        {0x1ae, 0x1ae, -1, 218},
    {0x1af, 0x1af, -1, 1},        {0x1b1, 0x1b2, 1, 217},
    {0x1b3, 0x1b5, 2, 1},         {0x1b7, 0x1b7, -1, 219},
    {0x1b8, 0x1bc, 4, 1},         {0x1c4, 0x1c4, -1, 2},
    {0x1c5, 0x1c5, -1, 1},        {0x1c7, 0x1c7, -1, 2},
    {0x1c8, 0x1c8, -1, 1},        {0x1ca, 0x1ca, -1, 2},
    {0x1cb, 0x1db, 2, 1},         {0x1de, 0x1ee, 2, 1},
    {0x1f1, 0x1f1, -1, 2},        {0x1f2, 0x1f4, 2, 1},
    {0x1f6, 0x1f6, -1, -97},      {0x1f7, 0x1f7, -1, -56},
    {0x1f8, 0x21e, 2, 1},         {0x220, 0x220, -1, -130},
    {0x222, 0x232, 2, 1},         {0x23a, 0x23a, -1, 10795},
    {0x23b, 0x23b, -1, 1},        {0x23d, 0x23d, -1, -163},
    {0x23e, 0x23e, -1, 10792},    {0x241, 0x241, -1, 1},
    {0x243, 0x243, -1, -195},     {0x244, 0x244, -1, 69},
    {0x245, 0x245, -1, 71},       {0x246, 0x24e, 2, 1},
    {0x370, 0x372, 2, 1},         {0x376, 0x376, -1, 1},
    {0x37f, 0x37f, -1, 116},      {0x386, 0x386, -1, 38},
    {0x388, 0x38a, 1, 37},        {0x38c, 0x38c, -1, 64},
    {0x38e, 0x38f, 1, 63},        {0x391, 0x3a1, 1, 32},
    {0x3a3, 0x3ab, 1, 32},        {0x3cf, 0x3cf, -1, 8},
    {0x3d8, 0x3ee, 2, 1},         {0x3f4, 0x3f4, -1, -60},
    {0x3f7, 0x3f7, -1, 1},        {0x3f9, 0x3f9, -1, -7},
    {0x3fa, 0x3fa, -1, 1},        {0x3fd, 0x3ff, 1, -130},
    {0x400, 0x40f, 1, 80},        {0x410, 0x42f, 1, 32},
    {0x460, 0x480, 2, 1},         {0x48a, 0x4be, 2, 1},
    {0x4c0, 0x4c0, -1, 15},       {0x4c1, 0x4cd, 2, 1},
    {0x4d0, 0x52e, 2, 1},         {0x531, 0x556, 1, 48},
    {0x10a0, 0x10c5, 1, 7264},    {0x10c7, 0x10cd, 6, 7264},
    {0x13a0, 0x13ef, 1, 38864},   {0x13f0, 0x13f5, 1, 8},
    {0x1c90, 0x1cba, 1, -3008},   {0x1cbd, 0x1cbf, 1, -3008},
    {0x1e00, 0x1e94, 2, 1},       {0x1e9e, 0x1e9e, -1, -7615},
    {0x1ea0, 0x1efe, 2, 1},       {0x1f08, 0x1f0f, 1, -8},
    {0x1f18, 0x1f1d, 1, -8},      {0x1f28, 0x1f2f, 1, -8},
    {0x1f38, 0x1f3f, 1, -8},      {0x1f48, 0x1f4d, 1, -8},
    {0x1f59, 0x1f5f, 2, -8},      {0x1f68, 0x1f6f, 1, -8},
    {0x1f88, 0x1f8f, 1, -8},      {0x1f98, 0x1f9f, 1, -8},
    {0x1fa8, 0x1faf, 1, -8},      {0x1fb8, 0x1fb9, 1, -8},
    {0x1fba, 0x1fbb, 1, -74},     {0x1fbc, 0x1fbc, -1, -9},
    {0x1fc8, 0x1fcb, 1, -86},     {0x1fcc, 0x1fcc, -1, -9},
    {0x1fd8, 0x1fd9, 1, -8},      {0x1fda, 0x1fdb, 1, -100},
    {0x1fe8, 0x1fe9, 1, -8},      {0x1fea, 0x1feb, 1, -112},
    {0x1fec, 0x1fec, -1, -7},     {0x1ff8, 0x1ff9, 1, -128},
    {0x1ffa, 0x1ffb, 1, -126},    {0x1ffc, 0x1ffc, -1, -9},
    {0x2126, 0x2126, -1, -7517},  {0x212a, 0x212a, -1, -8383},
    {0x212b, 0x212b, -1, -8262},  {0x2132, 0x2132, -1, 28},
    {0x2160, 0x216f, 1, 16},      {0x2183, 0x2183, -1, 1},
    {0x24b6, 0x24cf, 1, 26},      {0x2c00, 0x2c2e, 1, 48},
    {0x2c60, 0x2c60, -1, 1},      {0x2c62, 0x2c62, -1, -10743},
    {0x2c63, 0x2c63, -1, -3814},  {0x2c64, 0x2c64, -1, -10727},
    {0x2c67, 0x2c6b, 2, 1},       {0x2c6d, 0x2c6d, -1, -10780},
    {0x2c6e, 0x2c6e, -1, -10749}, {0x2c6f, 0x2c6f, -1, -10783},
    {0x2c70, 0x2c70, -1, -10782}, {0x2c72, 0x2c75, 3, 1},
    {0x2c7e, 0x2c7f, 1, -10815},  {0x2c80, 0x2ce2, 2, 1},
    {0x2ceb, 0x2ced, 2, 1},       {0x2cf2, 0xa640, 31054, 1},
    {0xa642, 0xa66c, 2, 1},       {0xa680, 0xa69a, 2, 1},
    {0xa722, 0xa72e, 2, 1},       {0xa732, 0xa76e, 2, 1},
    {0xa779, 0xa77b, 2, 1},       {0xa77d, 0xa77d, -1, -35332},
    {0xa77e, 0xa786, 2, 1},       {0xa78b, 0xa78b, -1, 1},
    {0xa78d, 0xa78d, -1, -42280}, {0xa790, 0xa792, 2, 1},
    {0xa796, 0xa7a8, 2, 1},       {0xa7aa, 0xa7aa, -1, -42308},
    {0xa7ab, 0xa7ab, -1, -42319}, {0xa7ac, 0xa7ac, -1, -42315},
    {0xa7ad, 0xa7ad, -1, -42305}, {0xa7ae, 0xa7ae, -1, -42308},
    {0xa7b0, 0xa7b0, -1, -42258}, {0xa7b1, 0xa7b1, -1, -42282},
    {0xa7b2, 0xa7b2, -1, -42261}, {0xa7b3, 0xa7b3, -1, 928},
    {0xa7b4, 0xa7be, 2, 1},       {0xa7c2, 0xa7c2, -1, 1},
    {0xa7c4, 0xa7c4, -1, -48},    {0xa7c5, 0xa7c5, -1, -42307},
    {0xa7c6, 0xa7c6, -1, -35384}, {0xa7c7, 0xa7c9, 2, 1},
    {0xa7f5, 0xa7f5, -1, 1},      {0xff21, 0xff3a, 1, 32},
    {0x10400, 0x10427, 1, 40},    {0x104b0, 0x104d3, 1, 40},
    {0x10c80, 0x10cb2, 1, 64},    {0x118a0, 0x118bf, 1, 32},
    {0x16e40, 0x16e5f, 1, 32},    {0x1e900, 0x1e921, 1, 34}};

static convertStruct toUpper[] = {
    {0x61, 0x7a, 1, -32},         {0xb5, 0xb5, -1, 743},
    {0xe0, 0xf6, 1, -32},         {0xf8, 0xfe, 1, -32},
    {0xff, 0xff, -1, 121},        {0x101, 0x12f, 2, -1},
    {0x131, 0x131, -1, -232},     {0x133, 0x137, 2, -1},
    {0x13a, 0x148, 2, -1},        {0x14b, 0x177, 2, -1},
    {0x17a, 0x17e, 2, -1},        {0x17f, 0x17f, -1, -300},
    {0x180, 0x180, -1, 195},      {0x183, 0x185, 2, -1},
    {0x188, 0x18c, 4, -1},        {0x192, 0x192, -1, -1},
    {0x195, 0x195, -1, 97},       {0x199, 0x199, -1, -1},
    {0x19a, 0x19a, -1, 163},      {0x19e, 0x19e, -1, 130},
    {0x1a1, 0x1a5, 2, -1},        {0x1a8, 0x1ad, 5, -1},
    {0x1b0, 0x1b4, 4, -1},        {0x1b6, 0x1b9, 3, -1},
    {0x1bd, 0x1bd, -1, -1},       {0x1bf, 0x1bf, -1, 56},
    {0x1c5, 0x1c5, -1, -1},       {0x1c6, 0x1c6, -1, -2},
    {0x1c8, 0x1c8, -1, -1},       {0x1c9, 0x1c9, -1, -2},
    {0x1cb, 0x1cb, -1, -1},       {0x1cc, 0x1cc, -1, -2},
    {0x1ce, 0x1dc, 2, -1},        {0x1dd, 0x1dd, -1, -79},
    {0x1df, 0x1ef, 2, -1},        {0x1f2, 0x1f2, -1, -1},
    {0x1f3, 0x1f3, -1, -2},       {0x1f5, 0x1f9, 4, -1},
    {0x1fb, 0x21f, 2, -1},        {0x223, 0x233, 2, -1},
    {0x23c, 0x23c, -1, -1},       {0x23f, 0x240, 1, 10815},
    {0x242, 0x247, 5, -1},        {0x249, 0x24f, 2, -1},
    {0x250, 0x250, -1, 10783},    {0x251, 0x251, -1, 10780},
    {0x252, 0x252, -1, 10782},    {0x253, 0x253, -1, -210},
    {0x254, 0x254, -1, -206},     {0x256, 0x257, 1, -205},
    {0x259, 0x259, -1, -202},     {0x25b, 0x25b, -1, -203},
    {0x25c, 0x25c, -1, 42319},    {0x260, 0x260, -1, -205},
    {0x261, 0x261, -1, 42315},    {0x263, 0x263, -1, -207},
    {0x265, 0x265, -1, 42280},    {0x266, 0x266, -1, 42308},
    {0x268, 0x268, -1, -209},     {0x269, 0x269, -1, -211},
    {0x26a, 0x26a, -1, 42308},    {0x26b, 0x26b, -1, 10743},
    {0x26c, 0x26c, -1, 42305},    {0x26f, 0x26f, -1, -211},
    {0x271, 0x271, -1, 10749},    {0x272, 0x272, -1, -213},
    {0x275, 0x275, -1, -214},     {0x27d, 0x27d, -1, 10727},
    {0x280, 0x280, -1, -218},     {0x282, 0x282, -1, 42307},
    {0x283, 0x283, -1, -218},     {0x287, 0x287, -1, 42282},
    {0x288, 0x288, -1, -218},     {0x289, 0x289, -1, -69},
    {0x28a, 0x28b, 1, -217},      {0x28c, 0x28c, -1, -71},
    {0x292, 0x292, -1, -219},     {0x29d, 0x29d, -1, 42261},
    {0x29e, 0x29e, -1, 42258},    {0x345, 0x345, -1, 84},
    {0x371, 0x373, 2, -1},        {0x377, 0x377, -1, -1},
    {0x37b, 0x37d, 1, 130},       {0x3ac, 0x3ac, -1, -38},
    {0x3ad, 0x3af, 1, -37},       {0x3b1, 0x3c1, 1, -32},
    {0x3c2, 0x3c2, -1, -31},      {0x3c3, 0x3cb, 1, -32},
    {0x3cc, 0x3cc, -1, -64},      {0x3cd, 0x3ce, 1, -63},
    {0x3d0, 0x3d0, -1, -62},      {0x3d1, 0x3d1, -1, -57},
    {0x3d5, 0x3d5, -1, -47},      {0x3d6, 0x3d6, -1, -54},
    {0x3d7, 0x3d7, -1, -8},       {0x3d9, 0x3ef, 2, -1},
    {0x3f0, 0x3f0, -1, -86},      {0x3f1, 0x3f1, -1, -80},
    {0x3f2, 0x3f2, -1, 7},        {0x3f3, 0x3f3, -1, -116},
    {0x3f5, 0x3f5, -1, -96},      {0x3f8, 0x3fb, 3, -1},
    {0x430, 0x44f, 1, -32},       {0x450, 0x45f, 1, -80},
    {0x461, 0x481, 2, -1},        {0x48b, 0x4bf, 2, -1},
    {0x4c2, 0x4ce, 2, -1},        {0x4cf, 0x4cf, -1, -15},
    {0x4d1, 0x52f, 2, -1},        {0x561, 0x586, 1, -48},
    {0x10d0, 0x10fa, 1, 3008},    {0x10fd, 0x10ff, 1, 3008},
    {0x13f8, 0x13fd, 1, -8},      {0x1c80, 0x1c80, -1, -6254},
    {0x1c81, 0x1c81, -1, -6253},  {0x1c82, 0x1c82, -1, -6244},
    {0x1c83, 0x1c84, 1, -6242},   {0x1c85, 0x1c85, -1, -6243},
    {0x1c86, 0x1c86, -1, -6236},  {0x1c87, 0x1c87, -1, -6181},
    {0x1c88, 0x1c88, -1, 35266},  {0x1d79, 0x1d79, -1, 35332},
    {0x1d7d, 0x1d7d, -1, 3814},   {0x1d8e, 0x1d8e, -1, 35384},
    {0x1e01, 0x1e95, 2, -1},      {0x1e9b, 0x1e9b, -1, -59},
    {0x1ea1, 0x1eff, 2, -1},      {0x1f00, 0x1f07, 1, 8},
    {0x1f10, 0x1f15, 1, 8},       {0x1f20, 0x1f27, 1, 8},
    {0x1f30, 0x1f37, 1, 8},       {0x1f40, 0x1f45, 1, 8},
    {0x1f51, 0x1f57, 2, 8},       {0x1f60, 0x1f67, 1, 8},
    {0x1f70, 0x1f71, 1, 74},      {0x1f72, 0x1f75, 1, 86},
    {0x1f76, 0x1f77, 1, 100},     {0x1f78, 0x1f79, 1, 128},
    {0x1f7a, 0x1f7b, 1, 112},     {0x1f7c, 0x1f7d, 1, 126},
    {0x1f80, 0x1f87, 1, 8},       {0x1f90, 0x1f97, 1, 8},
    {0x1fa0, 0x1fa7, 1, 8},       {0x1fb0, 0x1fb1, 1, 8},
    {0x1fb3, 0x1fb3, -1, 9},      {0x1fbe, 0x1fbe, -1, -7205},
    {0x1fc3, 0x1fc3, -1, 9},      {0x1fd0, 0x1fd1, 1, 8},
    {0x1fe0, 0x1fe1, 1, 8},       {0x1fe5, 0x1fe5, -1, 7},
    {0x1ff3, 0x1ff3, -1, 9},      {0x214e, 0x214e, -1, -28},
    {0x2170, 0x217f, 1, -16},     {0x2184, 0x2184, -1, -1},
    {0x24d0, 0x24e9, 1, -26},     {0x2c30, 0x2c5e, 1, -48},
    {0x2c61, 0x2c61, -1, -1},     {0x2c65, 0x2c65, -1, -10795},
    {0x2c66, 0x2c66, -1, -10792}, {0x2c68, 0x2c6c, 2, -1},
    {0x2c73, 0x2c76, 3, -1},      {0x2c81, 0x2ce3, 2, -1},
    {0x2cec, 0x2cee, 2, -1},      {0x2cf3, 0x2cf3, -1, -1},
    {0x2d00, 0x2d25, 1, -7264},   {0x2d27, 0x2d2d, 6, -7264},
    {0xa641, 0xa66d, 2, -1},      {0xa681, 0xa69b, 2, -1},
    {0xa723, 0xa72f, 2, -1},      {0xa733, 0xa76f, 2, -1},
    {0xa77a, 0xa77c, 2, -1},      {0xa77f, 0xa787, 2, -1},
    {0xa78c, 0xa791, 5, -1},      {0xa793, 0xa793, -1, -1},
    {0xa794, 0xa794, -1, 48},     {0xa797, 0xa7a9, 2, -1},
    {0xa7b5, 0xa7bf, 2, -1},      {0xa7c3, 0xa7c8, 5, -1},
    {0xa7ca, 0xa7f6, 44, -1},     {0xab53, 0xab53, -1, -928},
    {0xab70, 0xabbf, 1, -38864},  {0xff41, 0xff5a, 1, -32},
    {0x10428, 0x1044f, 1, -40},   {0x104d8, 0x104fb, 1, -40},
    {0x10cc0, 0x10cf2, 1, -64},   {0x118c0, 0x118df, 1, -32},
    {0x16e60, 0x16e7f, 1, -32},   {0x1e922, 0x1e943, 1, -34}};

/*
 * Get byte length of character at "*p".  Returns zero when "*p" is NUL.
 * Used for mb_ptr2len() when 'encoding' latin.
 */
int latin_ptr2len(char_u *p) { return *p == NUL ? 0 : 1; }

/*
 * Lookup table to quickly get the length in bytes of a UTF-8 character from
 * the first byte of a UTF-8 string.
 * Bytes which are illegal when used as the first byte have a 1.
 * The NUL byte has length 1.
 */
static char utf8len_tab[256] = {
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
    2, 2, 2, 2, 2, 2, 2, 2, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3,
    4, 4, 4, 4, 4, 4, 4, 4, 5, 5, 5, 5, 6, 6, 1, 1,
};

/*
 * Like utf8len_tab above, but using a zero for illegal lead bytes.
 */
static char utf8len_tab_zero[256] = {
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
    2, 2, 2, 2, 2, 2, 2, 2, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3,
    4, 4, 4, 4, 4, 4, 4, 4, 5, 5, 5, 5, 6, 6, 0, 0,
};

/*
 * Get the length of a UTF-8 byte sequence, not including any following
 * composing characters.
 * Returns 0 for "".
 * Returns 1 for an illegal byte sequence.
 */
int utf_ptr2len(char_u *p) {
  int len;
  int i;

  if (*p == NUL)
    return 0;
  len = utf8len_tab[*p];
  for (i = 1; i < len; ++i)
    if ((p[i] & 0xc0) != 0x80)
      return 1;
  return len;
}

/*
 * Convert a UTF-8 byte sequence to a character number.
 * If the sequence is illegal or truncated by a NUL the first byte is
 * returned.
 * For an overlong sequence this may return zero.
 * Does not include composing characters, of course.
 */
int utf_ptr2char(char_u *p) {
  int len;

  if (p[0] < 0x80) // be quick for ASCII
    return p[0];

  len = utf8len_tab_zero[p[0]];
  if (len > 1 && (p[1] & 0xc0) == 0x80) {
    if (len == 2)
      return ((p[0] & 0x1f) << 6) + (p[1] & 0x3f);
    if ((p[2] & 0xc0) == 0x80) {
      if (len == 3)
        return ((p[0] & 0x0f) << 12) + ((p[1] & 0x3f) << 6) + (p[2] & 0x3f);
      if ((p[3] & 0xc0) == 0x80) {
        if (len == 4)
          return ((p[0] & 0x07) << 18) + ((p[1] & 0x3f) << 12) +
                 ((p[2] & 0x3f) << 6) + (p[3] & 0x3f);
        if ((p[4] & 0xc0) == 0x80) {
          if (len == 5)
            return ((p[0] & 0x03) << 24) + ((p[1] & 0x3f) << 18) +
                   ((p[2] & 0x3f) << 12) + ((p[3] & 0x3f) << 6) + (p[4] & 0x3f);
          if ((p[5] & 0xc0) == 0x80 && len == 6)
            return ((p[0] & 0x01) << 30) + ((p[1] & 0x3f) << 24) +
                   ((p[2] & 0x3f) << 18) + ((p[3] & 0x3f) << 12) +
                   ((p[4] & 0x3f) << 6) + (p[5] & 0x3f);
        }
      }
    }
  }
  // Illegal value, just return the first byte
  return p[0];
}

/*
 * Return the character length of "str".  Each multi-byte character (with
 * following composing characters) counts as one.
 */
int mb_charlen(char_u *str) {
  char_u *p = str;
  int count;

  if (p == NULL)
    return 0;

  for (count = 0; *p != NUL; count++)
    p += (*utf_ptr2len)(p);

  return count;
}

/*
 * Generic conversion function for case operations.
 * Return the converted equivalent of "a", which is a UCS-4 character.  Use
 * the given conversion "table".  Uses binary search on "table".
 */
static int utf_convert(int a, convertStruct table[], int tableSize) {
  int start, mid, end; // indices into table
  int entries = tableSize / sizeof(convertStruct);

  start = 0;
  end = entries;
  while (start < end) {
    // need to search further
    mid = (end + start) / 2;
    if (table[mid].rangeEnd < a)
      start = mid + 1;
    else
      end = mid;
  }
  if (start < entries && table[start].rangeStart <= a &&
      a <= table[start].rangeEnd &&
      (a - table[start].rangeStart) % table[start].step == 0)
    return (a + table[start].offset);
  else
    return a;
}

/*
 * Return the lower-case equivalent of "a", which is a UCS-4 character.  Use
 * simple case folding.
 */
int utf_tolower(int a) {
  // If 'casemap' contains "keepascii" use ASCII style tolower().
  if (a < 128)
    return TOLOWER_ASC(a);

  // For any other characters use the above mapping table.
  return utf_convert(a, toLower, (int)sizeof(toLower));
}

/*
 * Return the upper-case equivalent of "a", which is a UCS-4 character.  Use
 * simple case folding.
 */
int utf_toupper(int a) {
  // If 'casemap' contains "keepascii" use ASCII style toupper().
  if (a < 128)
    return TOUPPER_ASC(a);
  // For any other characters use the above mapping table.
  return utf_convert(a, toUpper, (int)sizeof(toUpper));
}

int utf_islower(int a) {
  // German sharp s is lower case but has no upper case equivalent.
  return (utf_toupper(a) != a) || a == 0xdf;
}

int utf_isupper(int a) { return (utf_tolower(a) != a); }
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

__attribute__((export_name("fuzzyMatch"))) int
fuzzyMatch(char_u *str, char_u *pattern, int_u *matches, int matchseq) {
  int score = 0;
  int res = 0;
  res = fuzzy_match(str, pattern, matchseq, &score, matches, MAX_FUZZY_MATCHES);
  if (!res)
    return 0;
  return score;
}
