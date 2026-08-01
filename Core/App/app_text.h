#ifndef APP_TEXT_H
#define APP_TEXT_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

bool AppText_IsAsciiDigit(char ch);
bool AppText_HasUtf8Bom(const char *text);
bool AppText_ParseUnsigned(const char **cursor, uint32_t *value);
bool AppText_ParseTimestampTag(const char **cursor, uint32_t *timestamp_ms);
bool AppText_SkipInlineTimestampTag(const char **cursor);
size_t AppText_Utf8GlyphLength(const char *text);
size_t AppText_Utf8GlyphCount(const char *text);
void AppText_CopyTruncated(char *destination, size_t destination_size, const char *source);

#endif
