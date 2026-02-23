#include "fstrcmp.h"

#include <ctype.h>
#include <string.h>

double fstrcmp(const char* left, const char* right)
{
  size_t i;
  size_t left_len;
  size_t right_len;
  size_t min_len;
  size_t same_pos = 0;

  if (!left || !right)
    return 0.0;

  if (left == right || strcmp(left, right) == 0)
    return 1.0;

  left_len = strlen(left);
  right_len = strlen(right);
  min_len = left_len < right_len ? left_len : right_len;

  if (left_len == 0 || right_len == 0)
    return 0.0;

  for (i = 0; i < min_len; ++i)
  {
    unsigned char a = (unsigned char)left[i];
    unsigned char b = (unsigned char)right[i];
    if (tolower(a) == tolower(b))
      ++same_pos;
  }

  return (2.0 * (double)same_pos) / (double)(left_len + right_len);
}
