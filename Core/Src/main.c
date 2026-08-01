/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2026 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "i2c.h"
#include "spi.h"
#include "tim.h"
#include "gpio.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "ff.h"
#include "sd_diskio.h"
#include "ssd1306.h"
#include "app_logic.h"
#include <stdio.h>
#include <string.h>

/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */
typedef enum
{
  BUTTON_RELEASED,
  BUTTON_PRESS_DEBOUNCE,
  BUTTON_PRESSED,
  BUTTON_RELEASE_DEBOUNCE
} ButtonDebounceState;

typedef enum
{
  BUTTON_ACTION_NEXT,
  BUTTON_ACTION_PLAY,
  BUTTON_ACTION_BACK
} ButtonAction;

typedef struct
{
  GPIO_TypeDef *port;
  uint16_t pin;
  ButtonAction action;
  ButtonDebounceState state;
  uint32_t state_changed_at;
} ButtonDebouncer;

typedef enum
{
  APP_MODE_BROWSER,
  APP_MODE_LYRICS
} AppMode;

typedef struct
{
  uint32_t timestamp_ms;
  char text[48];
} LyricLine;

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
#define BUTTON_DEBOUNCE_MS 20U
#define MAX_TRACKS 32U
#define TRACK_NAME_LENGTH 64U
#define MAX_LYRIC_LINES 48U
#define DISPLAY_CHARS_PER_LINE 18U
#define DISPLAY_ROW_BUFFER_LENGTH 56U
#define LYRIC_ANIMATION_INTERVAL_MS 140U
#define LYRIC_ANIMATION_COUNT 10U
#define LYRIC_BACKGROUND_ANIMATION_COUNT 6U
#define LYRIC_PARTICLE_COUNT 22U
#define LED_PLAYING_STATE GPIO_PIN_RESET
#define LED_STOPPED_STATE GPIO_PIN_SET

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */
static ButtonDebouncer buttons[] =
{
  {NEXT_GPIO_Port, NEXT_Pin, BUTTON_ACTION_NEXT, BUTTON_RELEASED, 0U},
  {PLAY_RESUME_GPIO_Port, PLAY_RESUME_Pin, BUTTON_ACTION_PLAY, BUTTON_RELEASED, 0U},
  {BACK_GPIO_Port, BACK_Pin, BUTTON_ACTION_BACK, BUTTON_RELEASED, 0U},
};
static bool display_ready = false;
static FRESULT sd_scan_result = FR_NOT_READY;
static char track_names[MAX_TRACKS][TRACK_NAME_LENGTH];
static uint32_t track_count = 0U;
static uint32_t selected_track = 0U;
static LyricLine lyric_lines[MAX_LYRIC_LINES];
static uint32_t lyric_count = 0U;
static uint32_t lyric_started_at = 0U;
static int32_t current_lyric_index = -1;
static AppMode app_mode = APP_MODE_BROWSER;
static char lyric_artist[TRACK_NAME_LENGTH];
static char lyric_title[TRACK_NAME_LENGTH];
static uint32_t lyric_duration_ms = 0U;
static bool lyric_led_on = false;
static uint32_t lyric_last_animation_at = 0U;
static uint8_t lyric_animation_frame = 0U;
static uint8_t lyric_animation_style = 0U;
static uint8_t lyric_background_style = 0U;
static uint32_t lyric_animation_rng = 0xA53C9E17U;
static bool display_rendering_lyric = false;

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
/* USER CODE BEGIN PFP */
static void ButtonDebounce_Process(ButtonDebouncer *button, uint32_t now);
static void ButtonDebounce_Init(void);
static void ButtonPressed_Handler(ButtonAction action);
static FRESULT SD_LoadTrackList(void);
static void Display_ShowSdScanning(void);
static void Display_ShowTrackBrowser(void);
static void Display_ShowMessage(const char *line0, const char *line1, const char *line2);
static void Display_PrintLine(uint8_t y, const char *text);
static void Display_PrintLineOffset(uint8_t y, const char *text, int8_t offset_x, int8_t offset_y);
static void Display_PrintCenteredRows(char rows[][DISPLAY_ROW_BUFFER_LENGTH], uint32_t row_count);
static void App_NextTrack(void);
static void App_BackTrack(void);
static void App_PlaySelectedTrack(void);
static void Lyrics_Update(uint32_t now);
static FRESULT Lyrics_LoadForSelectedTrack(void);
static uint32_t Utf8_DecodeGlyph(const char **text);
static size_t Utf8_GlyphLength(const char *text);
static size_t Utf8_GlyphCount(const char *text);
static void Display_DrawCodepoint(uint32_t codepoint, uint8_t x, uint8_t y, SSD1306_Font_t *font, SSD1306_COLOR_t color);
static void Display_DrawInvertedExclamation(uint8_t x, uint8_t y, SSD1306_Font_t *font, SSD1306_COLOR_t foreground, SSD1306_COLOR_t background);
static void Display_DrawInvertedQuestion(uint8_t x, uint8_t y, SSD1306_Font_t *font, SSD1306_COLOR_t foreground, SSD1306_COLOR_t background);
static void Display_DrawAccent(uint8_t x, uint8_t y, SSD1306_Font_t *font, char accent, SSD1306_COLOR_t color);
static void Display_ShowLyricText(const char *text);
static void Display_PrintWrappedText(const char *text);
static void Display_PrintHighlightedSingleLine(const char *text);
static void Display_DrawAnimatedHighlight(uint8_t x, uint8_t y, uint8_t w, uint8_t h, uint8_t frame, uint8_t style);
static void Display_DrawLyricBackground(uint8_t frame, uint8_t style);
static void Display_DrawLyricParticles(uint8_t frame, uint8_t style);
static void Display_DrawSonarBackground(uint8_t frame, uint8_t style);
static void Display_DrawTremorBackground(uint8_t frame, uint8_t style);
static void Display_DrawGradientBackground(uint8_t frame, uint8_t style);
static void Display_DrawRainBackground(uint8_t frame, uint8_t style);
static void Display_DrawWaveDotBackground(uint8_t frame, uint8_t style);
static void Display_DrawLyricParticle(int16_t x, int16_t y, uint8_t radius);
static void Display_DrawWaveHighlight(uint8_t x, uint8_t y, uint8_t w, uint8_t h, uint8_t frame);
static void Display_DrawMultiLineAccent(uint8_t first_y, uint8_t last_y, uint8_t frame, uint8_t style);
static void Display_DrawBurstHighlight(uint8_t x, uint8_t y, uint8_t w, uint8_t h, uint8_t frame);
static void Display_DrawWingHighlight(uint8_t x, uint8_t y, uint8_t w, uint8_t h, uint8_t frame);
static void Display_DrawStripeHighlight(uint8_t x, uint8_t y, uint8_t w, uint8_t h, uint8_t frame);
static void Display_DrawSparkHighlight(uint8_t x, uint8_t y, uint8_t w, uint8_t h, uint8_t frame);
static void Display_DrawDoubleFrameHighlight(uint8_t x, uint8_t y, uint8_t w, uint8_t h, uint8_t frame);
static void Display_DrawZigZagHighlight(uint8_t x, uint8_t y, uint8_t w, uint8_t h, uint8_t frame);
static void Display_DrawBandHighlight(uint8_t x, uint8_t y, uint8_t w, uint8_t h, uint8_t frame);
static void Display_DrawDottedHighlight(uint8_t x, uint8_t y, uint8_t w, uint8_t h, uint8_t frame);
static void Display_DrawShadowHighlight(uint8_t x, uint8_t y, uint8_t w, uint8_t h, uint8_t frame);
static uint8_t Lyric_SelectAnimation(void);
static void App_SetPlaybackLed(bool is_playing);
/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{

  /* USER CODE BEGIN 1 */

  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_I2C2_Init();
  MX_SPI2_Init();
  MX_TIM2_Init();
  /* USER CODE BEGIN 2 */
  App_SetPlaybackLed(false);
  ButtonDebounce_Init();
  SD_SPI_Setup();
  display_ready = SSD1306_Init(&hi2c2);
  Display_ShowSdScanning();
  sd_scan_result = SD_LoadTrackList();
  Display_ShowTrackBrowser();

  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
    uint32_t now = HAL_GetTick();

    for (uint32_t i = 0U; i < (sizeof(buttons) / sizeof(buttons[0])); i++)
    {
      ButtonDebounce_Process(&buttons[i], now);
    }

    Lyrics_Update(now);
  }
  /* USER CODE END 3 */
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.HSEPredivValue = RCC_HSE_PREDIV_DIV1;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLMUL = RCC_PLL_MUL9;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK)
  {
    Error_Handler();
  }
}

