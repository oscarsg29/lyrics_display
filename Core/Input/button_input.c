#include "button_input.h"

#include "button_port.h"

#include <stddef.h>

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
  ButtonInputAction action;
  ButtonDebounceState state;
  AppTimeMs state_changed_at;
} ButtonDebouncer;

static ButtonDebouncer buttons[] =
{
  {BUTTON_INPUT_ACTION_NEXT, BUTTON_RELEASED, {0U}},
  {BUTTON_INPUT_ACTION_PLAY, BUTTON_RELEASED, {0U}},
  {BUTTON_INPUT_ACTION_BACK, BUTTON_RELEASED, {0U}},
};

static void ButtonInput_ProcessOne(ButtonDebouncer *button, AppTimeMs now, ButtonInputHandler handler);

void ButtonInput_Init(AppTimeMs now)
{
  for (uint32_t i = 0U; i < (sizeof(buttons) / sizeof(buttons[0])); i++)
  {
    buttons[i].state = ButtonPort_IsPressed(buttons[i].action)
                     ? BUTTON_PRESSED
                     : BUTTON_RELEASED;
    buttons[i].state_changed_at = now;
  }
}

void ButtonInput_Process(AppTimeMs now, ButtonInputHandler handler)
{
  for (uint32_t i = 0U; i < (sizeof(buttons) / sizeof(buttons[0])); i++)
  {
    ButtonInput_ProcessOne(&buttons[i], now, handler);
  }
}

static void ButtonInput_ProcessOne(ButtonDebouncer *button, AppTimeMs now, ButtonInputHandler handler)
{
  bool is_pressed = ButtonPort_IsPressed(button->action);

  switch (button->state)
  {
    case BUTTON_RELEASED:
      if (is_pressed)
      {
        button->state = BUTTON_PRESS_DEBOUNCE;
        button->state_changed_at = now;
      }
      break;

    case BUTTON_PRESS_DEBOUNCE:
      if (!is_pressed)
      {
        button->state = BUTTON_RELEASED;
      }
      else if ((now.value - button->state_changed_at.value) >= BUTTON_DEBOUNCE_MS)
      {
        button->state = BUTTON_PRESSED;
        if (handler != NULL)
        {
          handler(button->action);
        }
      }
      break;

    case BUTTON_PRESSED:
      if (!is_pressed)
      {
        button->state = BUTTON_RELEASE_DEBOUNCE;
        button->state_changed_at = now;
      }
      break;

    case BUTTON_RELEASE_DEBOUNCE:
      if (is_pressed)
      {
        button->state = BUTTON_PRESSED;
      }
      else if ((now.value - button->state_changed_at.value) >= BUTTON_DEBOUNCE_MS)
      {
        button->state = BUTTON_RELEASED;
      }
      break;

    default:
      button->state = BUTTON_RELEASED;
      break;
  }
}
