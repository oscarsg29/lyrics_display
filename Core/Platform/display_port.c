#include "display_port.h"

#include "stm32f1xx_hal.h"
#include "ssd1306.h"

bool DisplayPort_Init(void)
{
  return SSD1306_Init();
}

void DisplayPort_DelayMs(uint32_t duration_ms)
{
  HAL_Delay(duration_ms);
}
