#ifndef APP_LYRICS_H
#define APP_LYRICS_H

#include <stddef.h>
#include <stdint.h>

#include "app_config.h"
#include "app_logic.h"

typedef struct
{
  uint32_t timestamp_ms;
  char text[APP_LYRIC_TEXT_LENGTH];
} AppLyricLine;

typedef enum
{
  APP_LYRICS_STATUS_OK,
  APP_LYRICS_STATUS_INVALID_PARAMETER,
  APP_LYRICS_STATUS_FILE_NOT_FOUND,
  APP_LYRICS_STATUS_OPEN_FAILED,
  APP_LYRICS_STATUS_READ_FAILED,
  APP_LYRICS_STATUS_NO_TIMED_LINES
} AppLyricsStatus;

typedef struct
{
  AppLyricLine *lines;
  uint32_t max_lines;
  uint32_t count;
  AppLyricMetadata metadata;
} AppLyricsDocument;

void AppLyrics_ClearDocument(AppLyricsDocument *document);
AppLyricsStatus AppLyrics_LoadForTrack(const char *track_name, AppLyricsDocument *document);

#endif
