#include "app_logic.h"

#include <string.h>

static bool SkipInlineTimestampTag(const char **cursor);
static bool ParseTimestampTag(const char **cursor, uint32_t *timestamp_ms);
static size_t Utf8_GlyphLength(const char *text);

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

  strncpy(lrc_name, mp3_name, lrc_name_size - 1U);
  lrc_name[lrc_name_size - 1U] = '\0';

  length = strlen(lrc_name);
  if (length >= 4U && AppLogic_StringEndsWithIgnoreCase(lrc_name, ".mp3"))
  {
    lrc_name[length - 4U] = '\0';
  }

  strncat(lrc_name, ".lrc", lrc_name_size - strlen(lrc_name) - 1U);
}

bool AppLogic_ParseLyricLine(const char *line, uint32_t *timestamp_ms, const char **text)
{
  uint32_t ignored_timestamp;
  uint32_t minutes = 0U;
  uint32_t seconds = 0U;
  uint32_t fraction = 0U;
  uint32_t fraction_digits = 0U;
  const char *p = line;

  if (p == NULL || timestamp_ms == NULL || text == NULL)
  {
    return false;
  }

  if ((uint8_t)p[0] == 0xEFU && (uint8_t)p[1] == 0xBBU && (uint8_t)p[2] == 0xBFU)
  {
    p += 3;
  }

  if (*p == '"')
  {
    p++;
  }

  if (*p != '[')
  {
    return false;
  }

  p++;
  while (*p >= '0' && *p <= '9')
  {
    minutes = (minutes * 10U) + (uint32_t)(*p - '0');
    p++;
  }

  if (*p != ':')
  {
    return false;
  }
  p++;

  while (*p >= '0' && *p <= '9')
  {
    seconds = (seconds * 10U) + (uint32_t)(*p - '0');
    p++;
  }

  if (*p == '.')
  {
    p++;
    while (*p >= '0' && *p <= '9' && fraction_digits < 3U)
    {
      fraction = (fraction * 10U) + (uint32_t)(*p - '0');
      fraction_digits++;
      p++;
    }
  }

  if (*p != ']')
  {
    return false;
  }
  p++;

  while (fraction_digits < 3U)
  {
    fraction *= 10U;
    fraction_digits++;
  }

  *timestamp_ms = ((minutes * 60U) + seconds) * 1000U + fraction;

  while (ParseTimestampTag(&p, &ignored_timestamp))
  {
  }

  while (*p == ' ')
  {
    p++;
  }

  if (*p == '"' && p[1] == '\0')
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

  while (*p >= '0' && *p <= '9')
  {
    minutes = (minutes * 10U) + (uint32_t)(*p - '0');
    p++;
  }

  if (*p != ':')
  {
    return false;
  }
  p++;

  while (*p >= '0' && *p <= '9')
  {
    seconds = (seconds * 10U) + (uint32_t)(*p - '0');
    p++;
  }

  *duration_ms = ((minutes * 60U) + seconds) * 1000U;
  return true;
}

