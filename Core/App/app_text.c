#include "app_text.h"

#include <string.h>

#define ASCII_ZERO '0'
#define ASCII_NINE '9'
#define ASCII_SPACE ' '
#define ASCII_TIMESTAMP_OPEN '['
#define ASCII_TIMESTAMP_CLOSE ']'
#define ASCII_INLINE_TIMESTAMP_OPEN '<'
#define ASCII_INLINE_TIMESTAMP_CLOSE '>'
#define ASCII_TIME_SEPARATOR ':'
#define ASCII_FRACTION_SEPARATOR '.'
#define ASCII_NUL '\0'
#define DECIMAL_BASE 10U
#define SECONDS_PER_MINUTE 60U
#define MILLISECONDS_PER_SECOND 1000U
#define TIMESTAMP_FRACTION_DIGITS 3U
#define UTF8_BOM_0 0xEFU
#define UTF8_BOM_1 0xBBU
#define UTF8_BOM_2 0xBFU
#define UTF8_ASCII_LIMIT 0x80U
#define UTF8_TWO_BYTE_PREFIX_MASK 0xE0U
#define UTF8_TWO_BYTE_PREFIX 0xC0U

static bool ParseTimeFields(const char **cursor, uint32_t *timestamp_ms, char close_tag);

bool AppText_IsAsciiDigit(char ch)
{
  return ch >= ASCII_ZERO && ch <= ASCII_NINE;
}

bool AppText_HasUtf8Bom(const char *text)
{
  return text != NULL
      && (uint8_t)text[0] == UTF8_BOM_0
      && text[1] != ASCII_NUL
      && (uint8_t)text[1] == UTF8_BOM_1
      && text[2] != ASCII_NUL
      && (uint8_t)text[2] == UTF8_BOM_2;
}

bool AppText_ParseUnsigned(const char **cursor, uint32_t *value)
{
  const char *p;
  uint32_t parsed = 0U;

  if (cursor == NULL || *cursor == NULL || value == NULL || !AppText_IsAsciiDigit(**cursor))
  {
    return false;
  }

  p = *cursor;
  while (AppText_IsAsciiDigit(*p))
  {
    parsed = (parsed * DECIMAL_BASE) + (uint32_t)(*p - ASCII_ZERO);
    p++;
  }

  *cursor = p;
  *value = parsed;
  return true;
}

bool AppText_ParseTimestampTag(const char **cursor, uint32_t *timestamp_ms)
{
  const char *p;

  if (cursor == NULL || *cursor == NULL || **cursor != ASCII_TIMESTAMP_OPEN)
  {
    return false;
  }

  p = *cursor + 1;
  if (!ParseTimeFields(&p, timestamp_ms, ASCII_TIMESTAMP_CLOSE))
  {
    return false;
  }

  *cursor = p;
  return true;
}

bool AppText_SkipInlineTimestampTag(const char **cursor)
{
  const char *p;
  uint32_t ignored_timestamp;

  if (cursor == NULL || *cursor == NULL || **cursor != ASCII_INLINE_TIMESTAMP_OPEN)
  {
    return false;
  }

  p = *cursor + 1;
  if (!ParseTimeFields(&p, &ignored_timestamp, ASCII_INLINE_TIMESTAMP_CLOSE))
  {
    return false;
  }

  *cursor = p;
  return true;
}

size_t AppText_Utf8GlyphLength(const char *text)
{
  uint8_t byte = (uint8_t)text[0];

  if (byte < UTF8_ASCII_LIMIT)
  {
    return 1U;
  }

  if ((byte & UTF8_TWO_BYTE_PREFIX_MASK) == UTF8_TWO_BYTE_PREFIX && text[1] != ASCII_NUL)
  {
    return 2U;
  }

  return 1U;
}

size_t AppText_Utf8GlyphCount(const char *text)
{
  size_t count = 0U;

  while (text != NULL && *text != ASCII_NUL)
  {
    text += AppText_Utf8GlyphLength(text);
    count++;
  }

  return count;
}

void AppText_CopyTruncated(char *destination, size_t destination_size, const char *source)
{
  if (destination == NULL || destination_size == 0U)
  {
    return;
  }

  if (source == NULL)
  {
    destination[0] = ASCII_NUL;
    return;
  }

  strncpy(destination, source, destination_size - 1U);
  destination[destination_size - 1U] = ASCII_NUL;
}

static bool ParseTimeFields(const char **cursor, uint32_t *timestamp_ms, char close_tag)
{
  const char *p;
  uint32_t minutes;
  uint32_t seconds;
  uint32_t fraction = 0U;
  uint32_t fraction_digits = 0U;

  if (cursor == NULL || *cursor == NULL || timestamp_ms == NULL)
  {
    return false;
  }

  p = *cursor;
  if (!AppText_ParseUnsigned(&p, &minutes) || *p != ASCII_TIME_SEPARATOR)
  {
    return false;
  }
  p++;

  if (!AppText_ParseUnsigned(&p, &seconds))
  {
    return false;
  }

  if (*p == ASCII_FRACTION_SEPARATOR)
  {
    p++;
    while (AppText_IsAsciiDigit(*p) && fraction_digits < TIMESTAMP_FRACTION_DIGITS)
    {
      fraction = (fraction * DECIMAL_BASE) + (uint32_t)(*p - ASCII_ZERO);
      fraction_digits++;
      p++;
    }

    while (AppText_IsAsciiDigit(*p))
    {
      p++;
    }
  }

  if (*p != close_tag)
  {
    return false;
  }
  p++;

  while (fraction_digits < TIMESTAMP_FRACTION_DIGITS)
  {
    fraction *= DECIMAL_BASE;
    fraction_digits++;
  }

  *timestamp_ms = ((minutes * SECONDS_PER_MINUTE) + seconds) * MILLISECONDS_PER_SECOND + fraction;
  *cursor = p;
  return true;
}
