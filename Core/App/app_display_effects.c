#include "app_display_effects.h"

#include "ssd1306.h"

#define LYRIC_ANIMATION_COUNT 10U
#define LYRIC_BACKGROUND_ANIMATION_COUNT 6U
#define LYRIC_PARTICLE_COUNT 22U
#define LYRIC_PARTICLE_SPEED_X_SHIFT 8U
#define LYRIC_PARTICLE_SPEED_Y_SHIFT 12U
#define RANDOM_XORSHIFT_LEFT_A 13U
#define RANDOM_XORSHIFT_RIGHT_B 17U
#define RANDOM_XORSHIFT_LEFT_C 5U
#define LYRIC_PARTICLE_SEED 0x9E3779B9U
#define LYRIC_PARTICLE_STYLE_SEED 0x85EBCA6BU

static void Effects_DrawWaveHighlight(AppDisplayRect rect, uint8_t frame);
static void Effects_DrawLyricParticles(uint8_t frame, uint8_t style);
static void Effects_DrawSonarBackground(uint8_t frame, uint8_t style);
static void Effects_DrawTremorBackground(uint8_t frame, uint8_t style);
static void Effects_DrawGradientBackground(uint8_t frame, uint8_t style);
static void Effects_DrawRainBackground(uint8_t frame, uint8_t style);
static void Effects_DrawWaveDotBackground(uint8_t frame, uint8_t style);
static void Effects_DrawLyricParticle(int16_t x, int16_t y, uint8_t radius);
static void Effects_DrawBurstHighlight(AppDisplayRect rect, uint8_t frame);
static void Effects_DrawWingHighlight(AppDisplayRect rect, uint8_t frame);
static void Effects_DrawStripeHighlight(AppDisplayRect rect, uint8_t frame);
static void Effects_DrawSparkHighlight(AppDisplayRect rect, uint8_t frame);
static void Effects_DrawDoubleFrameHighlight(AppDisplayRect rect, uint8_t frame);
static void Effects_DrawZigZagHighlight(AppDisplayRect rect, uint8_t frame);
static void Effects_DrawBandHighlight(AppDisplayRect rect, uint8_t frame);
static void Effects_DrawDottedHighlight(AppDisplayRect rect, uint8_t frame);
static void Effects_DrawShadowHighlight(AppDisplayRect rect, uint8_t frame);
static uint16_t Coordinate_SubtractFloorZero(uint16_t value, uint16_t amount);
static uint32_t Random_Xorshift32(uint32_t seed);

