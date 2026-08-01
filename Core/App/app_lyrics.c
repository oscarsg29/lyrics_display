#include "app_lyrics.h"

#include "app_logic.h"
#include "ff.h"

void AppLyrics_ClearDocument(AppLyricsDocument *document)
{
  if (document == NULL)
  {
    return;
  }

  document->count = 0U;
  AppLogic_ClearMetadata(&document->metadata);
}

AppLyricsStatus AppLyrics_LoadForTrack(const char *track_name, AppLyricsDocument *document)
{
  FIL file;
  FRESULT result;
  UINT bytes_read;
  char file_name[APP_TRACK_NAME_LENGTH];
  char line[APP_LRC_READ_LINE_LENGTH];
  uint32_t line_length = 0U;
  uint8_t byte;

  AppLyrics_ClearDocument(document);
  if (track_name == NULL || document == NULL || document->lines == NULL)
  {
    return APP_LYRICS_STATUS_INVALID_PARAMETER;
  }

  AppLogic_BuildLrcFileName(track_name, file_name, sizeof(file_name));

  result = f_open(&file, file_name, FA_READ);
  if (result != FR_OK)
  {
    return (result == FR_NO_FILE) ? APP_LYRICS_STATUS_FILE_NOT_FOUND : APP_LYRICS_STATUS_OPEN_FAILED;
  }

  do
  {
    result = f_read(&file, &byte, 1U, &bytes_read);
    if (result != FR_OK)
    {
      break;
    }

    if (bytes_read == 1U && byte != '\n')
    {
      if (byte != '\r' && line_length < (sizeof(line) - 1U))
      {
        line[line_length++] = (char)byte;
      }
      continue;
    }

    line[line_length] = '\0';
    if (line_length > 0U)
    {
      uint32_t timestamp_ms;
      const char *text;

      if (AppLogic_ParseMetadataLine(line, &document->metadata))
      {
      }
      else if (document->count < document->max_lines && AppLogic_ParseLyricLine(line, &timestamp_ms, &text) && text[0] != '\0')
      {
        document->lines[document->count].timestamp_ms = timestamp_ms;
        AppLogic_CopyDisplayText(document->lines[document->count].text, sizeof(document->lines[document->count].text), text);
        if (document->lines[document->count].text[0] != '\0')
        {
          document->count++;
        }
      }
    }

    line_length = 0U;
  } while (bytes_read == 1U && document->count < document->max_lines);

  f_close(&file);
  if (result != FR_OK)
  {
    return APP_LYRICS_STATUS_READ_FAILED;
  }

  return (document->count == 0U) ? APP_LYRICS_STATUS_NO_TIMED_LINES : APP_LYRICS_STATUS_OK;
}
