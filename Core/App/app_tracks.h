#ifndef APP_TRACKS_H
#define APP_TRACKS_H

#include <stddef.h>
#include <stdint.h>

typedef enum
{
  APP_TRACKS_STATUS_OK,
  APP_TRACKS_STATUS_INVALID_PARAMETER,
  APP_TRACKS_STATUS_MOUNT_FAILED,
  APP_TRACKS_STATUS_OPEN_DIR_FAILED,
  APP_TRACKS_STATUS_READ_DIR_FAILED,
  APP_TRACKS_STATUS_NO_TRACKS
} AppTracksStatus;

AppTracksStatus AppTracks_LoadFromRoot(char *track_names,
                                       uint32_t max_tracks,
                                       size_t track_name_length,
                                       uint32_t *track_count);

#endif