void AppDisplayEffects_DrawMultiLineAccent(AppDisplayLineSpan span, AppAnimationFrame frame, AppAnimationStyle style)
{
  uint8_t first_y = span.first_y;
  uint8_t last_y = span.last_y;
  uint8_t frame_value = frame.value;
  uint8_t style_value = style.value;
  uint8_t top = (first_y > 2U) ? (uint8_t)(first_y - 2U) : 0U;
  uint8_t bottom = (uint8_t)(last_y + SSD1306_Font_7x10.FontHeight + 1U);
  uint8_t pulse = (uint8_t)(frame_value & 3U);

  if (bottom >= SSD1306_HEIGHT)
  {
    bottom = SSD1306_HEIGHT - 1U;
  }

  switch (style_value % 5U)
  {
    case 0U:
      SSD1306_DrawLine((uint16_t)(2U + pulse), top, (uint16_t)(2U + pulse), bottom, SSD1306_COLOR_WHITE);
      SSD1306_DrawLine((uint16_t)(SSD1306_WIDTH - 3U - pulse), top, (uint16_t)(SSD1306_WIDTH - 3U - pulse), bottom, SSD1306_COLOR_WHITE);
      break;

    case 1U:
      SSD1306_DrawLine(0U, top, (uint16_t)(8U + pulse), top, SSD1306_COLOR_WHITE);
      SSD1306_DrawLine((uint16_t)(SSD1306_WIDTH - 9U - pulse), top, (uint16_t)(SSD1306_WIDTH - 1U), top, SSD1306_COLOR_WHITE);
      SSD1306_DrawLine(0U, bottom, (uint16_t)(8U + pulse), bottom, SSD1306_COLOR_WHITE);
      SSD1306_DrawLine((uint16_t)(SSD1306_WIDTH - 9U - pulse), bottom, (uint16_t)(SSD1306_WIDTH - 1U), bottom, SSD1306_COLOR_WHITE);
      break;

    case 2U:
      for (uint8_t x = (uint8_t)(frame_value & 3U); x < SSD1306_WIDTH; x = (uint8_t)(x + 12U))
      {
        SSD1306_DrawPixel(x, top, SSD1306_COLOR_WHITE);
        SSD1306_DrawPixel((uint16_t)(SSD1306_WIDTH - 1U - x), bottom, SSD1306_COLOR_WHITE);
      }
      break;

    case 3U:
      SSD1306_DrawLine(0U, (uint16_t)(top + pulse), (uint16_t)(SSD1306_WIDTH - 1U), (uint16_t)(top + pulse), SSD1306_COLOR_WHITE);
      SSD1306_DrawLine(0U, (uint16_t)(bottom - pulse), (uint16_t)(SSD1306_WIDTH - 1U), (uint16_t)(bottom - pulse), SSD1306_COLOR_WHITE);
      break;

    default:
      SSD1306_DrawPixel((uint16_t)(12U + (pulse * 5U)), (uint16_t)(top > pulse ? top - pulse : top), SSD1306_COLOR_WHITE);
      SSD1306_DrawPixel((uint16_t)(SSD1306_WIDTH - 13U - (pulse * 5U)), (uint16_t)(top + pulse), SSD1306_COLOR_WHITE);
      SSD1306_DrawPixel((uint16_t)(24U + (pulse * 7U)), bottom, SSD1306_COLOR_WHITE);
      SSD1306_DrawPixel((uint16_t)(SSD1306_WIDTH - 25U - (pulse * 7U)), (uint16_t)(bottom > pulse ? bottom - pulse : bottom), SSD1306_COLOR_WHITE);
      break;
  }
}

void AppDisplayEffects_DrawHighlight(AppDisplayRect rect, AppAnimationFrame frame, AppAnimationStyle style)
{
  uint8_t frame_value = frame.value;
  uint8_t style_value = style.value;

  switch (style_value % LYRIC_ANIMATION_COUNT)
  {
    case 0U:
      Effects_DrawWaveHighlight(rect, frame_value);
      break;
    case 1U:
      Effects_DrawBurstHighlight(rect, frame_value);
      break;
    case 2U:
      Effects_DrawWingHighlight(rect, frame_value);
      break;
    case 3U:
      Effects_DrawStripeHighlight(rect, frame_value);
      break;
    case 4U:
      Effects_DrawSparkHighlight(rect, frame_value);
      break;
    case 5U:
      Effects_DrawDoubleFrameHighlight(rect, frame_value);
      break;
    case 6U:
      Effects_DrawZigZagHighlight(rect, frame_value);
      break;
    case 7U:
      Effects_DrawBandHighlight(rect, frame_value);
      break;
    case 8U:
      Effects_DrawDottedHighlight(rect, frame_value);
      break;
    default:
      Effects_DrawShadowHighlight(rect, frame_value);
      break;
  }
}

void AppDisplayEffects_DrawBackground(AppAnimationFrame frame, AppAnimationStyle style)
{
  uint8_t frame_value = frame.value;
  uint8_t style_value = style.value;

  switch (style_value % LYRIC_BACKGROUND_ANIMATION_COUNT)
  {
    case 0U:
      Effects_DrawLyricParticles(frame_value, style_value);
      break;
    case 1U:
      Effects_DrawSonarBackground(frame_value, style_value);
      break;
    case 2U:
      Effects_DrawTremorBackground(frame_value, style_value);
      break;
    case 3U:
      Effects_DrawGradientBackground(frame_value, style_value);
      break;
    case 4U:
      Effects_DrawRainBackground(frame_value, style_value);
      break;
    default:
      Effects_DrawWaveDotBackground(frame_value, style_value);
      break;
  }
}

