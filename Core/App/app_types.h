#ifndef APP_TYPES_H
#define APP_TYPES_H

#include <stdint.h>

typedef struct
{
  uint32_t value;
} AppTimeMs;

typedef struct
{
  uint8_t value;
} AppAnimationFrame;

typedef struct
{
  uint8_t value;
} AppAnimationStyle;

typedef struct
{
  uint8_t x;
  uint8_t y;
} AppDisplayPoint;

typedef struct
{
  uint8_t width;
  uint8_t height;
} AppDisplaySize;

typedef struct
{
  AppDisplayPoint origin;
  AppDisplaySize size;
} AppDisplayRect;

typedef struct
{
  const char *text;
  AppAnimationFrame frame;
  AppAnimationStyle text_style;
  AppAnimationStyle background_style;
} AppLyricRenderView;

typedef struct
{
  uint8_t first_y;
  uint8_t last_y;
} AppDisplayLineSpan;

#endif
