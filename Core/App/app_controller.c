#include "app_controller.h"

#include "app_config.h"
#include "app_display.h"
#include "app_lyrics.h"
#include "app_tracks.h"
#include "button_input.h"
#include "playback_led.h"
#include "storage_port.h"

#include <stdbool.h>

#define LYRIC_ANIMATION_INTERVAL_MS 140U
#define LYRIC_ANIMATION_COUNT 10U
#define LYRIC_BACKGROUND_ANIMATION_COUNT 6U
#define RANDOM_XORSHIFT_LEFT_A 13U
#define RANDOM_XORSHIFT_RIGHT_B 17U
#define RANDOM_XORSHIFT_LEFT_C 5U

typedef enum
{
  APP_MODE_BROWSER,
  APP_MODE_LYRICS
} AppMode;

static AppTracksStatus track_scan_status = APP_TRACKS_STATUS_MOUNT_FAILED;
static char track_names[APP_MAX_TRACKS][APP_TRACK_NAME_LENGTH];
static uint32_t track_count = 0U;
static uint32_t selected_track = 0U;
static AppLyricLine lyric_lines[APP_MAX_LYRIC_LINES];
static AppLyricsDocument lyric_document = {
  .lines = lyric_lines,
  .max_lines = APP_MAX_LYRIC_LINES,
  .count = 0U,
};
static AppTimeMs lyric_started_at = {0U};
static int32_t current_lyric_index = -1;
static AppMode app_mode = APP_MODE_BROWSER;
static AppTimeMs lyric_last_animation_at = {0U};
static AppAnimationFrame lyric_animation_frame = {0U};
static AppAnimationStyle lyric_animation_style = {0U};
static AppAnimationStyle lyric_background_style = {0U};
static uint32_t lyric_animation_rng = 0xA53C9E17U;
static AppTimeMs app_now = {0U};

static void ButtonPressed_Handler(ButtonInputAction action);
static AppTracksStatus SD_LoadTrackList(void);
static void App_NextTrack(void);
static void App_BackTrack(void);
static void App_PlaySelectedTrack(void);
static void Lyrics_Update(AppTimeMs now);
static void App_ShowTrackBrowser(void);
static void App_ShowCurrentLyric(void);
static uint32_t Random_Xorshift32(uint32_t seed);
static AppAnimationStyle Lyric_SelectAnimation(void);

void AppController_Init(AppTimeMs now)
{
  app_now = now;
  PlaybackLed_Set(false);
  ButtonInput_Init(now);
  (void)AppDisplay_Init();
  AppDisplay_ShowBootInfo();
  StoragePort_Setup();
  AppDisplay_ShowSdScanning();
  track_scan_status = SD_LoadTrackList();
  App_ShowTrackBrowser();
}

void AppController_Process(AppTimeMs now)
{
  app_now = now;
  ButtonInput_Process(now, ButtonPressed_Handler);
  Lyrics_Update(now);
}

static void ButtonPressed_Handler(ButtonInputAction action)
{
  switch (action)
  {
    case BUTTON_INPUT_ACTION_NEXT:
      App_NextTrack();
      break;

    case BUTTON_INPUT_ACTION_PLAY:
      App_PlaySelectedTrack();
      break;

    case BUTTON_INPUT_ACTION_BACK:
      App_BackTrack();
      break;

    default:
      break;
  }
}

static AppTracksStatus SD_LoadTrackList(void)
{
  track_count = 0U;
  selected_track = 0U;
  AppLyrics_ClearDocument(&lyric_document);
  current_lyric_index = -2;
  app_mode = APP_MODE_BROWSER;
  PlaybackLed_Set(false);

  return AppTracks_LoadFromRoot(&track_names[0][0], APP_MAX_TRACKS, APP_TRACK_NAME_LENGTH, &track_count);
}

static void App_NextTrack(void)
{
  if (track_scan_status != APP_TRACKS_STATUS_OK || track_count == 0U)
  {
    App_ShowTrackBrowser();
    return;
  }

  selected_track = (selected_track + 1U) % track_count;
  App_ShowTrackBrowser();
}

static void App_BackTrack(void)
{
  if (track_scan_status != APP_TRACKS_STATUS_OK || track_count == 0U)
  {
    App_ShowTrackBrowser();
    return;
  }

  selected_track = (selected_track == 0U) ? (track_count - 1U) : (selected_track - 1U);
  App_ShowTrackBrowser();
}

