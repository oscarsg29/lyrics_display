#ifndef BUTTON_INPUT_H
#define BUTTON_INPUT_H

#include <stdint.h>

typedef enum
{
  BUTTON_INPUT_ACTION_NEXT,
  BUTTON_INPUT_ACTION_PLAY,
  BUTTON_INPUT_ACTION_BACK
} ButtonInputAction;

typedef void (*ButtonInputHandler)(ButtonInputAction action);

void ButtonInput_Init(void);
void ButtonInput_Process(uint32_t now, ButtonInputHandler handler);

#endif
