#include "button_port.h"

#include "main.h"

bool ButtonPort_IsPressed(ButtonInputAction action)
{
  switch (action)
  {
    case BUTTON_INPUT_ACTION_NEXT:
      return HAL_GPIO_ReadPin(NEXT_GPIO_Port, NEXT_Pin) == GPIO_PIN_SET;

    case BUTTON_INPUT_ACTION_PLAY:
      return HAL_GPIO_ReadPin(PLAY_RESUME_GPIO_Port, PLAY_RESUME_Pin) == GPIO_PIN_SET;

    case BUTTON_INPUT_ACTION_BACK:
      return HAL_GPIO_ReadPin(BACK_GPIO_Port, BACK_Pin) == GPIO_PIN_SET;

    default:
      return false;
  }
}
