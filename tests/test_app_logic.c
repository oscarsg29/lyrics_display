#include "app_config.h"
#include "app_logic.h"
#include "app_lyrics.h"
#include "app_tracks.h"
#include "app_text.h"
#include "test_assert.h"

#include <stdbool.h>
#include <string.h>

#define TEST_SHORT_TRACK_NAME_LENGTH 10U
#define TEST_DISPLAY_TEXT_LENGTH 32U
#define TEST_TRACK_FIXTURE_COUNT 4U
#define TEST_TRUNCATED_TEXT_LENGTH 5U

static void test_string_suffix_matching_is_case_insensitive(void)
{
  TEST_ASSERT_TRUE(AppLogic_StringEndsWithIgnoreCase("Song.MP3", ".mp3"));
  TEST_ASSERT_TRUE(AppLogic_StringEndsWithIgnoreCase("mix.final.mp3", ".MP3"));
  TEST_ASSERT_FALSE(AppLogic_StringEndsWithIgnoreCase("cover.wav", ".mp3"));
  TEST_ASSERT_FALSE(AppLogic_StringEndsWithIgnoreCase("mp3", ".mp3"));
}

static void test_build_lrc_file_name_replaces_mp3_suffix(void)
{
  char name[APP_TRACK_NAME_LENGTH];

  AppLogic_BuildLrcFileName("Track 01.MP3", name, sizeof(name));
  TEST_ASSERT_STRING_EQ("Track 01.lrc", name);

  AppLogic_BuildLrcFileName("notes", name, sizeof(name));
  TEST_ASSERT_STRING_EQ("notes.lrc", name);
}

static void test_build_lrc_file_name_truncates_safely(void)
{
  char name[TEST_SHORT_TRACK_NAME_LENGTH];

  AppLogic_BuildLrcFileName("very-long-track-name.mp3", name, sizeof(name));
  TEST_ASSERT_STRING_EQ("very-long", name);
}

static void test_parse_lyric_line_reads_timestamp_and_text(void)
{
  uint32_t timestamp = 0U;
  const char *text = NULL;

  TEST_ASSERT_TRUE(AppLogic_ParseLyricLine("[01:02.34] Hello", &timestamp, &text));
  TEST_ASSERT_UINT32_EQ(62340U, timestamp);
  TEST_ASSERT_STRING_EQ("Hello", text);

  TEST_ASSERT_TRUE(AppLogic_ParseLyricLine("[00:01.007][00:02.000]Echo", &timestamp, &text));
  TEST_ASSERT_UINT32_EQ(1007U, timestamp);
  TEST_ASSERT_STRING_EQ("Echo", text);
}

static void test_parse_lyric_line_rejects_non_timed_lines(void)
{
  uint32_t timestamp = 0U;
  const char *text = NULL;

  TEST_ASSERT_FALSE(AppLogic_ParseLyricLine("", &timestamp, &text));
  TEST_ASSERT_FALSE(AppLogic_ParseLyricLine("[", &timestamp, &text));
  TEST_ASSERT_FALSE(AppLogic_ParseLyricLine("[ar:Artist]", &timestamp, &text));
  TEST_ASSERT_FALSE(AppLogic_ParseLyricLine("plain text", &timestamp, &text));
}

static void test_parse_metadata_line_handles_short_malformed_lines(void)
{
  AppLyricMetadata metadata;

  AppLogic_ClearMetadata(&metadata);
  TEST_ASSERT_FALSE(AppLogic_ParseMetadataLine("", &metadata));
  TEST_ASSERT_TRUE(AppLogic_ParseMetadataLine("[", &metadata));
  TEST_ASSERT_TRUE(AppLogic_ParseMetadataLine("[a", &metadata));
}

static void test_parse_metadata_line_extracts_artist_title_and_duration(void)
{
  AppLyricMetadata metadata;

  AppLogic_ClearMetadata(&metadata);
  TEST_ASSERT_TRUE(AppLogic_ParseMetadataLine("[ar: The Artist]", &metadata));
  TEST_ASSERT_TRUE(AppLogic_ParseMetadataLine("[ti: The Title]", &metadata));
  TEST_ASSERT_TRUE(AppLogic_ParseMetadataLine("[length: 03:45]", &metadata));

  TEST_ASSERT_STRING_EQ("The Artist", metadata.artist);
  TEST_ASSERT_STRING_EQ("The Title", metadata.title);
  TEST_ASSERT_UINT32_EQ(225000U, metadata.duration_ms);
}

static void test_copy_display_text_removes_inline_timestamps_and_preserves_latin1_utf8(void)
{
  char text[TEST_DISPLAY_TEXT_LENGTH];

  AppLogic_CopyDisplayText(text, sizeof(text), "Ho<00:12.34>la canción");
  TEST_ASSERT_STRING_EQ("Hola canción", text);
}