static void Effects_DrawWaveHighlight(AppDisplayRect rect, uint8_t frame)
{
  uint8_t x = rect.origin.x;
  uint8_t y = rect.origin.y;
  uint8_t w = rect.size.width;
  uint8_t h = rect.size.height;
  uint8_t center = (uint8_t)(x + (w / 2U));
  uint8_t phase = (uint8_t)(frame & 3U);

  for (uint8_t col = 0U; col <= w; col++)
  {
    uint8_t px = (uint8_t)(x + col);
    uint8_t distance = (px > center) ? (uint8_t)(px - center) : (uint8_t)(center - px);
    uint8_t band = (uint8_t)(((distance / 5U) + phase) & 3U);
    uint8_t wave = (band == 0U) ? 0U : ((band == 2U) ? 2U : 1U);
    uint8_t top = (uint8_t)(y + wave);
    uint8_t bottom = (uint8_t)(y + h - wave);

    if (px >= SSD1306_WIDTH)
    {
      break;
    }

    for (uint8_t py = top; py <= bottom && py < SSD1306_HEIGHT; py++)
    {
      SSD1306_DrawPixel(px, py, SSD1306_COLOR_WHITE);
    }
  }

  SSD1306_DrawPixel(x, y, SSD1306_COLOR_BLACK);
  SSD1306_DrawPixel((uint16_t)(x + w), y, SSD1306_COLOR_BLACK);
  SSD1306_DrawPixel(x, (uint16_t)(y + h), SSD1306_COLOR_BLACK);
  SSD1306_DrawPixel((uint16_t)(x + w), (uint16_t)(y + h), SSD1306_COLOR_BLACK);
}

static void Effects_DrawLyricParticles(uint8_t frame, uint8_t style)
{
  uint32_t seed = LYRIC_PARTICLE_SEED ^ ((uint32_t)style * LYRIC_PARTICLE_STYLE_SEED);

  for (uint8_t i = 0U; i < LYRIC_PARTICLE_COUNT; i++)
  {
    uint8_t base_x;
    uint8_t base_y;
    uint8_t speed_x;
    uint8_t speed_y;
    uint8_t wobble;
    uint8_t radius;
    int16_t x;
    int16_t y;

    seed = Random_Xorshift32(seed);
    base_x = (uint8_t)(seed % SSD1306_WIDTH);

    seed = Random_Xorshift32(seed);
    base_y = (uint8_t)(seed % SSD1306_HEIGHT);

    speed_x = (uint8_t)(1U + ((seed >> LYRIC_PARTICLE_SPEED_X_SHIFT) & 3U));
    speed_y = (uint8_t)(1U + ((seed >> LYRIC_PARTICLE_SPEED_Y_SHIFT) & 1U));
    wobble = (uint8_t)(((frame + (i * 3U)) & 7U) < 4U ? (frame & 3U) : (3U - (frame & 3U)));

    x = (int16_t)(((uint32_t)base_x + ((uint32_t)frame * speed_x) + ((uint32_t)i * 11U)) % SSD1306_WIDTH);
    y = (int16_t)(((uint32_t)base_y + ((uint32_t)frame * speed_y) + wobble + ((uint32_t)i * 5U)) % SSD1306_HEIGHT);

    if ((i % 9U) == 0U)
    {
      radius = 3U;
    }
    else if ((i % 4U) == 0U)
    {
      radius = 2U;
    }
    else if ((i % 3U) == 0U)
    {
      radius = 1U;
    }
    else
    {
      radius = 0U;
    }

    Effects_DrawLyricParticle(x, y, radius);
  }
}

static void Effects_DrawSonarBackground(uint8_t frame, uint8_t style)
{
  uint8_t center_x = (uint8_t)(18U + (((style * 23U) + (frame * 3U)) % 92U));
  uint8_t center_y = (uint8_t)(8U + (((style * 7U) + frame) % 17U));
  uint8_t pulse = (uint8_t)(2U + ((frame * 2U) % 18U));

  for (uint8_t ring = 0U; ring < 3U; ring++)
  {
    int16_t radius = (int16_t)(pulse + (ring * 8U));
    if (radius < SSD1306_HEIGHT)
    {
      SSD1306_DrawCircle(center_x, center_y, radius, SSD1306_COLOR_WHITE);
    }
  }

  for (uint8_t i = 0U; i < 10U; i++)
  {
    int16_t x = (int16_t)((center_x + (i * 13U) + frame) % SSD1306_WIDTH);
    int16_t y = (int16_t)((center_y + (i * 5U) + (frame * 2U)) % SSD1306_HEIGHT);
    Effects_DrawLyricParticle(x, y, (uint8_t)(i % 3U == 0U ? 1U : 0U));
  }
}