/* USER CODE BEGIN 4 */
static void ButtonDebounce_Init(void)
{
  uint32_t now = HAL_GetTick();

  for (uint32_t i = 0U; i < (sizeof(buttons) / sizeof(buttons[0])); i++)
  {
    buttons[i].state = (HAL_GPIO_ReadPin(buttons[i].port, buttons[i].pin) == GPIO_PIN_SET)
                     ? BUTTON_PRESSED
                     : BUTTON_RELEASED;
    buttons[i].state_changed_at = now;
  }
}

static void ButtonDebounce_Process(ButtonDebouncer *button, uint32_t now)
{
  GPIO_PinState sample = HAL_GPIO_ReadPin(button->port, button->pin);

  switch (button->state)
  {
    case BUTTON_RELEASED:
      if (sample == GPIO_PIN_SET)
      {
        button->state = BUTTON_PRESS_DEBOUNCE;
        button->state_changed_at = now;
      }
      break;

    case BUTTON_PRESS_DEBOUNCE:
      if (sample == GPIO_PIN_RESET)
      {
        button->state = BUTTON_RELEASED;
      }
      else if ((now - button->state_changed_at) >= BUTTON_DEBOUNCE_MS)
      {
        button->state = BUTTON_PRESSED;
        ButtonPressed_Handler(button->action);
      }
      break;

    case BUTTON_PRESSED:
      if (sample == GPIO_PIN_RESET)
      {
        button->state = BUTTON_RELEASE_DEBOUNCE;
        button->state_changed_at = now;
      }
      break;

    case BUTTON_RELEASE_DEBOUNCE:
      if (sample == GPIO_PIN_SET)
      {
        button->state = BUTTON_PRESSED;
      }
      else if ((now - button->state_changed_at) >= BUTTON_DEBOUNCE_MS)
      {
        button->state = BUTTON_RELEASED;
      }
      break;

    default:
      button->state = BUTTON_RELEASED;
      break;
  }
}

