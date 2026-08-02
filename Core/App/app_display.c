#include "app_display.h"

#include "app_config.h"
#include "app_display_effects.h"
#include "app_text.h"
#include "display_port.h"
#include "storage_port.h"
#include "ssd1306.h"

#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#ifndef APP_BUILD_COMMIT
#define APP_BUILD_COMMIT "unknown"
#endif

#ifndef APP_BUILD_TYPE
#define APP_BUILD_TYPE "unknown"
#endif

#define DISPLAY_CHARS_PER_LINE 18U
#define DISPLAY_ROW_BUFFER_LENGTH 56U
#define DISPLAY_MAX_ROWS 3U
#define DISPLAY_BROWSER_HEADER_Y 0U
#define DISPLAY_BROWSER_TRACK_LINE0_Y 11U
#define DISPLAY_BROWSER_TRACK_LINE1_Y 21U
#define DISPLAY_CENTER_ONE_LINE_Y 16U
#define DISPLAY_CENTER_TWO_LINE0_Y 10U
#define DISPLAY_CENTER_TWO_LINE1_Y 21U
#define DISPLAY_CENTER_THREE_LINE0_Y 1U
#define DISPLAY_CENTER_THREE_LINE1_Y 11U
#define DISPLAY_CENTER_THREE_LINE2_Y 21U
#define DISPLAY_LARGE_FONT_MARGIN_X 8U
#define DISPLAY_SHAKE_PATTERN_MASK 7U
#define DISPLAY_BOOT_INFO_HOLD_MS 1500U
#define LYRIC_STYLE_TREMOR 1U
#define LYRIC_STYLE_BURST 3U
#define LYRIC_STYLE_STRIPE 4U
#define LYRIC_STYLE_ZIGZAG 6U
#define LYRIC_STYLE_DOTTED 7U
#define LYRIC_STYLE_SHADOW 8U

static bool display_ready = false;
static bool display_rendering_lyric = false;
static AppAnimationFrame current_effect_frame = {0U};
static AppAnimationStyle current_effect_style = {0U};
static AppAnimationStyle current_background_style = {0U};

static void Display_PrintLine(uint8_t y, const char *text);
static void Display_PrintLineOffset(uint8_t y, const char *text, int8_t offset_x, int8_t offset_y);
static void Display_PrintCenteredRows(char rows[][DISPLAY_ROW_BUFFER_LENGTH], uint32_t row_count);
static uint32_t Utf8_DecodeGlyph(const char **text);
static void Display_DrawCodepoint(uint32_t codepoint, uint8_t x, uint8_t y, SSD1306_Font_t *font, SSD1306_COLOR_t color);
static void Display_DrawInvertedExclamation(uint8_t x, uint8_t y, SSD1306_Font_t *font, SSD1306_COLOR_t foreground, SSD1306_COLOR_t background);
static void Display_DrawInvertedQuestion(uint8_t x, uint8_t y, SSD1306_Font_t *font, SSD1306_COLOR_t foreground, SSD1306_COLOR_t background);
static void Display_DrawAccent(uint8_t x, uint8_t y, SSD1306_Font_t *font, char accent, SSD1306_COLOR_t color);
static void Display_PrintWrappedText(const char *text);
static void Display_PrintHighlightedSingleLine(const char *text);

bool AppDisplay_Init(void)
{
  display_ready = DisplayPort_Init();
  return display_ready;
}

void AppDisplay_ShowBootInfo(void)
{
  if (!display_ready)
  {
    return;
  }

  AppDisplay_ShowMessage("Lyrics", APP_BUILD_COMMIT, APP_BUILD_TYPE);
  DisplayPort_DelayMs(DISPLAY_BOOT_INFO_HOLD_MS);
}

void AppDisplay_ShowSdScanning(void)
{
  AppDisplay_ShowMessage("SD card", "Scanning...", "");
}