static void Effects_DrawTremorBackground(uint8_t frame, uint8_t style)
{
  uint8_t jitter = (uint8_t)(frame & 3U);

  for (uint8_t i = 0U; i < 12U; i++)
  {
    int16_t x = (int16_t)(((i * 17U) + (style * 11U) + (frame * 5U)) % SSD1306_WIDTH);
    int16_t y = (int16_t)(((i * 7U) + (style * 3U) + ((frame & 1U) ? jitter : (3U - jitter))) % SSD1306_HEIGHT);
    int16_t length = (int16_t)(3U + ((i + frame) & 5U));
    int16_t tilt = (int16_t)(((i + frame) & 1U) ? 2 : -2);
    int16_t x_end = (int16_t)(x + length);
    int16_t y_end = (int16_t)(y + tilt);

    if (x_end >= SSD1306_WIDTH)
    {
      x_end = SSD1306_WIDTH - 1U;
    }
    if (y_end < 0)
    {
      y_end = 0;
    }
    else if (y_end >= SSD1306_HEIGHT)
    {
      y_end = SSD1306_HEIGHT - 1U;
    }

    SSD1306_DrawLine((uint16_t)x, (uint16_t)y, (uint16_t)x_end, (uint16_t)y_end, SSD1306_COLOR_WHITE);
    if ((i & 3U) == 0U)
    {
      Effects_DrawLyricParticle((int16_t)(x + 2), y, 1U);
    }
  }
}

static void Effects_DrawGradientBackground(uint8_t frame, uint8_t style)
{
  for (uint8_t y = 0U; y < SSD1306_HEIGHT; y++)
  {
    uint8_t density = (uint8_t)(((y + frame + (style * 3U)) / 4U) & 3U);
    uint8_t spacing = (uint8_t)(5U - density);
    uint8_t offset = (uint8_t)((frame + y + style) % spacing);

    for (uint8_t x = offset; x < SSD1306_WIDTH; x = (uint8_t)(x + spacing))
    {
      SSD1306_DrawPixel(x, y, SSD1306_COLOR_WHITE);
    }
  }

  for (uint8_t band = 0U; band < 3U; band++)
  {
    uint8_t y = (uint8_t)(((frame * 2U) + (band * 11U)) % SSD1306_HEIGHT);
    SSD1306_DrawLine(0U, y, (uint16_t)(SSD1306_WIDTH - 1U), y, SSD1306_COLOR_WHITE);
  }
}

static void Effects_DrawRainBackground(uint8_t frame, uint8_t style)
{
  for (uint8_t i = 0U; i < 18U; i++)
  {
    uint8_t x = (uint8_t)(((i * 19U) + (style * 13U) + frame) % SSD1306_WIDTH);
    uint8_t y = (uint8_t)(((i * 9U) + (frame * 3U)) % SSD1306_HEIGHT);
    uint8_t length = (uint8_t)(2U + ((i + style) & 3U));
    uint8_t y_end = (uint8_t)(y + length);

    if (y_end >= SSD1306_HEIGHT)
    {
      y_end = SSD1306_HEIGHT - 1U;
    }

    SSD1306_DrawLine(x, y, (uint16_t)(x > 2U ? x - 2U : 0U), y_end, SSD1306_COLOR_WHITE);
    if ((i % 5U) == 0U)
    {
      Effects_DrawLyricParticle(x, y, 1U);
    }
  }
}

