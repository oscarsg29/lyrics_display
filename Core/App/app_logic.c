#include "app_logic.h"

#include "app_text.h"

#include <string.h>

#define ASCII_DOUBLE_QUOTE '"'
#define ASCII_SPACE ' '
#define ASCII_METADATA_OPEN '['
#define ASCII_METADATA_CLOSE ']'
#define ASCII_TIME_SEPARATOR ':'
#define ASCII_NUL '\0'
#define SECONDS_PER_MINUTE 60U
#define MILLISECONDS_PER_SECOND 1000U
#define UTF8_ASCII_LIMIT 0x80U
#define UTF8_LATIN1_PREFIX_0 0xC2U
#define UTF8_LATIN1_PREFIX_1 0xC3U

bool AppLogic_StringEndsWithIgnoreCase(const char *text, const char *suffix)
{
  size_t text_length;
  size_t suffix_length;

  if (text == NULL || suffix == NULL)
  {
    return false;
  }

  text_length = strlen(text);
  suffix_length = strlen(suffix);

  if (suffix_length > text_length)
  {
    return false;
  }

  text += text_length - suffix_length;
  for (size_t i = 0U; i < suffix_length; i++)
  {
    char a = text[i];
    char b = suffix[i];

    if (a >= 'A' && a <= 'Z')
    {
      a = (char)(a + ('a' - 'A'));
    }
    if (b >= 'A' && b <= 'Z')
    {
      b = (char)(b + ('a' - 'A'));
    }
    if (a != b)
    {
      return false;
    }
  }

  return true;
}

void AppLogic_BuildLrcFileName(const char *mp3_name, char *lrc_name, size_t lrc_name_size)
{
  size_t length;

  if (lrc_name_size == 0U || lrc_name == NULL)
  {
    return;
  }

  if (mp3_name == NULL)
  {
    mp3_name = "";
  }

  AppText_CopyTruncated(lrc_name, lrc_name_size, mp3_name);

  length = strlen(lrc_name);
  if (length >= strlen(APP_MP3_EXTENSION) && AppLogic_StringEndsWithIgnoreCase(lrc_name, APP_MP3_EXTENSION))
  {
    lrc_name[length - strlen(APP_MP3_EXTENSION)] = ASCII_NUL;
  }

  strncat(lrc_name, APP_LRC_EXTENSION, lrc_name_size - strlen(lrc_name) - 1U);
}

bool AppLogic_ParseLyricLine(const char *line, uint32_t *timestamp_ms, const char **text)
{
  uint32_t ignored_timestamp;
  const char *p = line;

  if (p == NULL || timestamp_ms == NULL || text == NULL)
  {
    return false;
  }

  if (AppText_HasUtf8Bom(p))
  {
    p += 3;
  }

  if (*p == ASCII_DOUBLE_QUOTE)
  {
    p++;
  }

  if (!AppText_ParseTimestampTag(&p, timestamp_ms))
  {
    return false;
  }

  while (AppText_ParseTimestampTag(&p, &ignored_timestamp))
  {
  }

  while (*p == ASCII_SPACE)
  {
    p++;
  }

  if (*p == ASCII_DOUBLE_QUOTE && p[1] == ASCII_NUL)
  {
    p++;
  }

  *text = p;
  return true;
}

bool AppLogic_ParseDuration(const char *value, uint32_t *duration_ms)
{
  uint32_t minutes = 0U;
  uint32_t seconds = 0U;
  const char *p = value;

  if (p == NULL || duration_ms == NULL)
  {
    return false;
  }

  if (!AppText_ParseUnsigned(&p, &minutes) || *p != ASCII_TIME_SEPARATOR)
  {
    return false;
  }
  p++;

  if (!AppText_ParseUnsigned(&p, &seconds))
  {
    return false;
  }

  *duration_ms = ((minutes * SECONDS_PER_MINUTE) + seconds) * MILLISECONDS_PER_SECOND;
  return true;
}

void AppLogic_ClearMetadata(AppLyricMetadata *metadata)
{
  if (metadata == NULL)
  {
    return;
  }

  metadata->artist[0] = ASCII_NUL;
  metadata->title[0] = ASCII_NUL;
  metadata->duration_ms = 0U;
}