void AppDisplay_ShowTrackBrowser(const AppTrackBrowserView *view)
{
  char header[DISPLAY_CHARS_PER_LINE + 1U];
  const char *selected_track_name;

  if (view == NULL)
  {
    AppDisplay_ShowMessage("Track scan error", "Bad parameter", "");
    return;
  }

  if (view->status != APP_TRACKS_STATUS_OK && view->status != APP_TRACKS_STATUS_NO_TRACKS)
  {
    char diagnostic[DISPLAY_CHARS_PER_LINE + 1U];
    snprintf(diagnostic,
             sizeof(diagnostic),
             "SD:%u C:%u R:%02X",
             (unsigned int)StoragePort_GetLastErrorStep(),
             (unsigned int)StoragePort_GetLastCommand(),
             (unsigned int)StoragePort_GetLastCommandResponse());
    switch (view->status)
    {
      case APP_TRACKS_STATUS_INVALID_PARAMETER:
        AppDisplay_ShowMessage("Track scan error", "Bad parameter", "");
        break;
      case APP_TRACKS_STATUS_MOUNT_FAILED:
        AppDisplay_ShowMessage("SD mount error", diagnostic, "");
        break;
      case APP_TRACKS_STATUS_OPEN_DIR_FAILED:
        AppDisplay_ShowMessage("SD dir error", diagnostic, "");
        break;
      case APP_TRACKS_STATUS_READ_DIR_FAILED:
        AppDisplay_ShowMessage("SD read error", diagnostic, "");
        break;
      default:
        AppDisplay_ShowMessage("Track scan error", diagnostic, "");
        break;
    }
    return;
  }

  if (view->status == APP_TRACKS_STATUS_NO_TRACKS || view->track_count == 0U)
  {
    AppDisplay_ShowMessage("No MP3 files", "Root only", "");
    return;
  }

  selected_track_name = view->selected_track_name;
  if (selected_track_name == NULL)
  {
    selected_track_name = "";
  }

  snprintf(header, sizeof(header), "%lu/%lu MP3", (unsigned long)(view->selected_track_index + 1U), (unsigned long)view->track_count);

  if (!display_ready)
  {
    return;
  }

  SSD1306_Fill(SSD1306_COLOR_BLACK);
  Display_PrintLine(DISPLAY_BROWSER_HEADER_Y, header);
  Display_PrintLine(DISPLAY_BROWSER_TRACK_LINE0_Y, selected_track_name);
  if (strlen(selected_track_name) > DISPLAY_CHARS_PER_LINE)
  {
    Display_PrintLine(DISPLAY_BROWSER_TRACK_LINE1_Y, &selected_track_name[DISPLAY_CHARS_PER_LINE]);
  }
  SSD1306_UpdateScreen();
}

void AppDisplay_ShowMessage(const char *line0, const char *line1, const char *line2)
{
  char rows[DISPLAY_MAX_ROWS][DISPLAY_ROW_BUFFER_LENGTH] = {{0}};

  if (!display_ready)
  {
    return;
  }

  if (line0 != NULL)
  {
    strncpy(rows[0], line0, DISPLAY_ROW_BUFFER_LENGTH - 1U);
  }
  if (line1 != NULL)
  {
    strncpy(rows[1], line1, DISPLAY_ROW_BUFFER_LENGTH - 1U);
  }
  if (line2 != NULL)
  {
    strncpy(rows[2], line2, DISPLAY_ROW_BUFFER_LENGTH - 1U);
  }

  SSD1306_Fill(SSD1306_COLOR_BLACK);
  Display_PrintCenteredRows(rows, DISPLAY_MAX_ROWS);
  SSD1306_UpdateScreen();
}

static void Display_PrintLine(uint8_t y, const char *text)
{
  Display_PrintLineOffset(y, text, 0, 0);
}