static void Effects_DrawWaveDotBackground(uint8_t frame, uint8_t style)
{
  for (uint8_t x = (uint8_t)(frame & 3U); x < SSD1306_WIDTH; x = (uint8_t)(x + 6U))
  {
    uint8_t wave_y = (uint8_t)((((x / 6U) + frame + style) & 7U) < 4U ? 8U : 20U);
    uint8_t y = (uint8_t)(wave_y + (((x + frame) & 3U) == 0U ? 2U : 0U));

    SSD1306_DrawPixel(x, y, SSD1306_COLOR_WHITE);
    SSD1306_DrawPixel((uint16_t)(SSD1306_WIDTH - 1U - x), (uint16_t)(SSD1306_HEIGHT - 1U - y), SSD1306_COLOR_WHITE);

    if (((x + frame) % 18U) == 0U)
    {
      Effects_DrawLyricParticle(x, y, 1U);
    }
  }
}

static void Effects_DrawLyricParticle(int16_t x, int16_t y, uint8_t radius)
{
  if (radius == 0U)
  {
    if (x >= 0 && x < SSD1306_WIDTH && y >= 0 && y < SSD1306_HEIGHT)
    {
      SSD1306_DrawPixel((uint16_t)x, (uint16_t)y, SSD1306_COLOR_WHITE);
    }
    return;
  }

  for (int16_t dy = -(int16_t)radius; dy <= (int16_t)radius; dy++)
  {
    for (int16_t dx = -(int16_t)radius; dx <= (int16_t)radius; dx++)
    {
      int16_t px = (int16_t)(x + dx);
      int16_t py = (int16_t)(y + dy);

      if ((dx * dx + dy * dy) <= (int16_t)(radius * radius) &&
          px >= 0 && px < SSD1306_WIDTH &&
          py >= 0 && py < SSD1306_HEIGHT)
      {
        SSD1306_DrawPixel((uint16_t)px, (uint16_t)py, SSD1306_COLOR_WHITE);
      }
    }
  }
}

static void Effects_DrawBurstHighlight(AppDisplayRect rect, uint8_t frame)
{
  uint8_t x = rect.origin.x;
  uint8_t y = rect.origin.y;
  uint8_t w = rect.size.width;
  uint8_t h = rect.size.height;
  uint8_t center_x = (uint8_t)(x + (w / 2U));
  uint8_t center_y = (uint8_t)(y + (h / 2U));
  uint8_t reach = (uint8_t)(2U + (frame & 3U));

  SSD1306_DrawRectangle(x, y, w, h, SSD1306_COLOR_WHITE);
  SSD1306_DrawLine(center_x, y, center_x, Coordinate_SubtractFloorZero(y, reach), SSD1306_COLOR_WHITE);
  SSD1306_DrawLine(center_x, (uint16_t)(y + h), center_x, (uint16_t)(y + h + reach), SSD1306_COLOR_WHITE);
  SSD1306_DrawLine(x, center_y, Coordinate_SubtractFloorZero(x, reach), center_y, SSD1306_COLOR_WHITE);
  SSD1306_DrawLine((uint16_t)(x + w), center_y, (uint16_t)(x + w + reach), center_y, SSD1306_COLOR_WHITE);
  SSD1306_DrawPixel((uint16_t)(center_x > reach ? center_x - reach : center_x), (uint16_t)(center_y > reach ? center_y - reach : center_y), SSD1306_COLOR_WHITE);
  SSD1306_DrawPixel((uint16_t)(center_x + reach), (uint16_t)(center_y > reach ? center_y - reach : center_y), SSD1306_COLOR_WHITE);
  SSD1306_DrawPixel((uint16_t)(center_x > reach ? center_x - reach : center_x), (uint16_t)(center_y + reach), SSD1306_COLOR_WHITE);
  SSD1306_DrawPixel((uint16_t)(center_x + reach), (uint16_t)(center_y + reach), SSD1306_COLOR_WHITE);
}

