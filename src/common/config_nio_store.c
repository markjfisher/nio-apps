#include "config_nio.h"
#include "fujinet-nio.h"

#include <stdlib.h>
#include <string.h>

static uint8_t store_buf[CONFIG_NIO_TEXT_MAX + 1];
static char line_buf[CONFIG_NIO_URI_MAX + 1];
static char host_tmp[CONFIG_NIO_URI_MAX + 1];

#define CONFIG_NIO_APPSTORE_READ_MAX 502

static void append_digit(char *buf, uint16_t *off, uint8_t value)
{
  buf[*off] = (char) ('0' + value);
  *off = (uint16_t) (*off + 1);
  buf[*off] = 0;
}

static int appstore_read_text(const char *key, char *buf, uint16_t cap,
                              uint8_t *exists)
{
  fn_appstore_read_t rr;
  uint16_t max_read;
  uint8_t result;
  uint16_t n;

  if (cap == 0)
    return 0;

  buf[0] = 0;
  if (exists)
    *exists = 0;

  max_read = (uint16_t) (cap - 1);
  if (max_read > CONFIG_NIO_APPSTORE_READ_MAX)
    max_read = CONFIG_NIO_APPSTORE_READ_MAX;

  result = fn_appstore_read(CONFIG_NIO_NS, key, 0, (uint8_t *) buf,
                            max_read, &rr);
  if (result != FN_OK)
    return 0;
  if ((rr.flags & FN_APPSTORE_READ_EXISTS) == 0)
    return 1;

  n = rr.bytes_read;
  if (n >= cap)
    n = (uint16_t) (cap - 1);
  buf[n] = 0;
  if (exists)
    *exists = 1;
  return 1;
}

static int appstore_write_text(const char *key, const char *buf)
{
  fn_appstore_write_t wr;
  uint16_t len;
  uint8_t result;

  len = (uint16_t) strlen(buf);
  result = fn_appstore_write(CONFIG_NIO_NS, key, 0, (const uint8_t *) buf,
                             len, &wr);
  return result == FN_OK && wr.bytes_written == len;
}

static void trim_line(char *s)
{
  char *p;

  p = strchr(s, '\n');
  if (p)
    *p = 0;
  p = strchr(s, '\r');
  if (p)
    *p = 0;
}

static int next_line(const char **src, char *out, uint16_t cap)
{
  const char *p;
  uint16_t len;

  if (!src || !*src || !**src || cap == 0)
    return 0;

  p = *src;
  len = 0;
  while (p[len] && p[len] != '\n' && p[len] != '\r' && len < (uint16_t) (cap - 1)) {
    out[len] = p[len];
    len++;
  }
  out[len] = 0;

  while (p[len] && p[len] != '\n' && p[len] != '\r')
    len++;
  while (p[len] == '\n' || p[len] == '\r')
    len++;
  *src = &p[len];
  trim_line(out);
  return 1;
}

static void seed_hosts(config_nio_state_t *state)
{
  state->host_count = 3;
  strcpy(state->hosts[0], "sd0:/");
  strcpy(state->hosts[1], "tnfs.diller.org");
  strcpy(state->hosts[2], "apps.irata.online");
}

static void parse_hosts(config_nio_state_t *state, const char *text)
{
  const char *p;

  state->host_count = 0;
  p = text;
  while (next_line(&p, line_buf, sizeof(line_buf)) &&
         state->host_count < CONFIG_NIO_MAX_HOSTS) {
    if (!line_buf[0])
      continue;
    strncpy(host_tmp, line_buf, CONFIG_NIO_URI_MAX);
    host_tmp[CONFIG_NIO_URI_MAX] = 0;
    strcpy(state->hosts[state->host_count], host_tmp);
    state->host_count++;
  }
}

static void parse_mappings(config_nio_state_t *state, const char *text)
{
  const char *p;

  p = text;
  while (next_line(&p, line_buf, sizeof(line_buf))) {
    char *a;
    char *b;
    char *c;
    int unit;
    int slot;

    a = line_buf;
    b = strchr(a, '\t');
    if (!b)
      continue;
    *b++ = 0;
    c = strchr(b, '\t');
    if (!c)
      continue;
    *c++ = 0;

    unit = atoi(a);
    slot = atoi(b);
    if (unit < 0 || unit >= FNCTL_MAX_UNITS || slot < 0 || slot >= FNCTL_MAX_UNITS)
      continue;

    state->mappings[unit].valid = 1;
    state->mappings[unit].slot = (uint8_t) slot;
    state->mappings[unit].readonly =
      (uint8_t) (strcmp(c, "ro") == 0 || strcmp(c, "RO") == 0);
  }
}

int config_nio_save_hosts(const config_nio_state_t *state)
{
  uint8_t i;
  uint16_t off;

  off = 0;
  store_buf[0] = 0;
  for (i = 0; i < state->host_count; i++) {
    uint16_t len;

    len = (uint16_t) strlen(state->hosts[i]);
    if ((uint16_t) (off + len + 1) >= sizeof(store_buf))
      return 0;
    memcpy(&store_buf[off], state->hosts[i], len);
    off = (uint16_t) (off + len);
    store_buf[off++] = '\n';
    store_buf[off] = 0;
  }
  return appstore_write_text(CONFIG_NIO_KEY_HOSTS, (const char *) store_buf);
}

int config_nio_save_mappings(const config_nio_state_t *state)
{
  uint8_t unit;
  uint16_t off;

  off = 0;
  store_buf[0] = 0;
  for (unit = 0; unit < FNCTL_MAX_UNITS; unit++) {
    if (!state->mappings[unit].valid)
      continue;
    if ((uint16_t) (off + 8) >= sizeof(store_buf))
      return 0;
    append_digit((char *) store_buf, &off, unit);
    store_buf[off++] = '\t';
    append_digit((char *) store_buf, &off, state->mappings[unit].slot);
    store_buf[off++] = '\t';
    store_buf[off++] = state->mappings[unit].readonly ? 'r' : 'r';
    store_buf[off++] = state->mappings[unit].readonly ? 'o' : 'w';
    store_buf[off++] = '\n';
    store_buf[off] = 0;
  }
  return appstore_write_text(CONFIG_NIO_KEY_MAPPINGS, (const char *) store_buf);
}

int config_nio_load(config_nio_state_t *state)
{
  uint8_t exists;

  if (!state)
    return 0;

  memset(state, 0, sizeof(*state));
  if (!appstore_read_text(CONFIG_NIO_KEY_HOSTS, (char *) store_buf,
                          sizeof(store_buf), &exists))
    return 0;
  if (exists)
    parse_hosts(state, (const char *) store_buf);
  if (state->host_count == 0) {
    seed_hosts(state);
    (void) config_nio_save_hosts(state);
  }

  if (!appstore_read_text(CONFIG_NIO_KEY_MAPPINGS, (char *) store_buf,
                          sizeof(store_buf), &exists))
    return 0;
  if (exists)
    parse_mappings(state, (const char *) store_buf);

  (void) config_nio_refresh_slots(state);
  config_nio_set_status(state, "Ready");
  return 1;
}