static void Display_PrintLineOffset(uint8_t y, const char *text, int8_t offset_x, int8_t offset_y)
{
  uint8_t x;
  uint8_t draw_y;
  uint32_t glyph_count = 0U;
  size_t visible_glyphs;
  uint16_t text_width;
  int16_t shifted_x;
  int16_t shifted_y;

  if (text == NULL)
  {
    text = "";
  }

  visible_glyphs = AppText_Utf8GlyphCount(text);
  if (visible_glyphs > DISPLAY_CHARS_PER_LINE)
  {
    visible_glyphs = DISPLAY_CHARS_PER_LINE;
  }

  text_width = (uint16_t)(visible_glyphs * SSD1306_Font_7x10.FontWidth);
  shifted_x = (int16_t)((int16_t)((SSD1306_WIDTH - text_width) / 2U) + (int16_t)offset_x);
  shifted_y = (int16_t)(y + offset_y);

  if (shifted_x < 0)
  {
    shifted_x = 0;
  }
  else if ((shifted_x + text_width) >= SSD1306_WIDTH)
  {
    shifted_x = (int16_t)(SSD1306_WIDTH - text_width - 1U);
  }

  if (shifted_y < 0)
  {
    shifted_y = 0;
  }
  else if (shifted_y >= SSD1306_HEIGHT)
  {
    shifted_y = SSD1306_HEIGHT - 1U;
  }

  x = (uint8_t)shifted_x;
  draw_y = (uint8_t)shifted_y;

  while (*text != '\0' && glyph_count < DISPLAY_CHARS_PER_LINE)
  {
    uint32_t codepoint = Utf8_DecodeGlyph(&text);

    Display_DrawCodepoint(codepoint, x, draw_y, &SSD1306_Font_7x10, SSD1306_COLOR_WHITE);
    x += SSD1306_Font_7x10.FontWidth;
    glyph_count++;
  }
}

static uint32_t Utf8_DecodeGlyph(const char **text)
{
  const uint8_t *bytes = (const uint8_t *)*text;
  uint32_t codepoint;

  if (bytes[0] < 0x80U)
  {
    (*text)++;
    return bytes[0];
  }

  if ((bytes[0] & 0xE0U) == 0xC0U && bytes[1] != 0U)
  {
    codepoint = ((uint32_t)(bytes[0] & 0x1FU) << 6) | (uint32_t)(bytes[1] & 0x3FU);
    *text += 2;
    return codepoint;
  }

  (*text)++;
  return '?';
}

static void Display_DrawCodepoint(uint32_t codepoint, uint8_t x, uint8_t y, SSD1306_Font_t *font, SSD1306_COLOR_t color)
{
  char base = '\0';
  char accent = '\0';

  switch (codepoint)
  {
    case 0x00A1U:
      Display_DrawInvertedExclamation(x, y, font, color, (SSD1306_COLOR_t)!color);
      return;
    case 0x00BFU:
      Display_DrawInvertedQuestion(x, y, font, color, (SSD1306_COLOR_t)!color);
      return;
    case 0x00C1U:
      base = 'A';
      accent = '/';
      break;
    case 0x00E1U:
      base = 'a';
      accent = '/';
      break;
    case 0x00C9U:
      base = 'E';
      accent = '/';
      break;
    case 0x00E9U:
      base = 'e';
      accent = '/';
      break;
    case 0x00CDU:
      base = 'I';
      accent = '/';
      break;
    case 0x00EDU:
      base = 'i';
      accent = '/';
      break;
    case 0x00D3U:
      base = 'O';
      accent = '/';
      break;
    case 0x00F3U:
      base = 'o';
      accent = '/';
      break;
    case 0x00DAU:
      base = 'U';
      accent = '/';
      break;
    case 0x00FAU:
      base = 'u';
      accent = '/';
      break;
    case 0x00DCU:
      base = 'U';
      accent = ':';
      break;
    case 0x00FCU:
      base = 'u';
      accent = ':';
      break;
    case 0x00D1U:
      base = 'N';
      accent = '~';
      break;
    case 0x00F1U:
      base = 'n';
      accent = '~';
      break;
    default:
      base = (codepoint >= 32U && codepoint <= 126U) ? (char)codepoint : '?';
      break;
  }

  SSD1306_GotoXY(x, y);
  (void)SSD1306_Putc(base, font, color);

  if (accent != '\0')
  {
    Display_DrawAccent(x, y, font, accent, color);
  }
}