static void Effects_DrawWingHighlight(AppDisplayRect rect, uint8_t frame)
{
  uint8_t x = rect.origin.x;
  uint8_t y = rect.origin.y;
  uint8_t w = rect.size.width;
  uint8_t h = rect.size.height;
  uint8_t wing = (uint8_t)(3U + ((frame & 3U) * 2U));
  uint8_t mid_y = (uint8_t)(y + (h / 2U));

  SSD1306_DrawFilledRectangle(x, y, w, h, SSD1306_COLOR_WHITE);
  SSD1306_DrawLine(Coordinate_SubtractFloorZero(x, wing), mid_y, x, y, SSD1306_COLOR_WHITE);
  SSD1306_DrawLine(Coordinate_SubtractFloorZero(x, wing), mid_y, x, (uint16_t)(y + h), SSD1306_COLOR_WHITE);
  SSD1306_DrawLine((uint16_t)(x + w), y, (uint16_t)(x + w + wing), mid_y, SSD1306_COLOR_WHITE);
  SSD1306_DrawLine((uint16_t)(x + w), (uint16_t)(y + h), (uint16_t)(x + w + wing), mid_y, SSD1306_COLOR_WHITE);
}

static void Effects_DrawStripeHighlight(AppDisplayRect rect, uint8_t frame)
{
  uint8_t x = rect.origin.x;
  uint8_t y = rect.origin.y;
  uint8_t w = rect.size.width;
  uint8_t h = rect.size.height;

  SSD1306_DrawFilledRectangle(x, y, w, h, SSD1306_COLOR_WHITE);
  for (uint8_t col = (uint8_t)(frame & 3U); col <= w; col = (uint8_t)(col + 4U))
  {
    uint8_t px = (uint8_t)(x + col);
    if (px < SSD1306_WIDTH)
    {
      SSD1306_DrawLine(px, y, px, (uint16_t)(y + h), SSD1306_COLOR_BLACK);
    }
  }
}

static void Effects_DrawSparkHighlight(AppDisplayRect rect, uint8_t frame)
{
  uint8_t x = rect.origin.x;
  uint8_t y = rect.origin.y;
  uint8_t w = rect.size.width;
  uint8_t h = rect.size.height;
  uint8_t sparkle = (uint8_t)(2U + (frame & 3U));

  SSD1306_DrawRectangle(x, y, w, h, SSD1306_COLOR_WHITE);
  SSD1306_DrawPixel((uint16_t)(x + sparkle), (uint16_t)(y > 1U ? y - 1U : y), SSD1306_COLOR_WHITE);
  SSD1306_DrawPixel(Coordinate_SubtractFloorZero((uint16_t)(x + w), sparkle), Coordinate_SubtractFloorZero(y, 1U), SSD1306_COLOR_WHITE);
  SSD1306_DrawPixel((uint16_t)(x + sparkle), (uint16_t)(y + h + 1U), SSD1306_COLOR_WHITE);
  SSD1306_DrawPixel(Coordinate_SubtractFloorZero((uint16_t)(x + w), sparkle), (uint16_t)(y + h + 1U), SSD1306_COLOR_WHITE);
  SSD1306_DrawPixel((uint16_t)(x + sparkle + 1U), y, SSD1306_COLOR_WHITE);
  SSD1306_DrawPixel(Coordinate_SubtractFloorZero((uint16_t)(x + w), (uint16_t)(sparkle + 1U)), (uint16_t)(y + h), SSD1306_COLOR_WHITE);
  SSD1306_DrawPixel(Coordinate_SubtractFloorZero(x, sparkle), (uint16_t)(y + (h / 2U)), SSD1306_COLOR_WHITE);
  SSD1306_DrawPixel((uint16_t)(x + w + sparkle), (uint16_t)(y + (h / 2U)), SSD1306_COLOR_WHITE);
}

static void Effects_DrawDoubleFrameHighlight(AppDisplayRect rect, uint8_t frame)
{
  uint8_t x = rect.origin.x;
  uint8_t y = rect.origin.y;
  uint8_t w = rect.size.width;
  uint8_t h = rect.size.height;
  uint8_t inset = (uint8_t)(1U + (frame & 1U));

  SSD1306_DrawFilledRectangle(x, y, w, h, SSD1306_COLOR_WHITE);
  SSD1306_DrawRectangle(x, y, w, h, SSD1306_COLOR_BLACK);
  if (w > (inset * 2U) && h > (inset * 2U))
  {
    SSD1306_DrawRectangle((uint16_t)(x + inset), (uint16_t)(y + inset), (uint16_t)(w - (inset * 2U)), (uint16_t)(h - (inset * 2U)), SSD1306_COLOR_BLACK);
  }
}