bool AppLogic_ParseMetadataLine(const char *line,
                                char *artist,
                                size_t artist_size,
                                char *title,
                                size_t title_size,
                                uint32_t *duration_ms)
{
  const char *p = line;
  const char *value;
  char *target = NULL;
  size_t target_size = 0U;
  char metadata_text[64];
  size_t metadata_length = 0U;
  bool is_length = false;

  if (p == NULL)
  {
    return false;
  }

  if ((uint8_t)p[0] == 0xEFU && (uint8_t)p[1] == 0xBBU && (uint8_t)p[2] == 0xBFU)
  {
    p += 3;
  }

  if (*p == '"')
  {
    p++;
  }

  if (p[0] != '[')
  {
    return false;
  }

  if (p[1] >= '0' && p[1] <= '9')
  {
    return false;
  }

  if (p[1] == 'a' && p[2] == 'r' && p[3] == ':')
  {
    target = artist;
    target_size = artist_size;
  }
  else if (p[1] == 't' && p[2] == 'i' && p[3] == ':')
  {
    target = title;
    target_size = title_size;
  }
  else if (strncmp(&p[1], "length", 6U) == 0 && p[7] == ':')
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
  while (*value == ' ')
  {
    value++;
  }

  while (value[metadata_length] != '\0' && value[metadata_length] != ']' && metadata_length < (sizeof(metadata_text) - 1U))
  {
    metadata_text[metadata_length] = value[metadata_length];
    metadata_length++;
  }
  metadata_text[metadata_length] = '\0';

  if (is_length)
  {
    (void)AppLogic_ParseDuration(metadata_text, duration_ms);
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

    if (SkipInlineTimestampTag(&source))
    {
      continue;
    }

    if (byte < 0x80U)
    {
      destination[write_index++] = (char)byte;
      source++;
    }
    else if ((byte == 0xC2U || byte == 0xC3U) && source[1] != '\0')
    {
      glyph_length = Utf8_GlyphLength(source);
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
  char temp[64];

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
        strncpy(temp, left, sizeof(temp));
        strncpy(left, right, track_name_length);
        strncpy(right, temp, track_name_length);
      }
    }
  }
}

static bool SkipInlineTimestampTag(const char **cursor)
{
  const char *p = *cursor;

  if (p == NULL || *p != '<')
  {
    return false;
  }

  p++;
  if (*p < '0' || *p > '9')
  {
    return false;
  }

  while (*p >= '0' && *p <= '9')
  {
    p++;
  }

  if (*p != ':')
  {
    return false;
  }
  p++;

  if (*p < '0' || *p > '9')
  {
    return false;
  }

  while (*p >= '0' && *p <= '9')
  {
    p++;
  }

  if (*p == '.')
  {
    p++;
    while (*p >= '0' && *p <= '9')
    {
      p++;
    }
  }

  if (*p != '>')
  {
    return false;
  }

  *cursor = p + 1;
  return true;
}

static bool ParseTimestampTag(const char **cursor, uint32_t *timestamp_ms)
{
  uint32_t minutes = 0U;
  uint32_t seconds = 0U;
  uint32_t fraction = 0U;
  uint32_t fraction_digits = 0U;
  const char *p = *cursor;

  if (p == NULL || *p != '[' || timestamp_ms == NULL)
  {
    return false;
  }

  p++;
  if (*p < '0' || *p > '9')
  {
    return false;
  }

  while (*p >= '0' && *p <= '9')
  {
    minutes = (minutes * 10U) + (uint32_t)(*p - '0');
    p++;
  }

  if (*p != ':')
  {
    return false;
  }
  p++;

  if (*p < '0' || *p > '9')
  {
    return false;
  }

  while (*p >= '0' && *p <= '9')
  {
    seconds = (seconds * 10U) + (uint32_t)(*p - '0');
    p++;
  }

  if (*p == '.')
  {
    p++;
    while (*p >= '0' && *p <= '9' && fraction_digits < 3U)
    {
      fraction = (fraction * 10U) + (uint32_t)(*p - '0');
      fraction_digits++;
      p++;
    }

    while (*p >= '0' && *p <= '9')
    {
      p++;
    }
  }

  if (*p != ']')
  {
    return false;
  }
  p++;

  while (fraction_digits < 3U)
  {
    fraction *= 10U;
    fraction_digits++;
  }

  *timestamp_ms = ((minutes * 60U) + seconds) * 1000U + fraction;
  *cursor = p;
  return true;
}

static size_t Utf8_GlyphLength(const char *text)
{
  uint8_t byte = (uint8_t)text[0];

  if (byte < 0x80U)
  {
    return 1U;
  }

  if ((byte & 0xE0U) == 0xC0U && text[1] != '\0')
  {
    return 2U;
  }

  return 1U;
}
