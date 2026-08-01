#ifndef APP_LOGIC_H
#define APP_LOGIC_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "app_config.h"

typedef struct
{
  char artist[APP_METADATA_TEXT_LENGTH];
  char title[APP_METADATA_TEXT_LENGTH];
  uint32_t duration_ms;
} AppLyricMetadata;

bool AppLogic_StringEndsWithIgnoreCase(const char *text, const char *suffix);
void AppLogic_BuildLrcFileName(const char *mp3_name, char *lrc_name, size_t lrc_name_size);
bool AppLogic_ParseLyricLine(const char *line, uint32_t *timestamp_ms, const char **text);
bool AppLogic_ParseDuration(const char *value, uint32_t *duration_ms);
void AppLogic_ClearMetadata(AppLyricMetadata *metadata);
bool AppLogic_ParseMetadataLine(const char *line, AppLyricMetadata *metadata);
void AppLogic_CopyDisplayText(char *destination, size_t destination_size, const char *source);
void AppLogic_SortTrackNames(char *track_names, uint32_t track_count, size_t track_name_length);

#endif