static void ButtonPressed_Handler(ButtonAction action)
{
  switch (action)
  {
    case BUTTON_ACTION_NEXT:
      App_NextTrack();
      break;

    case BUTTON_ACTION_PLAY:
      App_PlaySelectedTrack();
      break;

    case BUTTON_ACTION_BACK:
      App_BackTrack();
      break;

    default:
      break;
  }
}

static FRESULT SD_LoadTrackList(void)
{
  static FATFS fs;
  DIR dir;
  FILINFO file_info;
  FRESULT result;

  track_count = 0U;
  selected_track = 0U;
  lyric_count = 0U;
  current_lyric_index = -2;
  lyric_artist[0] = '\0';
  lyric_title[0] = '\0';
  lyric_duration_ms = 0U;
  app_mode = APP_MODE_BROWSER;
  App_SetPlaybackLed(false);

  result = f_mount(&fs, "", 1U);
  if (result != FR_OK)
  {
    return result;
  }

  result = f_opendir(&dir, "");
  if (result != FR_OK)
  {
    return result;
  }

  while (track_count < MAX_TRACKS)
  {
    result = f_readdir(&dir, &file_info);
    if (result != FR_OK || file_info.fname[0] == '\0')
    {
      break;
    }

    if (((file_info.fattrib & AM_DIR) == 0U) && AppLogic_StringEndsWithIgnoreCase(file_info.fname, ".mp3"))
    {
      strncpy(track_names[track_count], file_info.fname, TRACK_NAME_LENGTH - 1U);
      track_names[track_count][TRACK_NAME_LENGTH - 1U] = '\0';
      track_count++;
    }
  }

  f_closedir(&dir);
  AppLogic_SortTrackNames(&track_names[0][0], track_count, TRACK_NAME_LENGTH);
  return result;
}

static void Display_ShowSdScanning(void)
{
  Display_ShowMessage("SD card", "Scanning...", "");
}

static void Display_ShowTrackBrowser(void)
{
  char header[DISPLAY_CHARS_PER_LINE + 1U];

  app_mode = APP_MODE_BROWSER;
  current_lyric_index = -2;

  if (sd_scan_result != FR_OK)
  {
    char diagnostic[DISPLAY_CHARS_PER_LINE + 1U];
    snprintf(diagnostic, sizeof(diagnostic), "SD:%u C:%u R:%02X", (unsigned int)SD_GetLastErrorStep(), (unsigned int)SD_GetLastCommand(), (unsigned int)SD_GetLastCommandResponse());
    Display_ShowMessage("SD mount error", diagnostic, "");
    return;
  }

  if (track_count == 0U)
  {
    Display_ShowMessage("No MP3 files", "Root only", "");
    return;
  }

  snprintf(header, sizeof(header), "%lu/%lu MP3", (unsigned long)(selected_track + 1U), (unsigned long)track_count);

  if (!display_ready)
  {
    return;
  }

  SSD1306_Fill(SSD1306_COLOR_BLACK);
  Display_PrintLine(0U, header);
  Display_PrintLine(11U, track_names[selected_track]);
  if (strlen(track_names[selected_track]) > DISPLAY_CHARS_PER_LINE)
  {
    Display_PrintLine(21U, &track_names[selected_track][DISPLAY_CHARS_PER_LINE]);
  }
  SSD1306_UpdateScreen();
}