static void Display_DrawInvertedExclamation(uint8_t x, uint8_t y, SSD1306_Font_t *font, SSD1306_COLOR_t foreground, SSD1306_COLOR_t background)
{
  uint8_t center_x = (uint8_t)(x + (font->FontWidth / 2U));
  uint8_t dot_y = (uint8_t)(y + 1U);
  uint8_t line_start = (uint8_t)(y + 4U);
  uint8_t line_end = (uint8_t)(y + font->FontHeight - 2U);

  for (uint8_t row = 0U; row < font->FontHeight; row++)
  {
    for (uint8_t col = 0U; col < font->FontWidth; col++)
    {
      SSD1306_DrawPixel((uint16_t)(x + col), (uint16_t)(y + row), background);
    }
  }

  SSD1306_DrawPixel(center_x, dot_y, foreground);
  for (uint8_t row = line_start; row <= line_end; row++)
  {
    SSD1306_DrawPixel(center_x, row, foreground);
    if (font->FontWidth > 8U)
    {
      SSD1306_DrawPixel((uint16_t)(center_x + 1U), row, foreground);
    }
  }
}

static void Display_DrawInvertedQuestion(uint8_t x, uint8_t y, SSD1306_Font_t *font, SSD1306_COLOR_t foreground, SSD1306_COLOR_t background)
{
  uint8_t center_x = (uint8_t)(x + (font->FontWidth / 2U));
  uint8_t bottom_y = (uint8_t)(y + font->FontHeight - 2U);

  for (uint8_t row = 0U; row < font->FontHeight; row++)
  {
    for (uint8_t col = 0U; col < font->FontWidth; col++)
    {
      SSD1306_DrawPixel((uint16_t)(x + col), (uint16_t)(y + row), background);
    }
  }

  SSD1306_DrawPixel(center_x, (uint16_t)(y + 1U), foreground);
  SSD1306_DrawPixel(center_x, (uint16_t)(y + 4U), foreground);
  SSD1306_DrawLine((uint16_t)(center_x - 2U), (uint16_t)(y + 6U), center_x, (uint16_t)(y + 4U), foreground);
  SSD1306_DrawLine((uint16_t)(center_x - 3U), (uint16_t)(y + 7U), (uint16_t)(center_x - 3U), (uint16_t)(bottom_y - 3U), foreground);
  SSD1306_DrawLine((uint16_t)(center_x - 2U), (uint16_t)(bottom_y - 2U), (uint16_t)(center_x + 2U), bottom_y, foreground);
}

static void Display_DrawAccent(uint8_t x, uint8_t y, SSD1306_Font_t *font, char accent, SSD1306_COLOR_t color)
{
  uint8_t mid = (uint8_t)(x + (font->FontWidth / 2U));

  switch (accent)
  {
    case '/':
      SSD1306_DrawPixel((uint16_t)(mid + 1U), y, color);
      SSD1306_DrawPixel(mid, (uint16_t)(y + 1U), color);
      if (font->FontWidth > 8U)
      {
        SSD1306_DrawPixel((uint16_t)(mid - 1U), (uint16_t)(y + 2U), color);
      }
      break;
    case ':':
      SSD1306_DrawPixel((uint16_t)(mid - 2U), y, color);
      SSD1306_DrawPixel((uint16_t)(mid + 2U), y, color);
      break;
    case '~':
      SSD1306_DrawPixel((uint16_t)(mid - 3U), (uint16_t)(y + 1U), color);
      SSD1306_DrawPixel((uint16_t)(mid - 2U), y, color);
      SSD1306_DrawPixel((uint16_t)(mid - 1U), y, color);
      SSD1306_DrawPixel(mid, (uint16_t)(y + 1U), color);
      SSD1306_DrawPixel((uint16_t)(mid + 1U), (uint16_t)(y + 1U), color);
      SSD1306_DrawPixel((uint16_t)(mid + 2U), y, color);
      break;
    default:
      break;
  }
}

