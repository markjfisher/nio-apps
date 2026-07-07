#include "fujinet-nio.h"

#include <stdio.h>
#include <string.h>

#ifdef __ATARI__
#include <conio.h>
#endif

#define ASTEST_NS "nio.astest"
#define ASTEST_KEY_A "alpha"
#define ASTEST_KEY_B "beta"
#define ASTEST_VALUE_A "alpha value from astest"
#define ASTEST_VALUE_B1 "beta:"
#define ASTEST_VALUE_B2 "chunk-two"
#define ASTEST_READ_MAX 96
#define ASTEST_KEY_DATA_MAX 160
#define ASTEST_KEY_NAME_MAX 64

static uint8_t read_buf[ASTEST_READ_MAX + 1];
static uint8_t key_data[ASTEST_KEY_DATA_MAX];
static char key_name[ASTEST_KEY_NAME_MAX];

static int failures;

static void pass(const char *name)
{
  printf("%s OK\n", name);
}

static void fail_msg(const char *name, const char *msg)
{
  printf("%s FAIL %s\n", name, msg);
  failures++;
}

static void fail_code(const char *name, uint8_t result)
{
  printf("%s FAIL %u(%s)\n", name, (unsigned) result, fn_error_string(result));
  failures++;
}

static int write_value(const char *key, uint32_t offset, const char *value)
{
  fn_appstore_write_t wr;
  uint16_t len = (uint16_t) strlen(value);
  uint8_t result;

  result = fn_appstore_write(ASTEST_NS, key, offset, (const uint8_t *) value, len, &wr);
  if (result != FN_OK) {
    fail_code("write", result);
    return 0;
  }
  if (wr.offset != offset || wr.bytes_written != len) {
    fail_msg("write", "unexpected count");
    return 0;
  }
  return 1;
}

static int expect_stat(const char *key, uint8_t exists, uint32_t size)
{
  fn_appstore_stat_t st;
  uint8_t result;

  result = fn_appstore_stat(ASTEST_NS, key, &st);
  if (result != FN_OK) {
    fail_code("stat", result);
    return 0;
  }
  if ((st.exists ? 1 : 0) != (exists ? 1 : 0)) {
    fail_msg("stat", exists ? "missing" : "unexpected");
    return 0;
  }
  if (exists && st.size_bytes != size) {
    fail_msg("stat", "wrong size");
    return 0;
  }
  return 1;
}

static int expect_read(const char *key, const char *expected)
{
  fn_appstore_read_t rr;
  uint16_t expected_len = (uint16_t) strlen(expected);
  uint8_t result;

  memset(read_buf, 0, sizeof(read_buf));
  result = fn_appstore_read(ASTEST_NS, key, 0, read_buf, ASTEST_READ_MAX, &rr);
  if (result != FN_OK) {
    fail_code("read", result);
    return 0;
  }
  if ((rr.flags & FN_APPSTORE_READ_EXISTS) == 0) {
    fail_msg("read", "missing");
    return 0;
  }
  if (rr.bytes_read != expected_len) {
    fail_msg("read", "wrong length");
    return 0;
  }
  read_buf[rr.bytes_read] = 0;
  if (strcmp((const char *) read_buf, expected) != 0) {
    fail_msg("read", "wrong value");
    return 0;
  }
  return 1;
}

static int list_has_key(const char *wanted)
{
  fn_appstore_list_t lr;
  uint16_t off = 0;
  uint16_t i;
  uint8_t result;

  result = fn_appstore_list(ASTEST_NS, 0, key_data, sizeof(key_data), &lr);
  if (result != FN_OK) {
    fail_code("list", result);
    return 0;
  }

  for (i = 0; i < lr.key_count; i++) {
    result = fn_appstore_list_next_key(key_data, lr.key_data_len, &off,
                                       key_name, sizeof(key_name));
    if (result != FN_OK) {
      fail_code("list decode", result);
      return 0;
    }
    if (strcmp(key_name, wanted) == 0)
      return 1;
  }

  return 0;
}

static int delete_key(const char *key, uint8_t expect_deleted)
{
  fn_appstore_delete_t dr;
  uint8_t result;

  result = fn_appstore_delete(ASTEST_NS, key, &dr);
  if (result != FN_OK) {
    fail_code("delete", result);
    return 0;
  }
  if ((dr.deleted ? 1 : 0) != (expect_deleted ? 1 : 0)) {
    fail_msg("delete", "unexpected flag");
    return 0;
  }
  return 1;
}

static void pause_for_inspection(void)
{
#ifdef __ATARI__
  puts("press key");
  cgetc();
#else
  puts("done");
#endif
}

int main(void)
{
  uint8_t result;

  puts("ASTEST app-store");
  printf("ns=%s\n", ASTEST_NS);

  result = fn_init();
  if (result != FN_OK) {
    fail_code("init", result);
    pause_for_inspection();
    return 2;
  }

  if (!fn_is_ready()) {
    fail_msg("ready", "not ready");
    pause_for_inspection();
    return 2;
  }

  delete_key(ASTEST_KEY_A, 0);
  delete_key(ASTEST_KEY_B, 0);

  if (write_value(ASTEST_KEY_A, 0, ASTEST_VALUE_A) &&
      expect_stat(ASTEST_KEY_A, 1, (uint32_t) strlen(ASTEST_VALUE_A)) &&
      expect_read(ASTEST_KEY_A, ASTEST_VALUE_A))
    pass("alpha");

  if (write_value(ASTEST_KEY_B, 0, ASTEST_VALUE_B1) &&
      write_value(ASTEST_KEY_B, (uint32_t) strlen(ASTEST_VALUE_B1), ASTEST_VALUE_B2) &&
      expect_stat(ASTEST_KEY_B, 1,
                  (uint32_t) (strlen(ASTEST_VALUE_B1) + strlen(ASTEST_VALUE_B2))) &&
      expect_read(ASTEST_KEY_B, ASTEST_VALUE_B1 ASTEST_VALUE_B2))
    pass("beta");

  if (list_has_key(ASTEST_KEY_A) && list_has_key(ASTEST_KEY_B))
    pass("list");
  else
    fail_msg("list", "missing key");

  if (delete_key(ASTEST_KEY_B, 1) &&
      expect_stat(ASTEST_KEY_B, 0, 0) &&
      !list_has_key(ASTEST_KEY_B))
    pass("delete");

  printf("PASS=%u FAIL=%u\n", failures ? 0U : 1U, (unsigned) failures);
  pause_for_inspection();
  return failures ? 1 : 0;
}
