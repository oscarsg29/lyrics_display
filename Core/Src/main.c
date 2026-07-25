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
static void App_NextTrack(void);
static void App_BackTrack(void);
static void App_PlaySelectedTrack(void);
static void Lyrics_Update(uint32_t now);
static FRESULT Lyrics_LoadForSelectedTrack(void);
static bool Lyrics_ParseLine(const char *line, uint32_t *timestamp_ms, const char **text);
static bool Lyrics_ParseMetadataLine(const char *line);
static bool Lyrics_ParseTimestampTag(const char **cursor, uint32_t *timestamp_ms);
static bool Lyrics_ParseDuration(const char *value, uint32_t *duration_ms);
static void CopyDisplayText(char *destination, size_t destination_size, const char *source);
static void Display_ShowLyricText(const char *text);
static void Display_PrintWrappedText(const char *text);
static void App_SetPlaybackLed(bool is_playing);
static void BuildLrcFileName(const char *mp3_name, char *lrc_name, size_t lrc_name_size);
static bool StringEndsWithIgnoreCase(const char *text, const char *suffix);
static void SortTrackList(void);

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

    if (((file_info.fattrib & AM_DIR) == 0U) && StringEndsWithIgnoreCase(file_info.fname, ".mp3"))
    {
      strncpy(track_names[track_count], file_info.fname, TRACK_NAME_LENGTH - 1U);
      track_names[track_count][TRACK_NAME_LENGTH - 1U] = '\0';
      track_count++;
    }
  }

  f_closedir(&dir);
  SortTrackList();
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
  if (!display_ready)
  {
    return;
  }

  SSD1306_Fill(SSD1306_COLOR_BLACK);
  Display_PrintLine(0U, line0);
  Display_PrintLine(11U, line1);
  Display_PrintLine(21U, line2);
  SSD1306_UpdateScreen();
}

