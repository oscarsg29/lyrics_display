#ifndef TEST_ASSERT_H
#define TEST_ASSERT_H

#include <stdint.h>
#include <stdio.h>
#include <string.h>

static int test_failures = 0;

#define TEST_ASSERT_TRUE(expr) \
  do \
  { \
    if (!(expr)) \
    { \
      fprintf(stderr, "%s:%d: expected true: %s\n", __FILE__, __LINE__, #expr); \
      test_failures++; \
    } \
  } while (0)

#define TEST_ASSERT_FALSE(expr) \
  do \
  { \
    if (expr) \
    { \
      fprintf(stderr, "%s:%d: expected false: %s\n", __FILE__, __LINE__, #expr); \
      test_failures++; \
    } \
  } while (0)

#define TEST_ASSERT_UINT32_EQ(expected, actual) \
  do \
  { \
    uint32_t expected_value = (uint32_t)(expected); \
    uint32_t actual_value = (uint32_t)(actual); \
    if (expected_value != actual_value) \
    { \
      fprintf(stderr, "%s:%d: expected %lu, got %lu\n", __FILE__, __LINE__, \
              (unsigned long)expected_value, (unsigned long)actual_value); \
      test_failures++; \
    } \
  } while (0)

#define TEST_ASSERT_STRING_EQ(expected, actual) \
  do \
  { \
    const char *expected_value = (expected); \
    const char *actual_value = (actual); \
    if (strcmp(expected_value, actual_value) != 0) \
    { \
      fprintf(stderr, "%s:%d: expected \"%s\", got \"%s\"\n", __FILE__, __LINE__, \
              expected_value, actual_value); \
      test_failures++; \
    } \
  } while (0)

#define RUN_TEST(test_fn) \
  do \
  { \
    int failures_before = test_failures; \
    test_fn(); \
    printf("%s: %s\n", #test_fn, failures_before == test_failures ? "PASS" : "FAIL"); \
  } while (0)

#endif