static void test_copy_display_text_replaces_unsupported_utf8(void)
{
  char text[TEST_DISPLAY_TEXT_LENGTH];

  AppLogic_CopyDisplayText(text, sizeof(text), "A\xe2\x82\xac" "B");
  TEST_ASSERT_STRING_EQ("A???B", text);
}

static void test_sort_track_names_orders_fixed_width_rows(void)
{
  char tracks[TEST_TRACK_FIXTURE_COUNT][APP_TRACK_NAME_LENGTH] = {
    "zeta.mp3",
    "Alpha.mp3",
    "middle.mp3",
    "beta.mp3",
  };

  AppLogic_SortTrackNames(&tracks[0][0], TEST_TRACK_FIXTURE_COUNT, sizeof(tracks[0]));

  TEST_ASSERT_STRING_EQ("Alpha.mp3", tracks[0]);
  TEST_ASSERT_STRING_EQ("beta.mp3", tracks[1]);
  TEST_ASSERT_STRING_EQ("middle.mp3", tracks[2]);
  TEST_ASSERT_STRING_EQ("zeta.mp3", tracks[3]);
}

static void test_text_helpers_parse_timestamp_tags(void)
{
  const char *cursor = "[02:03.456]Text";
  uint32_t timestamp = 0U;

  TEST_ASSERT_TRUE(AppText_ParseTimestampTag(&cursor, &timestamp));
  TEST_ASSERT_UINT32_EQ(123456U, timestamp);
  TEST_ASSERT_STRING_EQ("Text", cursor);
}

static void test_text_helpers_skip_inline_timestamp_tags(void)
{
  const char *cursor = "<00:12.34>word";

  TEST_ASSERT_TRUE(AppText_SkipInlineTimestampTag(&cursor));
  TEST_ASSERT_STRING_EQ("word", cursor);
}

static void test_text_helpers_copy_truncated_always_terminates(void)
{
  char text[TEST_TRUNCATED_TEXT_LENGTH];

  AppText_CopyTruncated(text, sizeof(text), "abcdef");
  TEST_ASSERT_STRING_EQ("abcd", text);
}

static void test_track_status_values_are_distinct(void)
{
  TEST_ASSERT_TRUE(APP_TRACKS_STATUS_OK != APP_TRACKS_STATUS_INVALID_PARAMETER);
  TEST_ASSERT_TRUE(APP_TRACKS_STATUS_OK != APP_TRACKS_STATUS_MOUNT_FAILED);
  TEST_ASSERT_TRUE(APP_TRACKS_STATUS_OK != APP_TRACKS_STATUS_OPEN_DIR_FAILED);
  TEST_ASSERT_TRUE(APP_TRACKS_STATUS_OK != APP_TRACKS_STATUS_READ_DIR_FAILED);
  TEST_ASSERT_TRUE(APP_TRACKS_STATUS_OK != APP_TRACKS_STATUS_NO_TRACKS);
}

static void test_lyrics_status_values_are_distinct(void)
{
  TEST_ASSERT_TRUE(APP_LYRICS_STATUS_OK != APP_LYRICS_STATUS_INVALID_PARAMETER);
  TEST_ASSERT_TRUE(APP_LYRICS_STATUS_OK != APP_LYRICS_STATUS_FILE_NOT_FOUND);
  TEST_ASSERT_TRUE(APP_LYRICS_STATUS_OK != APP_LYRICS_STATUS_OPEN_FAILED);
  TEST_ASSERT_TRUE(APP_LYRICS_STATUS_OK != APP_LYRICS_STATUS_READ_FAILED);
  TEST_ASSERT_TRUE(APP_LYRICS_STATUS_OK != APP_LYRICS_STATUS_NO_TIMED_LINES);
}

int main(void)
{
  RUN_TEST(test_string_suffix_matching_is_case_insensitive);
  RUN_TEST(test_build_lrc_file_name_replaces_mp3_suffix);
  RUN_TEST(test_build_lrc_file_name_truncates_safely);
  RUN_TEST(test_parse_lyric_line_reads_timestamp_and_text);
  RUN_TEST(test_parse_lyric_line_rejects_non_timed_lines);
  RUN_TEST(test_parse_metadata_line_handles_short_malformed_lines);
  RUN_TEST(test_parse_metadata_line_extracts_artist_title_and_duration);
  RUN_TEST(test_copy_display_text_removes_inline_timestamps_and_preserves_latin1_utf8);
  RUN_TEST(test_copy_display_text_replaces_unsupported_utf8);
  RUN_TEST(test_sort_track_names_orders_fixed_width_rows);
  RUN_TEST(test_text_helpers_parse_timestamp_tags);
  RUN_TEST(test_text_helpers_skip_inline_timestamp_tags);
  RUN_TEST(test_text_helpers_copy_truncated_always_terminates);
  RUN_TEST(test_track_status_values_are_distinct);
  RUN_TEST(test_lyrics_status_values_are_distinct);

  return test_failures == 0 ? 0 : 1;
}
