#ifndef BUTTON_PORT_H
#define BUTTON_PORT_H

#include "button_input.h"

#include <stdbool.h>

bool ButtonPort_IsPressed(ButtonInputAction action);

#endif
