#ifndef APP_LOGIC_H
#define APP_LOGIC_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

bool AppLogic_StringEndsWithIgnoreCase(const char *text, const char *suffix);
void AppLogic_BuildLrcFileName(const char *mp3_name, char *lrc_name, size_t lrc_name_size);
bool AppLogic_ParseLyricLine(const char *line, uint32_t *timestamp_ms, const char **text);
bool AppLogic_ParseDuration(const char *value, uint32_t *duration_ms);
bool AppLogic_ParseMetadataLine(const char *line,
                                char *artist,
                                size_t artist_size,
                                char *title,
                                size_t title_size,
                                uint32_t *duration_ms);
void AppLogic_CopyDisplayText(char *destination, size_t destination_size, const char *source);
void AppLogic_SortTrackNames(char *track_names, uint32_t track_count, size_t track_name_length);

#endif