static void Display_ShowMessage(const char *line0, const char *line1, const char *line2)
{
  char rows[3][DISPLAY_ROW_BUFFER_LENGTH] = {{0}};

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
  Display_PrintCenteredRows(rows, 3U);
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

  visible_glyphs = Utf8_GlyphCount(text);
  if (visible_glyphs > DISPLAY_CHARS_PER_LINE)
  {
    visible_glyphs = DISPLAY_CHARS_PER_LINE;
  }

  text_width = (uint16_t)(visible_glyphs * SSD1306_Font_7x10.FontWidth);
  shifted_x = (int16_t)(((SSD1306_WIDTH - text_width) / 2U) + offset_x);
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

static void App_NextTrack(void)
{
  if (sd_scan_result != FR_OK || track_count == 0U)
  {
    Display_ShowTrackBrowser();
    return;
  }

  selected_track = (selected_track + 1U) % track_count;
  Display_ShowTrackBrowser();
}

static void App_BackTrack(void)
{
  if (sd_scan_result != FR_OK || track_count == 0U)
  {
    Display_ShowTrackBrowser();
    return;
  }

  selected_track = (selected_track == 0U) ? (track_count - 1U) : (selected_track - 1U);
  Display_ShowTrackBrowser();
}

static void App_PlaySelectedTrack(void)
{
  FRESULT result;

  if (sd_scan_result != FR_OK || track_count == 0U)
  {
    Display_ShowTrackBrowser();
    return;
  }

  Display_ShowMessage("Lyrics", "Loading...", "");
  result = Lyrics_LoadForSelectedTrack();
  if (result == FR_NO_FILE)
  {
    Display_ShowMessage("No LRC file", track_names[selected_track], "");
    return;
  }
  if (result != FR_OK)
  {
    char error_line[DISPLAY_CHARS_PER_LINE + 1U];
    snprintf(error_line, sizeof(error_line), "LRC error: %u", (unsigned int)result);
    Display_ShowMessage(error_line, track_names[selected_track], "");
    return;
  }
  if (lyric_count == 0U)
  {
    Display_ShowMessage(lyric_artist[0] != '\0' ? lyric_artist : "No lyric lines",
                        lyric_title[0] != '\0' ? lyric_title : track_names[selected_track],
                        lyric_artist[0] != '\0' || lyric_title[0] != '\0' ? "No timed lyrics" : "");
    return;
  }

  lyric_started_at = HAL_GetTick();
  current_lyric_index = -2;
  app_mode = APP_MODE_LYRICS;
  App_SetPlaybackLed(true);
  Lyrics_Update(lyric_started_at);
}

static void Lyrics_Update(uint32_t now)
{
  uint32_t elapsed_ms;
  int32_t next_index = -1;
  bool lyric_finished = false;

  if (app_mode != APP_MODE_LYRICS || lyric_count == 0U)
  {
    return;
  }

  elapsed_ms = now - lyric_started_at;
  for (uint32_t i = 0U; i < lyric_count; i++)
  {
    if (lyric_lines[i].timestamp_ms <= elapsed_ms)
    {
      next_index = (int32_t)i;
    }
    else
    {
      break;
    }
  }

  if (lyric_duration_ms > 0U)
  {
    lyric_finished = elapsed_ms >= lyric_duration_ms;
  }
  else if (next_index == (int32_t)(lyric_count - 1U))
  {
    lyric_finished = elapsed_ms >= (lyric_lines[lyric_count - 1U].timestamp_ms + 3000U);
  }

  if (lyric_finished)
  {
    App_SetPlaybackLed(false);
  }
  else
  {
    App_SetPlaybackLed(true);
  }

  if (next_index == current_lyric_index)
  {
    if (next_index >= 0 && (now - lyric_last_animation_at) >= LYRIC_ANIMATION_INTERVAL_MS)
    {
      lyric_last_animation_at = now;
      lyric_animation_frame++;
      Display_ShowLyricText(lyric_lines[next_index].text);
    }
    return;
  }

  current_lyric_index = next_index;
  lyric_last_animation_at = now;
  lyric_animation_frame = 0U;
  if (next_index >= 0)
  {
    lyric_animation_style = Lyric_SelectAnimation();
    lyric_background_style = (uint8_t)(lyric_animation_rng % LYRIC_BACKGROUND_ANIMATION_COUNT);
  }

  if (next_index < 0)
  {
    Display_ShowMessage(lyric_artist, lyric_title, "Waiting...");
    return;
  }

  Display_ShowLyricText(lyric_lines[next_index].text);
}

static FRESULT Lyrics_LoadForSelectedTrack(void)
{
  FIL file;
  FRESULT result;
  UINT bytes_read;
  char file_name[TRACK_NAME_LENGTH];
  char line[96];
  uint32_t line_length = 0U;
  uint8_t byte;

  lyric_count = 0U;
  lyric_artist[0] = '\0';
  lyric_title[0] = '\0';
  lyric_duration_ms = 0U;
  AppLogic_BuildLrcFileName(track_names[selected_track], file_name, sizeof(file_name));

  result = f_open(&file, file_name, FA_READ);
  if (result != FR_OK)
  {
    return result;
  }

  do
  {
    result = f_read(&file, &byte, 1U, &bytes_read);
    if (result != FR_OK)
    {
      break;
    }

    if (bytes_read == 1U && byte != '\n')
    {
      if (byte != '\r' && line_length < (sizeof(line) - 1U))
      {
        line[line_length++] = (char)byte;
      }
      continue;
    }

    line[line_length] = '\0';
    if (line_length > 0U)
    {
      uint32_t timestamp_ms;
      const char *text;

      if (AppLogic_ParseMetadataLine(line, lyric_artist, sizeof(lyric_artist), lyric_title, sizeof(lyric_title), &lyric_duration_ms))
      {
      }
      else if (lyric_count < MAX_LYRIC_LINES && AppLogic_ParseLyricLine(line, &timestamp_ms, &text) && text[0] != '\0')
      {
        lyric_lines[lyric_count].timestamp_ms = timestamp_ms;
        AppLogic_CopyDisplayText(lyric_lines[lyric_count].text, sizeof(lyric_lines[lyric_count].text), text);
        if (lyric_lines[lyric_count].text[0] != '\0')
        {
          lyric_count++;
        }
      }
    }

    line_length = 0U;
  } while (bytes_read == 1U && lyric_count < MAX_LYRIC_LINES);

  f_close(&file);
  return result;
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

static size_t Utf8_GlyphCount(const char *text)
{
  size_t count = 0U;

  while (text != NULL && *text != '\0')
  {
    text += Utf8_GlyphLength(text);
    count++;
  }

  return count;
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

static void Display_ShowLyricText(const char *text)
{
  if (!display_ready)
  {
    return;
  }

  SSD1306_Fill(SSD1306_COLOR_BLACK);
  display_rendering_lyric = true;
  Display_DrawLyricBackground(lyric_animation_frame, lyric_background_style);
  Display_PrintWrappedText(text);
  display_rendering_lyric = false;
  SSD1306_UpdateScreen();
}

static void Display_PrintWrappedText(const char *text)
{
  char rows[3][DISPLAY_ROW_BUFFER_LENGTH] = {{0}};
  uint32_t row = 0U;
  size_t row_bytes = 0U;
  size_t row_glyphs = 0U;
  const char *p = text;

  while (*p != '\0' && row < 3U)
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
      size_t glyph_length = Utf8_GlyphLength(p);

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
      p += Utf8_GlyphLength(p);
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
      row_glyphs = Utf8_GlyphCount(rows[row]);
    }
    else if ((row_glyphs + 1U + word_glyphs) <= DISPLAY_CHARS_PER_LINE)
    {
      if ((row_bytes + 1U + word_bytes) < DISPLAY_ROW_BUFFER_LENGTH)
      {
        rows[row][row_bytes++] = ' ';
        rows[row][row_bytes] = '\0';
        strncat(rows[row], word, DISPLAY_ROW_BUFFER_LENGTH - row_bytes - 1U);
        row_bytes = strlen(rows[row]);
        row_glyphs = Utf8_GlyphCount(rows[row]);
      }
    }
    else
    {
      row++;
      row_bytes = 0U;
      row_glyphs = 0U;
      if (row < 3U)
      {
        strncpy(rows[row], word, DISPLAY_ROW_BUFFER_LENGTH - 1U);
        rows[row][DISPLAY_ROW_BUFFER_LENGTH - 1U] = '\0';
        row_bytes = strlen(rows[row]);
        row_glyphs = Utf8_GlyphCount(rows[row]);
      }
    }
  }

  Display_PrintCenteredRows(rows, 3U);
}

static void Display_PrintCenteredRows(char rows[][DISPLAY_ROW_BUFFER_LENGTH], uint32_t row_count)
{
  char visible_rows[3][DISPLAY_ROW_BUFFER_LENGTH] = {{0}};
  uint32_t visible_count = 0U;
  const uint8_t *y_positions;
  static const uint8_t one_line_y[] = {16U};
  static const uint8_t two_line_y[] = {10U, 21U};
  static const uint8_t three_line_y[] = {4U, 14U, 23U};

  for (uint32_t i = 0U; i < row_count; i++)
  {
    if (rows[i][0] != '\0' && visible_count < 3U)
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
      uint8_t shake_seed = (uint8_t)(lyric_animation_frame + lyric_animation_style + (i * 5U));
      int8_t shake_x = shake_pattern_x[shake_seed & 7U];
      int8_t shake_y = shake_pattern_y[(shake_seed + (i * 3U)) & 7U];

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
    Display_DrawMultiLineAccent(y_positions[0], y_positions[visible_count - 1U], lyric_animation_frame, lyric_animation_style);
  }
}

static void Display_PrintHighlightedSingleLine(const char *text)
{
  SSD1306_Font_t *font;
  size_t visible_glyphs = Utf8_GlyphCount(text);
  uint16_t text_width;
  uint8_t text_x;
  uint8_t text_y;
  uint8_t padding = (uint8_t)(2U + (lyric_animation_frame & 1U));
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

  if (visible_glyphs > DISPLAY_CHARS_PER_LINE)
  {
    visible_glyphs = DISPLAY_CHARS_PER_LINE;
  }

  large_font_fits = (visible_glyphs * SSD1306_Font_11x18.FontWidth) <= (SSD1306_WIDTH - 8U);
  force_small_font = (lyric_animation_style == 3U || lyric_animation_style == 6U || lyric_animation_style == 8U);
  inverted_text = !(lyric_animation_style == 1U || lyric_animation_style == 4U || lyric_animation_style == 6U || lyric_animation_style == 8U);
  font = (large_font_fits && !force_small_font) ? &SSD1306_Font_11x18 : &SSD1306_Font_7x10;
  text_color = inverted_text ? SSD1306_COLOR_BLACK : SSD1306_COLOR_WHITE;

  text_width = (uint16_t)(visible_glyphs * font->FontWidth);
  text_x = (uint8_t)((SSD1306_WIDTH - text_width) / 2U);
  text_y = (uint8_t)((SSD1306_HEIGHT - font->FontHeight) / 2U);

  shape_x = (text_x > (padding + 1U)) ? (uint8_t)(text_x - padding - 1U) : 0U;
  shape_y = (text_y > (padding + 1U)) ? (uint8_t)(text_y - padding - 1U) : 0U;
  shape_w = (uint8_t)(text_width + (padding * 2U));
  shape_h = (uint8_t)(font->FontHeight + (padding * 2U) + 1U);

  if (lyric_animation_style == 1U || lyric_animation_style == 4U || lyric_animation_style == 7U)
  {
    static const int8_t shake_pattern_x[] = {0, 2, -1, 1, -2, 1, 0, -1};
    static const int8_t shake_pattern_y[] = {0, -1, 1, 0, -1, 1, 0, 1};
    uint8_t shake_index = (uint8_t)(lyric_animation_frame & 7U);

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

  Display_DrawAnimatedHighlight(shape_x, shape_y, shape_w, shape_h, lyric_animation_frame, lyric_animation_style);

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

static void Display_DrawWaveHighlight(uint8_t x, uint8_t y, uint8_t w, uint8_t h, uint8_t frame)
{
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

static void Display_DrawMultiLineAccent(uint8_t first_y, uint8_t last_y, uint8_t frame, uint8_t style)
{
  uint8_t top = (first_y > 2U) ? (uint8_t)(first_y - 2U) : 0U;
  uint8_t bottom = (uint8_t)(last_y + SSD1306_Font_7x10.FontHeight + 1U);
  uint8_t pulse = (uint8_t)(frame & 3U);

  if (bottom >= SSD1306_HEIGHT)
  {
    bottom = SSD1306_HEIGHT - 1U;
  }

  switch (style % 5U)
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
      for (uint8_t x = (uint8_t)(frame & 3U); x < SSD1306_WIDTH; x = (uint8_t)(x + 12U))
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

static void Display_DrawAnimatedHighlight(uint8_t x, uint8_t y, uint8_t w, uint8_t h, uint8_t frame, uint8_t style)
{
  switch (style % LYRIC_ANIMATION_COUNT)
  {
    case 0U:
      Display_DrawWaveHighlight(x, y, w, h, frame);
      break;
    case 1U:
      Display_DrawBurstHighlight(x, y, w, h, frame);
      break;
    case 2U:
      Display_DrawWingHighlight(x, y, w, h, frame);
      break;
    case 3U:
      Display_DrawStripeHighlight(x, y, w, h, frame);
      break;
    case 4U:
      Display_DrawSparkHighlight(x, y, w, h, frame);
      break;
    case 5U:
      Display_DrawDoubleFrameHighlight(x, y, w, h, frame);
      break;
    case 6U:
      Display_DrawZigZagHighlight(x, y, w, h, frame);
      break;
    case 7U:
      Display_DrawBandHighlight(x, y, w, h, frame);
      break;
    case 8U:
      Display_DrawDottedHighlight(x, y, w, h, frame);
      break;
    default:
      Display_DrawShadowHighlight(x, y, w, h, frame);
      break;
  }
}

static void Display_DrawLyricBackground(uint8_t frame, uint8_t style)
{
  switch (style % LYRIC_BACKGROUND_ANIMATION_COUNT)
  {
    case 0U:
      Display_DrawLyricParticles(frame, style);
      break;
    case 1U:
      Display_DrawSonarBackground(frame, style);
      break;
    case 2U:
      Display_DrawTremorBackground(frame, style);
      break;
    case 3U:
      Display_DrawGradientBackground(frame, style);
      break;
    case 4U:
      Display_DrawRainBackground(frame, style);
      break;
    default:
      Display_DrawWaveDotBackground(frame, style);
      break;
  }
}

static void Display_DrawLyricParticles(uint8_t frame, uint8_t style)
{
  uint32_t seed = 0x9E3779B9U ^ ((uint32_t)style * 0x85EBCA6BU);

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

    seed ^= seed << 13;
    seed ^= seed >> 17;
    seed ^= seed << 5;
    base_x = (uint8_t)(seed % SSD1306_WIDTH);

    seed ^= seed << 13;
    seed ^= seed >> 17;
    seed ^= seed << 5;
    base_y = (uint8_t)(seed % SSD1306_HEIGHT);

    speed_x = (uint8_t)(1U + ((seed >> 8) & 3U));
    speed_y = (uint8_t)(1U + ((seed >> 12) & 1U));
    wobble = (uint8_t)(((frame + (i * 3U)) & 7U) < 4U ? (frame & 3U) : (3U - (frame & 3U)));

    x = (int16_t)((base_x + (frame * speed_x) + (i * 11U)) % SSD1306_WIDTH);
    y = (int16_t)((base_y + (frame * speed_y) + wobble + (i * 5U)) % SSD1306_HEIGHT);

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

    Display_DrawLyricParticle(x, y, radius);
  }
}

static void Display_DrawSonarBackground(uint8_t frame, uint8_t style)
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
    Display_DrawLyricParticle(x, y, (uint8_t)(i % 3U == 0U ? 1U : 0U));
  }
}

static void Display_DrawTremorBackground(uint8_t frame, uint8_t style)
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
      Display_DrawLyricParticle((int16_t)(x + 2), y, 1U);
    }
  }
}

