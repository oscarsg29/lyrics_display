#ifndef DISPLAY_PORT_H
#define DISPLAY_PORT_H

#include <stdbool.h>
#include <stdint.h>

bool DisplayPort_Init(void);
void DisplayPort_DelayMs(uint32_t duration_ms);

#endif
