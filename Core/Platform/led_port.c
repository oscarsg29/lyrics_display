#include "led_port.h"

#include "main.h"

#define LED_PLAYING_STATE GPIO_PIN_RESET
#define LED_STOPPED_STATE GPIO_PIN_SET

void LedPort_SetPlayback(bool is_playing)
{
  HAL_GPIO_WritePin(LED_D2_GPIO_Port, LED_D2_Pin, is_playing ? LED_PLAYING_STATE : LED_STOPPED_STATE);
}
