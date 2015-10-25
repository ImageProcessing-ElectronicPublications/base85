#include "include/base85.h"

#include <stdbool.h>

/// Ascii85 alphabet.
static const unsigned char g_ascii85_encode[] = {
  '!', '"', '#', '$', '%', '&', '\'', '(',
  ')', '*', '+', ',', '-', '.', '/', '0',
  '1', '2', '3', '4', '5', '6', '7', '8',
  '9', ':', ';', '<', '=', '>', '?', '@',
  'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H',
  'I', 'J', 'K', 'L', 'M', 'N', 'O', 'P',
  'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X',
  'Y', 'Z', '[', '\\', ']', '^', '_', '`',
  'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h',
  'i', 'j', 'k', 'l', 'm', 'n', 'o', 'p',
  'q', 'r', 's', 't', 'u', 
};

/// Returns true if @a c is a white space character.
static bool
base85_whitespace (char c)
{
  switch (c)
  {
  case ' ':
  case '\n':
  case '\r':
  case '\t':
    return true;
  }
  return false;
}

static bool
base85_decode_valid_char (char c)
{
  return (c > 32) && (c < 118);
}

size_t
base85_required_buffer_size (size_t input_size)
{
  size_t s = ((input_size + 3) / 4) * 4;
  return 1 + s + (s / 4);
}

void
base85_encode (const char *b, size_t cb_b, char *out)
{
  unsigned int v;
  while (cb_b)
  {
    v = 0;
    size_t start_cb = cb_b;
    size_t padding = 0;
    for (int c = 24; c >= 0; c -= 8)
    {
      v |= *b++ << c;
      if (!--cb_b)
      {
        padding = 4 - start_cb;
        break;
      }
    }

    if (!v)
    {
      *out++ = 'z';
      continue;
    }

    for (int c = 4; c >= 0; --c)
    {
      out[c] = g_ascii85_encode[v % 85];
      v /= 85;
    }
    out += 5 - padding;
  }
  *out = 0;
}

/// Decodes exactly 5 bytes from @a b and stores 4 bytes in @a out.
/// @return -1 on overflow.
static int
base85_decode_strict (const char *b, char *out)
{
  unsigned int v = 0;
  unsigned char x;
  for (int c = 0; c < 4; ++c)
  {
    x = *b++ - 33;
    v = v * 85 + x;
  }

  x = *b - 33;

  // Check for overflow.
  if ((0xffffffff / 85 < v) || (0xffffffff - x < (v *= 85)))
    return -1;

  v += x;

  for (int c = 24; c >= 0; c -= 8)
    *out++ = (v >> c) & 0xff;

  return 0;
}

int
base85_decode (const char *b, size_t cb_b, char *out, size_t *out_cb)
{
  if (!b || !out_cb)
    return -1;

  char a[5];
  size_t pos = 0;
  size_t cb = 0;
  while (cb_b--)
  {
    char c = *b++;

    // Special case for 'z'.
    if ('z' == c && !pos)
    {
      if (out)
      {
        memset (out, 0, 4);
        out += 4;
      }
      cb += 4;
      continue;
    }

    if (base85_whitespace (c))
      continue;

    if (!base85_decode_valid_char (c))
      return -1;

    a[pos++] = c;
    if (5 == pos)
    {
      if (out)
      {
        if (base85_decode_strict (a, out))
          return -1;
        out += 4;
      }
      cb += 4;
      pos = 0;
    }
  }

  // pos contains the number of items in a.
  if (pos)
  {
    for (int i = pos; i < 5; ++i)
      a[i] = 'u';

    if (out && base85_decode_strict (a, out))
      return -1;

    cb = cb + 4 - (5 - pos);
  }


  *out_cb = cb;
  return 0;
}
