#ifndef APP_DISPLAY_EFFECTS_H
#define APP_DISPLAY_EFFECTS_H

#include "app_types.h"

void AppDisplayEffects_DrawBackground(AppAnimationFrame frame, AppAnimationStyle style);
void AppDisplayEffects_DrawHighlight(AppDisplayRect rect, AppAnimationFrame frame, AppAnimationStyle style);
void AppDisplayEffects_DrawMultiLineAccent(AppDisplayLineSpan span, AppAnimationFrame frame, AppAnimationStyle style);

#endif
