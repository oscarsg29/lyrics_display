#ifndef APP_DISPLAY_H
#define APP_DISPLAY_H

#include "app_tracks.h"
#include "app_types.h"

#include <stdbool.h>

typedef struct
{
  AppTracksStatus status;
  const char *selected_track_name;
  uint32_t selected_track_index;
  uint32_t track_count;
} AppTrackBrowserView;

bool AppDisplay_Init(void);
void AppDisplay_ShowBootInfo(void);
void AppDisplay_ShowSdScanning(void);
void AppDisplay_ShowTrackBrowser(const AppTrackBrowserView *view);
void AppDisplay_ShowMessage(const char *line0, const char *line1, const char *line2);
void AppDisplay_ShowLyricText(const AppLyricRenderView *view);

#endif