static void Effects_DrawZigZagHighlight(AppDisplayRect rect, uint8_t frame)
{
  uint8_t x = rect.origin.x;
  uint8_t y = rect.origin.y;
  uint8_t w = rect.size.width;
  uint8_t h = rect.size.height;

  for (uint8_t col = 0U; col <= w; col++)
  {
    uint8_t px = (uint8_t)(x + col);
    uint8_t offset = (uint8_t)(((col + frame) & 3U) < 2U ? 0U : 2U);
    if (px < SSD1306_WIDTH)
    {
      SSD1306_DrawPixel(px, (uint16_t)(y + offset), SSD1306_COLOR_WHITE);
      SSD1306_DrawPixel(px, (uint16_t)(y + h - offset), SSD1306_COLOR_WHITE);
    }
  }
}

static void Effects_DrawBandHighlight(AppDisplayRect rect, uint8_t frame)
{
  uint8_t x = rect.origin.x;
  uint8_t y = rect.origin.y;
  uint8_t w = rect.size.width;
  uint8_t h = rect.size.height;
  uint8_t band = (uint8_t)(2U + (frame % 5U));

  SSD1306_DrawFilledRectangle(x, y, w, h, SSD1306_COLOR_WHITE);
  SSD1306_DrawLine(x, (uint16_t)(y + band), (uint16_t)(x + w), (uint16_t)(y + band), SSD1306_COLOR_BLACK);
  if (h > band + 4U)
  {
    SSD1306_DrawLine(x, (uint16_t)(y + h - band), (uint16_t)(x + w), (uint16_t)(y + h - band), SSD1306_COLOR_BLACK);
  }
}

static void Effects_DrawDottedHighlight(AppDisplayRect rect, uint8_t frame)
{
  uint8_t x = rect.origin.x;
  uint8_t y = rect.origin.y;
  uint8_t w = rect.size.width;
  uint8_t h = rect.size.height;

  for (uint8_t col = (uint8_t)(frame & 1U); col <= w; col = (uint8_t)(col + 3U))
  {
    SSD1306_DrawPixel((uint16_t)(x + col), y, SSD1306_COLOR_WHITE);
    SSD1306_DrawPixel((uint16_t)(x + col), (uint16_t)(y + h), SSD1306_COLOR_WHITE);
  }
  for (uint8_t row = (uint8_t)((frame + 1U) & 1U); row <= h; row = (uint8_t)(row + 3U))
  {
    SSD1306_DrawPixel(x, (uint16_t)(y + row), SSD1306_COLOR_WHITE);
    SSD1306_DrawPixel((uint16_t)(x + w), (uint16_t)(y + row), SSD1306_COLOR_WHITE);
  }
}

static void Effects_DrawShadowHighlight(AppDisplayRect rect, uint8_t frame)
{
  uint8_t x = rect.origin.x;
  uint8_t y = rect.origin.y;
  uint8_t w = rect.size.width;
  uint8_t h = rect.size.height;
  uint8_t shadow = (uint8_t)(1U + (frame & 1U));

  if ((x + w + shadow) < SSD1306_WIDTH && (y + h + shadow) < SSD1306_HEIGHT)
  {
    SSD1306_DrawRectangle((uint16_t)(x + shadow), (uint16_t)(y + shadow), w, h, SSD1306_COLOR_WHITE);
  }
  SSD1306_DrawFilledRectangle(x, y, w, h, SSD1306_COLOR_WHITE);
  SSD1306_DrawLine(x, y, (uint16_t)(x + w), y, SSD1306_COLOR_BLACK);
  SSD1306_DrawLine(x, y, x, (uint16_t)(y + h), SSD1306_COLOR_BLACK);
}

static uint32_t Random_Xorshift32(uint32_t seed)
{
  seed ^= seed << RANDOM_XORSHIFT_LEFT_A;
  seed ^= seed >> RANDOM_XORSHIFT_RIGHT_B;
  seed ^= seed << RANDOM_XORSHIFT_LEFT_C;

  return seed;
}

static uint16_t Coordinate_SubtractFloorZero(uint16_t value, uint16_t amount)
{
  return (value > amount) ? (uint16_t)(value - amount) : 0U;
}
