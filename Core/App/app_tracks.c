#include "app_tracks.h"

#include "app_config.h"
#include "app_logic.h"
#include "ff.h"

#include <string.h>

AppTracksStatus AppTracks_LoadFromRoot(char *track_names,
                                       uint32_t max_tracks,
                                       size_t track_name_length,
                                       uint32_t *track_count)
{
  static FATFS fs;
  DIR dir;
  FILINFO file_info;
  FRESULT result;

  if (track_count != NULL)
  {
    *track_count = 0U;
  }

  if (track_names == NULL || track_count == NULL || track_name_length == 0U)
  {
    return APP_TRACKS_STATUS_INVALID_PARAMETER;
  }

  result = f_mount(&fs, "", 1U);
  if (result != FR_OK)
  {
    return APP_TRACKS_STATUS_MOUNT_FAILED;
  }

  result = f_opendir(&dir, "");
  if (result != FR_OK)
  {
    return APP_TRACKS_STATUS_OPEN_DIR_FAILED;
  }

  while (*track_count < max_tracks)
  {
    result = f_readdir(&dir, &file_info);
    if (result != FR_OK || file_info.fname[0] == '\0')
    {
      break;
    }

    if (((file_info.fattrib & AM_DIR) == 0U) && AppLogic_StringEndsWithIgnoreCase(file_info.fname, APP_MP3_EXTENSION))
    {
      char *track_name = &track_names[*track_count * track_name_length];
      strncpy(track_name, file_info.fname, track_name_length - 1U);
      track_name[track_name_length - 1U] = '\0';
      (*track_count)++;
    }
  }

  f_closedir(&dir);
  if (result != FR_OK)
  {
    return APP_TRACKS_STATUS_READ_DIR_FAILED;
  }

  AppLogic_SortTrackNames(track_names, *track_count, track_name_length);
  return (*track_count == 0U) ? APP_TRACKS_STATUS_NO_TRACKS : APP_TRACKS_STATUS_OK;
}