static void Display_DrawGradientBackground(uint8_t frame, uint8_t style)
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

static void Display_DrawRainBackground(uint8_t frame, uint8_t style)
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
      Display_DrawLyricParticle(x, y, 1U);
    }
  }
}

static void Display_DrawWaveDotBackground(uint8_t frame, uint8_t style)
{
  for (uint8_t x = (uint8_t)(frame & 3U); x < SSD1306_WIDTH; x = (uint8_t)(x + 6U))
  {
    uint8_t wave_y = (uint8_t)((((x / 6U) + frame + style) & 7U) < 4U ? 8U : 20U);
    uint8_t y = (uint8_t)(wave_y + (((x + frame) & 3U) == 0U ? 2U : 0U));

    SSD1306_DrawPixel(x, y, SSD1306_COLOR_WHITE);
    SSD1306_DrawPixel((uint16_t)(SSD1306_WIDTH - 1U - x), (uint16_t)(SSD1306_HEIGHT - 1U - y), SSD1306_COLOR_WHITE);

    if (((x + frame) % 18U) == 0U)
    {
      Display_DrawLyricParticle(x, y, 1U);
    }
  }
}

static void Display_DrawLyricParticle(int16_t x, int16_t y, uint8_t radius)
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

static void Display_DrawBurstHighlight(uint8_t x, uint8_t y, uint8_t w, uint8_t h, uint8_t frame)
{
  uint8_t center_x = (uint8_t)(x + (w / 2U));
  uint8_t center_y = (uint8_t)(y + (h / 2U));
  uint8_t reach = (uint8_t)(2U + (frame & 3U));

  SSD1306_DrawRectangle(x, y, w, h, SSD1306_COLOR_WHITE);
  SSD1306_DrawLine(center_x, y, center_x, (uint16_t)(y > reach ? y - reach : 0U), SSD1306_COLOR_WHITE);
  SSD1306_DrawLine(center_x, (uint16_t)(y + h), center_x, (uint16_t)(y + h + reach), SSD1306_COLOR_WHITE);
  SSD1306_DrawLine(x, center_y, (uint16_t)(x > reach ? x - reach : 0U), center_y, SSD1306_COLOR_WHITE);
  SSD1306_DrawLine((uint16_t)(x + w), center_y, (uint16_t)(x + w + reach), center_y, SSD1306_COLOR_WHITE);
  SSD1306_DrawPixel((uint16_t)(center_x > reach ? center_x - reach : center_x), (uint16_t)(center_y > reach ? center_y - reach : center_y), SSD1306_COLOR_WHITE);
  SSD1306_DrawPixel((uint16_t)(center_x + reach), (uint16_t)(center_y > reach ? center_y - reach : center_y), SSD1306_COLOR_WHITE);
  SSD1306_DrawPixel((uint16_t)(center_x > reach ? center_x - reach : center_x), (uint16_t)(center_y + reach), SSD1306_COLOR_WHITE);
  SSD1306_DrawPixel((uint16_t)(center_x + reach), (uint16_t)(center_y + reach), SSD1306_COLOR_WHITE);
}

