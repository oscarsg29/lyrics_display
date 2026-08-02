#include "playback_led.h"

#include "led_port.h"

static bool playback_led_on = false;

void PlaybackLed_Set(bool is_playing)
{
  if (playback_led_on == is_playing)
  {
    return;
  }

  playback_led_on = is_playing;
  LedPort_SetPlayback(is_playing);
}
