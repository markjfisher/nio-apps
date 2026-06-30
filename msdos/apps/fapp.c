#include "fujinet-nio.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define CHUNK_SIZE 256
#define KEY_DATA_SIZE 420
#define KEY_NAME_SIZE 128

static uint8_t data_buf[CHUNK_SIZE];
static uint8_t key_data[KEY_DATA_SIZE];
static char key_name[KEY_NAME_SIZE];

static void usage(void)
{
  puts("FAPP app-store demo");
  puts("Usage:");
  puts("  FAPP LIST ns");
  puts("  FAPP STAT ns key");
  puts("  FAPP GET ns key");
  puts("  FAPP PUT ns key value");
  puts("  FAPP DEL ns key");
}

static int is_cmd(const char *a, const char *b)
{
  while (*a && *b) {
    char ca = *a;
    char cb = *b;
    if (ca >= 'a' && ca <= 'z')
      ca = (char) (ca - 'a' + 'A');
    if (cb >= 'a' && cb <= 'z')
      cb = (char) (cb - 'a' + 'A');
    if (ca != cb)
      return 0;
    a++;
    b++;
  }
  return *a == 0 && *b == 0;
}

static int fail(const char *what, uint8_t result)
{
  printf("%s failed: %u (%s)\n", what, (unsigned) result,
         fn_error_string(result));
  return 2;
}

static int do_stat(const char *ns, const char *key)
{
  fn_appstore_stat_t st;
  uint8_t result;

  result = fn_appstore_stat(ns, key, &st);
  if (result != FN_OK)
    return fail("STAT", result);

  printf("%s/%s\n", ns, key);
  printf("  exists: %s\n", st.exists ? "yes" : "no");
  printf("  size:   %lu\n", (unsigned long) st.size_bytes);
  printf("  mtime:  %lu\n", (unsigned long) st.mtime_unix);
  return 0;
}

static int do_get(const char *ns, const char *key)
{
  uint32_t offset = 0;

  for (;;) {
    fn_appstore_read_t rr;
    uint8_t result;
    uint16_t i;

    result = fn_appstore_read(ns, key, offset, data_buf, sizeof(data_buf), &rr);
    if (result != FN_OK)
      return fail("GET", result);
    if ((rr.flags & FN_APPSTORE_READ_EXISTS) == 0) {
      puts("Key not found");
      return 1;
    }

    for (i = 0; i < rr.bytes_read; i++)
      putchar(data_buf[i]);

    offset += rr.bytes_read;
    if ((rr.flags & FN_APPSTORE_READ_EOF) || rr.bytes_read == 0)
      break;
  }
  putchar('\n');
  return 0;
}

static int do_put(const char *ns, const char *key, const char *value)
{
  const uint8_t *p = (const uint8_t *) value;
  uint16_t left = (uint16_t) strlen(value);
  uint32_t offset = 0;

  if (left == 0) {
    fn_appstore_write_t wr;
    uint8_t result = fn_appstore_write(ns, key, 0, (const uint8_t *) "", 0, &wr);
    if (result != FN_OK)
      return fail("PUT", result);
    puts("Wrote 0 bytes");
    return 0;
  }

  while (left != 0) {
    fn_appstore_write_t wr;
    uint16_t chunk = left > CHUNK_SIZE ? CHUNK_SIZE : left;
    uint8_t result = fn_appstore_write(ns, key, offset, p, chunk, &wr);
    if (result != FN_OK)
      return fail("PUT", result);
    if (wr.bytes_written == 0) {
      puts("Write stalled");
      return 2;
    }
    p += wr.bytes_written;
    left = (uint16_t) (left - wr.bytes_written);
    offset += wr.bytes_written;
  }

  printf("Wrote %lu bytes\n", (unsigned long) offset);
  return 0;
}

static int do_delete(const char *ns, const char *key)
{
  fn_appstore_delete_t dr;
  uint8_t result;

  result = fn_appstore_delete(ns, key, &dr);
  if (result != FN_OK)
    return fail("DEL", result);
  printf("Deleted: %s\n", dr.deleted ? "yes" : "no");
  return 0;
}

static int do_list(const char *ns)
{
  uint16_t start = 0;
  unsigned total = 0;

  for (;;) {
    fn_appstore_list_t lr;
    uint16_t off = 0;
    uint16_t i;
    uint8_t result;

    result = fn_appstore_list(ns, start, key_data, sizeof(key_data), &lr);
    if (result != FN_OK)
      return fail("LIST", result);

    for (i = 0; i < lr.key_count; i++) {
      result = fn_appstore_list_next_key(key_data, lr.key_data_len, &off,
                                         key_name, sizeof(key_name));
      if (result != FN_OK)
        return fail("LIST decode", result);
      puts(key_name);
      total++;
    }

    start = (uint16_t) (start + lr.key_count);
    if ((lr.flags & FN_APPSTORE_LIST_MORE) == 0)
      break;
    if (lr.key_count == 0) {
      puts("List made no progress");
      return 2;
    }
  }

  printf("%u key%s\n", total, total == 1 ? "" : "s");
  return 0;
}

int main(int argc, char **argv)
{
  if (argc < 3 || argv[1][0] == '?') {
    usage();
    return 1;
  }

  if (is_cmd(argv[1], "LIST") && argc == 3)
    return do_list(argv[2]);
  if (is_cmd(argv[1], "STAT") && argc == 4)
    return do_stat(argv[2], argv[3]);
  if (is_cmd(argv[1], "GET") && argc == 4)
    return do_get(argv[2], argv[3]);
  if (is_cmd(argv[1], "PUT") && argc == 5)
    return do_put(argv[2], argv[3], argv[4]);
  if ((is_cmd(argv[1], "DEL") || is_cmd(argv[1], "DELETE")) && argc == 4)
    return do_delete(argv[2], argv[3]);

  usage();
  return 1;
}