bool AppLogic_ParseMetadataLine(const char *line, AppLyricMetadata *metadata)
{
  const char *p = line;
  const char *value;
  char *target = NULL;
  size_t target_size = 0U;
  char metadata_text[APP_METADATA_TEXT_LENGTH];
  size_t metadata_length = 0U;
  bool is_length = false;

  if (p == NULL)
  {
    return false;
  }

  if (AppText_HasUtf8Bom(p))
  {
    p += 3;
  }

  if (*p == ASCII_DOUBLE_QUOTE)
  {
    p++;
  }

  if (p[0] != ASCII_METADATA_OPEN)
  {
    return false;
  }

  if (AppText_IsAsciiDigit(p[1]))
  {
    return false;
  }

  if (strncmp(p, "[ar:", 4U) == 0)
  {
    target = metadata != NULL ? metadata->artist : NULL;
    target_size = metadata != NULL ? sizeof(metadata->artist) : 0U;
  }
  else if (strncmp(p, "[ti:", 4U) == 0)
  {
    target = metadata != NULL ? metadata->title : NULL;
    target_size = metadata != NULL ? sizeof(metadata->title) : 0U;
  }
  else if (strncmp(p, "[length:", 8U) == 0)
  {
    is_length = true;
    value = &p[8];
  }
  else
  {
    return true;
  }

  if (!is_length)
  {
    value = &p[4];
  }
  while (*value == ASCII_SPACE)
  {
    value++;
  }

  while (value[metadata_length] != ASCII_NUL && value[metadata_length] != ASCII_METADATA_CLOSE && metadata_length < (sizeof(metadata_text) - 1U))
  {
    metadata_text[metadata_length] = value[metadata_length];
    metadata_length++;
  }
  metadata_text[metadata_length] = ASCII_NUL;

  if (is_length)
  {
    if (metadata != NULL)
    {
      (void)AppLogic_ParseDuration(metadata_text, &metadata->duration_ms);
    }
  }
  else
  {
    AppLogic_CopyDisplayText(target, target_size, metadata_text);
  }
  return true;
}

void AppLogic_CopyDisplayText(char *destination, size_t destination_size, const char *source)
{
  size_t write_index = 0U;

  if (destination_size == 0U || destination == NULL)
  {
    return;
  }

  if (source == NULL)
  {
    destination[0] = '\0';
    return;
  }

  while (*source != '\0' && write_index < (destination_size - 1U))
  {
    uint8_t byte = (uint8_t)*source;
    size_t glyph_length;

    if (AppText_SkipInlineTimestampTag(&source))
    {
      continue;
    }

    if (byte < UTF8_ASCII_LIMIT)
    {
      destination[write_index++] = (char)byte;
      source++;
    }
    else if ((byte == UTF8_LATIN1_PREFIX_0 || byte == UTF8_LATIN1_PREFIX_1) && source[1] != ASCII_NUL)
    {
      glyph_length = AppText_Utf8GlyphLength(source);
      if ((write_index + glyph_length) >= destination_size)
      {
        break;
      }
      for (size_t i = 0U; i < glyph_length; i++)
      {
        destination[write_index++] = source[i];
      }
      source += glyph_length;
    }
    else
    {
      destination[write_index++] = '?';
      source++;
    }
  }

  destination[write_index] = '\0';
}

void AppLogic_SortTrackNames(char *track_names, uint32_t track_count, size_t track_name_length)
{
  char temp[APP_TRACK_NAME_LENGTH];

  if (track_names == NULL || track_name_length == 0U || track_name_length > sizeof(temp))
  {
    return;
  }

  for (uint32_t i = 0U; i < track_count; i++)
  {
    for (uint32_t j = i + 1U; j < track_count; j++)
    {
      char *left = &track_names[i * track_name_length];
      char *right = &track_names[j * track_name_length];

      if (strcmp(left, right) > 0)
      {
        AppText_CopyTruncated(temp, sizeof(temp), left);
        AppText_CopyTruncated(left, track_name_length, right);
        AppText_CopyTruncated(right, track_name_length, temp);
      }
    }
  }
}