void AppDisplay_ShowLyricText(const AppLyricRenderView *view)
{
  if (view == NULL)
  {
    return;
  }

  current_effect_frame = view->frame;
  current_effect_style = view->text_style;
  current_background_style = view->background_style;

  if (!display_ready)
  {
    return;
  }

  SSD1306_Fill(SSD1306_COLOR_BLACK);
  display_rendering_lyric = true;
  AppDisplayEffects_DrawBackground(current_effect_frame, current_background_style);
  Display_PrintWrappedText(view->text);
  display_rendering_lyric = false;
  SSD1306_UpdateScreen();
}

static void Display_PrintWrappedText(const char *text)
{
  char rows[DISPLAY_MAX_ROWS][DISPLAY_ROW_BUFFER_LENGTH] = {{0}};
  uint32_t row = 0U;
  size_t row_bytes = 0U;
  size_t row_glyphs = 0U;
  const char *p = text;

  while (*p != '\0' && row < DISPLAY_MAX_ROWS)
  {
    char word[DISPLAY_ROW_BUFFER_LENGTH];
    size_t word_bytes = 0U;
    size_t word_glyphs = 0U;

    while (*p == ' ')
    {
      p++;
    }

    if (*p == '\0')
    {
      break;
    }

    while (*p != '\0' && *p != ' ' && word_glyphs < DISPLAY_CHARS_PER_LINE)
    {
      size_t glyph_length = AppText_Utf8GlyphLength(p);

      if ((word_bytes + glyph_length) >= sizeof(word))
      {
        break;
      }

      for (size_t i = 0U; i < glyph_length; i++)
      {
        word[word_bytes++] = p[i];
      }

      p += glyph_length;
      word_glyphs++;
    }
    word[word_bytes] = '\0';

    while (*p != '\0' && *p != ' ')
    {
      p += AppText_Utf8GlyphLength(p);
    }

    if (word_glyphs == 0U)
    {
      continue;
    }

    if (row_glyphs == 0U)
    {
      strncpy(rows[row], word, DISPLAY_ROW_BUFFER_LENGTH - 1U);
      rows[row][DISPLAY_ROW_BUFFER_LENGTH - 1U] = '\0';
      row_bytes = strlen(rows[row]);
      row_glyphs = AppText_Utf8GlyphCount(rows[row]);
    }
    else if ((row_glyphs + 1U + word_glyphs) <= DISPLAY_CHARS_PER_LINE)
    {
      if ((row_bytes + 1U + word_bytes) < DISPLAY_ROW_BUFFER_LENGTH)
      {
        rows[row][row_bytes++] = ' ';
        rows[row][row_bytes] = '\0';
        strncat(rows[row], word, DISPLAY_ROW_BUFFER_LENGTH - row_bytes - 1U);
        row_bytes = strlen(rows[row]);
        row_glyphs = AppText_Utf8GlyphCount(rows[row]);
      }
    }
    else
    {
      row++;
      row_bytes = 0U;
      row_glyphs = 0U;
      if (row < DISPLAY_MAX_ROWS)
      {
        strncpy(rows[row], word, DISPLAY_ROW_BUFFER_LENGTH - 1U);
        rows[row][DISPLAY_ROW_BUFFER_LENGTH - 1U] = '\0';
        row_bytes = strlen(rows[row]);
        row_glyphs = AppText_Utf8GlyphCount(rows[row]);
      }
    }
  }

  Display_PrintCenteredRows(rows, DISPLAY_MAX_ROWS);
}