static void Display_PrintLine(uint8_t y, const char *text)
{
  char clipped[DISPLAY_CHARS_PER_LINE + 1U];

  if (text == NULL)
  {
    text = "";
  }

  strncpy(clipped, text, DISPLAY_CHARS_PER_LINE);
  clipped[DISPLAY_CHARS_PER_LINE] = '\0';

  SSD1306_GotoXY(0U, y);
  SSD1306_Puts(clipped, &SSD1306_Font_7x10, SSD1306_COLOR_WHITE);
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
    return;
  }

  current_lyric_index = next_index;

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
  BuildLrcFileName(track_names[selected_track], file_name, sizeof(file_name));

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

      if (Lyrics_ParseMetadataLine(line))
      {
      }
      else if (lyric_count < MAX_LYRIC_LINES && Lyrics_ParseLine(line, &timestamp_ms, &text) && text[0] != '\0')
      {
        lyric_lines[lyric_count].timestamp_ms = timestamp_ms;
        CopyDisplayText(lyric_lines[lyric_count].text, sizeof(lyric_lines[lyric_count].text), text);
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

static bool Lyrics_ParseLine(const char *line, uint32_t *timestamp_ms, const char **text)
{
  uint32_t ignored_timestamp;
  uint32_t minutes = 0U;
  uint32_t seconds = 0U;
  uint32_t fraction = 0U;
  uint32_t fraction_digits = 0U;
  const char *p = line;

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

  while (Lyrics_ParseTimestampTag(&p, &ignored_timestamp))
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

static bool Lyrics_ParseMetadataLine(const char *line)
{
  const char *p = line;
  const char *value;
  char *target = NULL;
  char metadata_text[TRACK_NAME_LENGTH];
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
    target = lyric_artist;
  }
  else if (p[1] == 't' && p[2] == 'i' && p[3] == ':')
  {
    target = lyric_title;
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
    (void)Lyrics_ParseDuration(metadata_text, &lyric_duration_ms);
  }
  else
  {
    CopyDisplayText(target, TRACK_NAME_LENGTH, metadata_text);
  }
  return true;
}

static bool Lyrics_ParseTimestampTag(const char **cursor, uint32_t *timestamp_ms)
{
  uint32_t minutes = 0U;
  uint32_t seconds = 0U;
  uint32_t fraction = 0U;
  uint32_t fraction_digits = 0U;
  const char *p = *cursor;

  if (p == NULL || *p != '[')
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

static bool Lyrics_ParseDuration(const char *value, uint32_t *duration_ms)
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

static void CopyDisplayText(char *destination, size_t destination_size, const char *source)
{
  size_t write_index = 0U;

  if (destination_size == 0U)
  {
    return;
  }

  while (*source != '\0' && write_index < (destination_size - 1U))
  {
    uint8_t byte = (uint8_t)*source;
    char output = '\0';

    if (byte < 0x80U)
    {
      output = (char)byte;
      source++;
    }
    else if (byte == 0xC3U && source[1] != '\0')
    {
      uint8_t next = (uint8_t)source[1];

      switch (next)
      {
        case 0x81U:
        case 0xA1U:
          output = 'A';
          break;
        case 0x89U:
        case 0xA9U:
          output = 'E';
          break;
        case 0x8DU:
        case 0xADU:
          output = 'I';
          break;
        case 0x93U:
        case 0xB3U:
          output = 'O';
          break;
        case 0x9AU:
        case 0xBAU:
        case 0x9CU:
        case 0xBCU:
          output = 'U';
          break;
        case 0x91U:
        case 0xB1U:
          output = 'N';
          break;
        default:
          output = '?';
          break;
      }
      source += 2;
    }
    else
    {
      output = '?';
      source++;
    }

    if (output != '\0')
    {
      destination[write_index++] = output;
    }
  }

  destination[write_index] = '\0';
}

static void Display_ShowLyricText(const char *text)
{
  if (!display_ready)
  {
    return;
  }

  SSD1306_Fill(SSD1306_COLOR_BLACK);
  Display_PrintWrappedText(text);
  SSD1306_UpdateScreen();
}

static void Display_PrintWrappedText(const char *text)
{
  char rows[3][DISPLAY_CHARS_PER_LINE + 1U] = {{0}};
  uint32_t row = 0U;
  size_t row_length = 0U;
  const char *p = text;

  while (*p != '\0' && row < 3U)
  {
    char word[DISPLAY_CHARS_PER_LINE + 1U];
    size_t word_length = 0U;

    while (*p == ' ')
    {
      p++;
    }

    if (*p == '\0')
    {
      break;
    }

    while (*p != '\0' && *p != ' ' && word_length < DISPLAY_CHARS_PER_LINE)
    {
      word[word_length++] = *p;
      p++;
    }
    word[word_length] = '\0';

    while (*p != '\0' && *p != ' ')
    {
      p++;
    }

    if (word_length == 0U)
    {
      continue;
    }

    if (row_length == 0U)
    {
      strncpy(rows[row], word, DISPLAY_CHARS_PER_LINE);
      rows[row][DISPLAY_CHARS_PER_LINE] = '\0';
      row_length = strlen(rows[row]);
    }
    else if ((row_length + 1U + word_length) <= DISPLAY_CHARS_PER_LINE)
    {
      rows[row][row_length++] = ' ';
      rows[row][row_length] = '\0';
      strncat(rows[row], word, DISPLAY_CHARS_PER_LINE - row_length);
      row_length = strlen(rows[row]);
    }
    else
    {
      row++;
      row_length = 0U;
      if (row < 3U)
      {
        strncpy(rows[row], word, DISPLAY_CHARS_PER_LINE);
        rows[row][DISPLAY_CHARS_PER_LINE] = '\0';
        row_length = strlen(rows[row]);
      }
    }
  }

  Display_PrintLine(0U, rows[0]);
  Display_PrintLine(11U, rows[1]);
  Display_PrintLine(21U, rows[2]);
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

static void BuildLrcFileName(const char *mp3_name, char *lrc_name, size_t lrc_name_size)
{
  size_t length;

  if (lrc_name_size == 0U)
  {
    return;
  }

  strncpy(lrc_name, mp3_name, lrc_name_size - 1U);
  lrc_name[lrc_name_size - 1U] = '\0';

  length = strlen(lrc_name);
  if (length >= 4U && StringEndsWithIgnoreCase(lrc_name, ".mp3"))
  {
    lrc_name[length - 4U] = '\0';
  }

  strncat(lrc_name, ".lrc", lrc_name_size - strlen(lrc_name) - 1U);
}

static bool StringEndsWithIgnoreCase(const char *text, const char *suffix)
{
  size_t text_length = strlen(text);
  size_t suffix_length = strlen(suffix);

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

static void SortTrackList(void)
{
  for (uint32_t i = 0U; i < track_count; i++)
  {
    for (uint32_t j = i + 1U; j < track_count; j++)
    {
      if (strcmp(track_names[i], track_names[j]) > 0)
      {
        char temp[TRACK_NAME_LENGTH];
        strncpy(temp, track_names[i], sizeof(temp));
        strncpy(track_names[i], track_names[j], TRACK_NAME_LENGTH);
        strncpy(track_names[j], temp, TRACK_NAME_LENGTH);
      }
    }
  }
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
