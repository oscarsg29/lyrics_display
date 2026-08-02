#ifndef BUTTON_INPUT_H
#define BUTTON_INPUT_H

#include "app_types.h"

typedef enum
{
  BUTTON_INPUT_ACTION_NEXT,
  BUTTON_INPUT_ACTION_PLAY,
  BUTTON_INPUT_ACTION_BACK
} ButtonInputAction;

typedef void (*ButtonInputHandler)(ButtonInputAction action);

void ButtonInput_Init(AppTimeMs now);
void ButtonInput_Process(AppTimeMs now, ButtonInputHandler handler);

#endif