static void Display_PrintCenteredRows(char rows[][DISPLAY_ROW_BUFFER_LENGTH], uint32_t row_count)
{
  char visible_rows[DISPLAY_MAX_ROWS][DISPLAY_ROW_BUFFER_LENGTH] = {{0}};
  uint32_t visible_count = 0U;
  const uint8_t *y_positions;
  static const uint8_t one_line_y[] = {DISPLAY_CENTER_ONE_LINE_Y};
  static const uint8_t two_line_y[] = {DISPLAY_CENTER_TWO_LINE0_Y, DISPLAY_CENTER_TWO_LINE1_Y};
  static const uint8_t three_line_y[] = {DISPLAY_CENTER_THREE_LINE0_Y, DISPLAY_CENTER_THREE_LINE1_Y, DISPLAY_CENTER_THREE_LINE2_Y};

  for (uint32_t i = 0U; i < row_count; i++)
  {
    if (rows[i][0] != '\0' && visible_count < DISPLAY_MAX_ROWS)
    {
      strncpy(visible_rows[visible_count], rows[i], DISPLAY_ROW_BUFFER_LENGTH - 1U);
      visible_count++;
    }
  }

  if (visible_count == 0U)
  {
    return;
  }

  if (visible_count == 1U)
  {
    if (display_rendering_lyric)
    {
      Display_PrintHighlightedSingleLine(visible_rows[0]);
      return;
    }
    y_positions = one_line_y;
  }
  else if (visible_count == 2U)
  {
    y_positions = two_line_y;
  }
  else
  {
    y_positions = three_line_y;
  }

  for (uint32_t i = 0U; i < visible_count; i++)
  {
    if (display_rendering_lyric && visible_count > 2U)
    {
      static const int8_t shake_pattern_x[] = {0, 2, -1, 1, -2, 1, 0, -1};
      static const int8_t shake_pattern_y[] = {0, -1, 1, 0, -1, 1, 0, 1};
      uint8_t shake_seed = (uint8_t)(current_effect_frame.value + current_effect_style.value + (i * 5U));
      int8_t shake_x = shake_pattern_x[shake_seed & DISPLAY_SHAKE_PATTERN_MASK];
      int8_t shake_y = shake_pattern_y[(shake_seed + (i * DISPLAY_MAX_ROWS)) & DISPLAY_SHAKE_PATTERN_MASK];

      if (((shake_seed >> 1U) & 1U) != 0U)
      {
        shake_x = (int8_t)-shake_x;
      }
      Display_PrintLineOffset(y_positions[i], visible_rows[i], shake_x, shake_y);
    }
    else
    {
      Display_PrintLine(y_positions[i], visible_rows[i]);
    }
  }

  if (display_rendering_lyric && visible_count > 1U)
  {
    AppDisplayEffects_DrawMultiLineAccent((AppDisplayLineSpan){y_positions[0], y_positions[visible_count - 1U]},
                                          current_effect_frame,
                                          current_effect_style);
  }
}

