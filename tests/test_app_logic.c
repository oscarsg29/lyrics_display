#include "app_logic.h"
#include "test_assert.h"

#include <stdbool.h>
#include <string.h>

static void test_string_suffix_matching_is_case_insensitive(void)
{
  TEST_ASSERT_TRUE(AppLogic_StringEndsWithIgnoreCase("Song.MP3", ".mp3"));
  TEST_ASSERT_TRUE(AppLogic_StringEndsWithIgnoreCase("mix.final.mp3", ".MP3"));
  TEST_ASSERT_FALSE(AppLogic_StringEndsWithIgnoreCase("cover.wav", ".mp3"));
  TEST_ASSERT_FALSE(AppLogic_StringEndsWithIgnoreCase("mp3", ".mp3"));
}

static void test_build_lrc_file_name_replaces_mp3_suffix(void)
{
  char name[64];

  AppLogic_BuildLrcFileName("Track 01.MP3", name, sizeof(name));
  TEST_ASSERT_STRING_EQ("Track 01.lrc", name);

  AppLogic_BuildLrcFileName("notes", name, sizeof(name));
  TEST_ASSERT_STRING_EQ("notes.lrc", name);
}

static void test_build_lrc_file_name_truncates_safely(void)
{
  char name[10];

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

  TEST_ASSERT_FALSE(AppLogic_ParseLyricLine("[ar:Artist]", &timestamp, &text));
  TEST_ASSERT_FALSE(AppLogic_ParseLyricLine("plain text", &timestamp, &text));
}

static void test_parse_metadata_line_extracts_artist_title_and_duration(void)
{
  char artist[64] = {0};
  char title[64] = {0};
  uint32_t duration = 0U;

  TEST_ASSERT_TRUE(AppLogic_ParseMetadataLine("[ar: The Artist]", artist, sizeof(artist), title, sizeof(title), &duration));
  TEST_ASSERT_TRUE(AppLogic_ParseMetadataLine("[ti: The Title]", artist, sizeof(artist), title, sizeof(title), &duration));
  TEST_ASSERT_TRUE(AppLogic_ParseMetadataLine("[length: 03:45]", artist, sizeof(artist), title, sizeof(title), &duration));

  TEST_ASSERT_STRING_EQ("The Artist", artist);
  TEST_ASSERT_STRING_EQ("The Title", title);
  TEST_ASSERT_UINT32_EQ(225000U, duration);
}

static void test_copy_display_text_removes_inline_timestamps_and_preserves_latin1_utf8(void)
{
  char text[32];

  AppLogic_CopyDisplayText(text, sizeof(text), "Ho<00:12.34>la canción");
  TEST_ASSERT_STRING_EQ("Hola canción", text);
}

static void test_copy_display_text_replaces_unsupported_utf8(void)
{
  char text[32];

  AppLogic_CopyDisplayText(text, sizeof(text), "A\xe2\x82\xac" "B");
  TEST_ASSERT_STRING_EQ("A???B", text);
}

static void test_sort_track_names_orders_fixed_width_rows(void)
{
  char tracks[4][64] = {
    "zeta.mp3",
    "Alpha.mp3",
    "middle.mp3",
    "beta.mp3",
  };

  AppLogic_SortTrackNames(&tracks[0][0], 4U, sizeof(tracks[0]));

  TEST_ASSERT_STRING_EQ("Alpha.mp3", tracks[0]);
  TEST_ASSERT_STRING_EQ("beta.mp3", tracks[1]);
  TEST_ASSERT_STRING_EQ("middle.mp3", tracks[2]);
  TEST_ASSERT_STRING_EQ("zeta.mp3", tracks[3]);
}

int main(void)
{
  RUN_TEST(test_string_suffix_matching_is_case_insensitive);
  RUN_TEST(test_build_lrc_file_name_replaces_mp3_suffix);
  RUN_TEST(test_build_lrc_file_name_truncates_safely);
  RUN_TEST(test_parse_lyric_line_reads_timestamp_and_text);
  RUN_TEST(test_parse_lyric_line_rejects_non_timed_lines);
  RUN_TEST(test_parse_metadata_line_extracts_artist_title_and_duration);
  RUN_TEST(test_copy_display_text_removes_inline_timestamps_and_preserves_latin1_utf8);
  RUN_TEST(test_copy_display_text_replaces_unsupported_utf8);
  RUN_TEST(test_sort_track_names_orders_fixed_width_rows);

  return test_failures == 0 ? 0 : 1;
}
