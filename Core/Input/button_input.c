#include "button_input.h"

#include "main.h"

#define BUTTON_DEBOUNCE_MS 20U

typedef enum
{
  BUTTON_RELEASED,
  BUTTON_PRESS_DEBOUNCE,
  BUTTON_PRESSED,
  BUTTON_RELEASE_DEBOUNCE
} ButtonDebounceState;

typedef struct
{
  GPIO_TypeDef *port;
  uint16_t pin;
  ButtonInputAction action;
  ButtonDebounceState state;
  uint32_t state_changed_at;
} ButtonDebouncer;

static ButtonDebouncer buttons[] =
{
  {NEXT_GPIO_Port, NEXT_Pin, BUTTON_INPUT_ACTION_NEXT, BUTTON_RELEASED, 0U},
  {PLAY_RESUME_GPIO_Port, PLAY_RESUME_Pin, BUTTON_INPUT_ACTION_PLAY, BUTTON_RELEASED, 0U},
  {BACK_GPIO_Port, BACK_Pin, BUTTON_INPUT_ACTION_BACK, BUTTON_RELEASED, 0U},
};

static void ButtonInput_ProcessOne(ButtonDebouncer *button, uint32_t now, ButtonInputHandler handler);

void ButtonInput_Init(void)
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

void ButtonInput_Process(uint32_t now, ButtonInputHandler handler)
{
  for (uint32_t i = 0U; i < (sizeof(buttons) / sizeof(buttons[0])); i++)
  {
    ButtonInput_ProcessOne(&buttons[i], now, handler);
  }
}

static void ButtonInput_ProcessOne(ButtonDebouncer *button, uint32_t now, ButtonInputHandler handler)
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
        if (handler != NULL)
        {
          handler(button->action);
        }
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