static void Display_DrawWingHighlight(uint8_t x, uint8_t y, uint8_t w, uint8_t h, uint8_t frame)
{
  uint8_t wing = (uint8_t)(3U + ((frame & 3U) * 2U));
  uint8_t mid_y = (uint8_t)(y + (h / 2U));

  SSD1306_DrawFilledRectangle(x, y, w, h, SSD1306_COLOR_WHITE);
  SSD1306_DrawLine((uint16_t)(x > wing ? x - wing : 0U), mid_y, x, y, SSD1306_COLOR_WHITE);
  SSD1306_DrawLine((uint16_t)(x > wing ? x - wing : 0U), mid_y, x, (uint16_t)(y + h), SSD1306_COLOR_WHITE);
  SSD1306_DrawLine((uint16_t)(x + w), y, (uint16_t)(x + w + wing), mid_y, SSD1306_COLOR_WHITE);
  SSD1306_DrawLine((uint16_t)(x + w), (uint16_t)(y + h), (uint16_t)(x + w + wing), mid_y, SSD1306_COLOR_WHITE);
}

static void Display_DrawStripeHighlight(uint8_t x, uint8_t y, uint8_t w, uint8_t h, uint8_t frame)
{
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

static void Display_DrawSparkHighlight(uint8_t x, uint8_t y, uint8_t w, uint8_t h, uint8_t frame)
{
  uint8_t sparkle = (uint8_t)(2U + (frame & 3U));

  SSD1306_DrawRectangle(x, y, w, h, SSD1306_COLOR_WHITE);
  SSD1306_DrawPixel((uint16_t)(x + sparkle), (uint16_t)(y > 1U ? y - 1U : y), SSD1306_COLOR_WHITE);
  SSD1306_DrawPixel((uint16_t)(x + w - sparkle), (uint16_t)(y > 1U ? y - 1U : y), SSD1306_COLOR_WHITE);
  SSD1306_DrawPixel((uint16_t)(x + sparkle), (uint16_t)(y + h + 1U), SSD1306_COLOR_WHITE);
  SSD1306_DrawPixel((uint16_t)(x + w - sparkle), (uint16_t)(y + h + 1U), SSD1306_COLOR_WHITE);
  SSD1306_DrawPixel((uint16_t)(x + sparkle + 1U), y, SSD1306_COLOR_WHITE);
  SSD1306_DrawPixel((uint16_t)(x + w - sparkle - 1U), (uint16_t)(y + h), SSD1306_COLOR_WHITE);
  SSD1306_DrawPixel((uint16_t)(x > sparkle ? x - sparkle : x), (uint16_t)(y + (h / 2U)), SSD1306_COLOR_WHITE);
  SSD1306_DrawPixel((uint16_t)(x + w + sparkle), (uint16_t)(y + (h / 2U)), SSD1306_COLOR_WHITE);
}

static void Display_DrawDoubleFrameHighlight(uint8_t x, uint8_t y, uint8_t w, uint8_t h, uint8_t frame)
{
  uint8_t inset = (uint8_t)(1U + (frame & 1U));

  SSD1306_DrawFilledRectangle(x, y, w, h, SSD1306_COLOR_WHITE);
  SSD1306_DrawRectangle(x, y, w, h, SSD1306_COLOR_BLACK);
  if (w > (inset * 2U) && h > (inset * 2U))
  {
    SSD1306_DrawRectangle((uint16_t)(x + inset), (uint16_t)(y + inset), (uint16_t)(w - (inset * 2U)), (uint16_t)(h - (inset * 2U)), SSD1306_COLOR_BLACK);
  }
}

static void Display_DrawZigZagHighlight(uint8_t x, uint8_t y, uint8_t w, uint8_t h, uint8_t frame)
{
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

static void Display_DrawBandHighlight(uint8_t x, uint8_t y, uint8_t w, uint8_t h, uint8_t frame)
{
  uint8_t band = (uint8_t)(2U + (frame % 5U));

  SSD1306_DrawFilledRectangle(x, y, w, h, SSD1306_COLOR_WHITE);
  SSD1306_DrawLine(x, (uint16_t)(y + band), (uint16_t)(x + w), (uint16_t)(y + band), SSD1306_COLOR_BLACK);
  if (h > band + 4U)
  {
    SSD1306_DrawLine(x, (uint16_t)(y + h - band), (uint16_t)(x + w), (uint16_t)(y + h - band), SSD1306_COLOR_BLACK);
  }
}

static void Display_DrawDottedHighlight(uint8_t x, uint8_t y, uint8_t w, uint8_t h, uint8_t frame)
{
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

static void Display_DrawShadowHighlight(uint8_t x, uint8_t y, uint8_t w, uint8_t h, uint8_t frame)
{
  uint8_t shadow = (uint8_t)(1U + (frame & 1U));

  if ((x + w + shadow) < SSD1306_WIDTH && (y + h + shadow) < SSD1306_HEIGHT)
  {
    SSD1306_DrawRectangle((uint16_t)(x + shadow), (uint16_t)(y + shadow), w, h, SSD1306_COLOR_WHITE);
  }
  SSD1306_DrawFilledRectangle(x, y, w, h, SSD1306_COLOR_WHITE);
  SSD1306_DrawLine(x, y, (uint16_t)(x + w), y, SSD1306_COLOR_BLACK);
  SSD1306_DrawLine(x, y, x, (uint16_t)(y + h), SSD1306_COLOR_BLACK);
}

static uint8_t Lyric_SelectAnimation(void)
{
  uint8_t previous = lyric_animation_style;

  lyric_animation_rng ^= lyric_animation_rng << 13;
  lyric_animation_rng ^= lyric_animation_rng >> 17;
  lyric_animation_rng ^= lyric_animation_rng << 5;
  lyric_animation_rng ^= HAL_GetTick();

  lyric_animation_style = (uint8_t)(lyric_animation_rng % LYRIC_ANIMATION_COUNT);
  if (lyric_animation_style == previous)
  {
    lyric_animation_style = (uint8_t)((lyric_animation_style + 3U) % LYRIC_ANIMATION_COUNT);
  }

  return lyric_animation_style;
}

static void App_SetPlaybackLed(bool is_playing)
{
  if (lyric_led_on == is_playing)
  {
    return;
  }

  lyric_led_on = is_playing;
  HAL_GPIO_WritePin(LED_D2_GPIO_Port, LED_D2_Pin, is_playing ? LED_PLAYING_STATE : LED_STOPPED_STATE);
}

/* USER CODE END 4 */

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  __disable_irq();
  while (1)
  {
  }
  /* USER CODE END Error_Handler_Debug */
}
#ifdef USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
