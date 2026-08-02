#include "display_port.h"

#include "ssd1306.h"

bool DisplayPort_Init(void)
{
  return SSD1306_Init();
}