static void App_PlaySelectedTrack(void)
{
  AppLyricsStatus result;

  if (track_scan_status != APP_TRACKS_STATUS_OK || track_count == 0U)
  {
    App_ShowTrackBrowser();
    return;
  }

  AppDisplay_ShowMessage("Lyrics", "Loading...", "");
  result = AppLyrics_LoadForTrack(track_names[selected_track], &lyric_document);
  if (result == APP_LYRICS_STATUS_FILE_NOT_FOUND)
  {
    AppDisplay_ShowMessage("No LRC file", track_names[selected_track], "");
    return;
  }
  if (result == APP_LYRICS_STATUS_NO_TIMED_LINES)
  {
    AppDisplay_ShowMessage(lyric_document.metadata.artist[0] != '\0' ? lyric_document.metadata.artist : "No lyric lines",
                           lyric_document.metadata.title[0] != '\0' ? lyric_document.metadata.title : track_names[selected_track],
                           lyric_document.metadata.artist[0] != '\0' || lyric_document.metadata.title[0] != '\0' ? "No timed lyrics" : "");
    return;
  }
  if (result != APP_LYRICS_STATUS_OK)
  {
    switch (result)
    {
      case APP_LYRICS_STATUS_INVALID_PARAMETER:
        AppDisplay_ShowMessage("LRC error", "Bad parameter", "");
        break;
      case APP_LYRICS_STATUS_OPEN_FAILED:
        AppDisplay_ShowMessage("LRC open error", track_names[selected_track], "");
        break;
      case APP_LYRICS_STATUS_READ_FAILED:
        AppDisplay_ShowMessage("LRC read error", track_names[selected_track], "");
        break;
      default:
        AppDisplay_ShowMessage("LRC error", track_names[selected_track], "");
        break;
    }
    return;
  }

  lyric_started_at = app_now;
  current_lyric_index = -2;
  app_mode = APP_MODE_LYRICS;
  PlaybackLed_Set(true);
  Lyrics_Update(lyric_started_at);
}

static void Lyrics_Update(AppTimeMs now)
{
  uint32_t elapsed_ms;
  int32_t next_index = -1;
  bool lyric_finished = false;

  if (app_mode != APP_MODE_LYRICS || lyric_document.count == 0U)
  {
    return;
  }

  elapsed_ms = now.value - lyric_started_at.value;
  for (uint32_t i = 0U; i < lyric_document.count; i++)
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

  if (lyric_document.metadata.duration_ms > 0U)
  {
    lyric_finished = elapsed_ms >= lyric_document.metadata.duration_ms;
  }
  else if (next_index == (int32_t)(lyric_document.count - 1U))
  {
    lyric_finished = elapsed_ms >= (lyric_lines[lyric_document.count - 1U].timestamp_ms + APP_LYRIC_END_GRACE_MS);
  }

  PlaybackLed_Set(!lyric_finished);

  if (next_index == current_lyric_index)
  {
    if (next_index >= 0 && (now.value - lyric_last_animation_at.value) >= LYRIC_ANIMATION_INTERVAL_MS)
    {
      lyric_last_animation_at = now;
      lyric_animation_frame.value++;
      App_ShowCurrentLyric();
    }
    return;
  }

  current_lyric_index = next_index;
  lyric_last_animation_at = now;
  lyric_animation_frame.value = 0U;
  if (next_index >= 0)
  {
    lyric_animation_style = Lyric_SelectAnimation();
    lyric_background_style.value = (uint8_t)(lyric_animation_rng % LYRIC_BACKGROUND_ANIMATION_COUNT);
  }

  if (next_index < 0)
  {
    AppDisplay_ShowMessage(lyric_document.metadata.artist, lyric_document.metadata.title, "Waiting...");
    return;
  }

  App_ShowCurrentLyric();
}

static void App_ShowTrackBrowser(void)
{
  AppTrackBrowserView view;

  app_mode = APP_MODE_BROWSER;
  current_lyric_index = -2;
  view.status = track_scan_status;
  view.selected_track_name = track_count > 0U ? track_names[selected_track] : "";
  view.selected_track_index = selected_track;
  view.track_count = track_count;
  AppDisplay_ShowTrackBrowser(&view);
}

static void App_ShowCurrentLyric(void)
{
  AppLyricRenderView view;

  if (current_lyric_index < 0)
  {
    return;
  }

  view.text = lyric_lines[current_lyric_index].text;
  view.frame = lyric_animation_frame;
  view.text_style = lyric_animation_style;
  view.background_style = lyric_background_style;
  AppDisplay_ShowLyricText(&view);
}

static uint32_t Random_Xorshift32(uint32_t seed)
{
  seed ^= seed << RANDOM_XORSHIFT_LEFT_A;
  seed ^= seed >> RANDOM_XORSHIFT_RIGHT_B;
  seed ^= seed << RANDOM_XORSHIFT_LEFT_C;

  return seed;
}

static AppAnimationStyle Lyric_SelectAnimation(void)
{
  uint8_t previous = lyric_animation_style.value;

  lyric_animation_rng = Random_Xorshift32(lyric_animation_rng);
  lyric_animation_rng ^= app_now.value;

  lyric_animation_style.value = (uint8_t)(lyric_animation_rng % LYRIC_ANIMATION_COUNT);
  if (lyric_animation_style.value == previous)
  {
    lyric_animation_style.value = (uint8_t)((lyric_animation_style.value + 3U) % LYRIC_ANIMATION_COUNT);
  }

  return lyric_animation_style;
}