static void Display_PrintHighlightedSingleLine(const char *text)
{
  SSD1306_Font_t *font;
  size_t visible_glyphs = AppText_Utf8GlyphCount(text);
  uint16_t text_width;
  uint8_t text_x;
  uint8_t text_y;
  uint8_t padding = (uint8_t)(2U + (current_effect_frame.value & 1U));
  uint8_t shape_x;
  uint8_t shape_y;
  uint8_t shape_w;
  uint8_t shape_h;
  const char *cursor = text;
  uint8_t x;
  uint32_t glyph_count = 0U;
  bool large_font_fits;
  bool force_small_font;
  bool inverted_text;
  SSD1306_COLOR_t text_color;
  int8_t shake_x = 0;
  int8_t shake_y = 0;
  AppDisplayRect highlight_rect;

  if (visible_glyphs > DISPLAY_CHARS_PER_LINE)
  {
    visible_glyphs = DISPLAY_CHARS_PER_LINE;
  }

  large_font_fits = (visible_glyphs * SSD1306_Font_11x18.FontWidth) <= (SSD1306_WIDTH - DISPLAY_LARGE_FONT_MARGIN_X);
  force_small_font = (current_effect_style.value == LYRIC_STYLE_BURST || current_effect_style.value == LYRIC_STYLE_ZIGZAG || current_effect_style.value == LYRIC_STYLE_SHADOW);
  inverted_text = !(current_effect_style.value == LYRIC_STYLE_TREMOR || current_effect_style.value == LYRIC_STYLE_STRIPE || current_effect_style.value == LYRIC_STYLE_ZIGZAG || current_effect_style.value == LYRIC_STYLE_SHADOW);
  font = (large_font_fits && !force_small_font) ? &SSD1306_Font_11x18 : &SSD1306_Font_7x10;
  text_color = inverted_text ? SSD1306_COLOR_BLACK : SSD1306_COLOR_WHITE;

  text_width = (uint16_t)(visible_glyphs * font->FontWidth);
  text_x = (uint8_t)((SSD1306_WIDTH - text_width) / 2U);
  text_y = (uint8_t)((SSD1306_HEIGHT - font->FontHeight) / 2U);

  shape_x = (text_x > (padding + 1U)) ? (uint8_t)(text_x - padding - 1U) : 0U;
  shape_y = (text_y > (padding + 1U)) ? (uint8_t)(text_y - padding - 1U) : 0U;
  shape_w = (uint8_t)(text_width + (padding * 2U));
  shape_h = (uint8_t)(font->FontHeight + (padding * 2U) + 1U);

  if (current_effect_style.value == LYRIC_STYLE_TREMOR || current_effect_style.value == LYRIC_STYLE_STRIPE || current_effect_style.value == LYRIC_STYLE_DOTTED)
  {
    static const int8_t shake_pattern_x[] = {0, 2, -1, 1, -2, 1, 0, -1};
    static const int8_t shake_pattern_y[] = {0, -1, 1, 0, -1, 1, 0, 1};
    uint8_t shake_index = (uint8_t)(current_effect_frame.value & DISPLAY_SHAKE_PATTERN_MASK);

    shake_x = shake_pattern_x[shake_index];
    shake_y = shake_pattern_y[shake_index];
  }

  if ((shape_x + shape_w) >= SSD1306_WIDTH)
  {
    shape_w = (uint8_t)(SSD1306_WIDTH - shape_x - 1U);
  }
  if ((shape_y + shape_h) >= SSD1306_HEIGHT)
  {
    shape_h = (uint8_t)(SSD1306_HEIGHT - shape_y - 1U);
  }

  highlight_rect.origin.x = shape_x;
  highlight_rect.origin.y = shape_y;
  highlight_rect.size.width = shape_w;
  highlight_rect.size.height = shape_h;
  AppDisplayEffects_DrawHighlight(highlight_rect, current_effect_frame, current_effect_style);

  if ((int16_t)text_x + shake_x < 0)
  {
    text_x = 0U;
  }
  else if ((text_x + text_width + shake_x) >= SSD1306_WIDTH)
  {
    text_x = (uint8_t)(SSD1306_WIDTH - text_width - 1U);
  }
  else
  {
    text_x = (uint8_t)(text_x + shake_x);
  }

  if ((int16_t)text_y + shake_y < 0)
  {
    text_y = 0U;
  }
  else if ((text_y + font->FontHeight + shake_y) >= SSD1306_HEIGHT)
  {
    text_y = (uint8_t)(SSD1306_HEIGHT - font->FontHeight - 1U);
  }
  else
  {
    text_y = (uint8_t)(text_y + shake_y);
  }

  x = text_x;
  while (*cursor != '\0' && glyph_count < DISPLAY_CHARS_PER_LINE)
  {
    uint32_t codepoint = Utf8_DecodeGlyph(&cursor);

    Display_DrawCodepoint(codepoint, x, text_y, font, text_color);
    x += font->FontWidth;
    glyph_count++;
  }
}
