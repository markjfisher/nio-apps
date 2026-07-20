#include "config_nio.h"

#include <conio.h>
#include <string.h>

#define BBC_WIDTH 40
#define BBC_ROWS 24
#define BBC_PAGE_ROWS 10
#define BBC_URI_WORK_MAX 160

enum {
  SCREEN_HOSTS,
  SCREEN_BROWSE,
  SCREEN_SLOTS
};

static char num_buf[6];
static char edit_buf[CONFIG_NIO_URI_MAX + 1];
static char uri_buf[BBC_URI_WORK_MAX];
static uint8_t current_screen;
static uint8_t selected_host;
static uint8_t selected_entry;
static uint8_t selected_drive;
static uint8_t browse_host;
static uint16_t browse_start;
static uint16_t browse_next;
static uint8_t browse_more;

static void nl(void)
{
  cputc('\n');
}

static void mode7(void)
{
  cputc(22);
  cputc(7);
}

static void clear_line(uint8_t row)
{
  uint8_t i;

  gotoxy(0, row);
  for (i = 0; i < BBC_WIDTH; i++)
    cputc(' ');
  gotoxy(0, row);
}

static void text_at(uint8_t x, uint8_t y, const char *s)
{
  gotoxy(x, y);
  cputs(s ? s : "");
}

static void put_fixed(const char *s, uint8_t width)
{
  uint8_t n;

  n = 0;
  while (s && *s && n < width) {
    cputc(*s++);
    n++;
  }
  while (n < width) {
    cputc(' ');
    n++;
  }
}

static void put_tail(const char *s, uint8_t width)
{
  uint16_t len;

  len = s ? (uint16_t) strlen(s) : 0;
  if (len > width)
    s += (len - width);
  put_fixed(s, width);
}

static void put_uint(unsigned value)
{
  uint8_t i;

  i = 0;
  do {
    num_buf[i++] = (char) ('0' + (value % 10U));
    value = (unsigned) (value / 10U);
  } while (value && i < sizeof(num_buf));

  while (i)
    cputc(num_buf[--i]);
}

static void header(const char *screen)
{
  clear_line(0);
  text_at(0, 0, "CONFIG-NIO ");
  cputs(screen);
  clear_line(1);
  text_at(0, 1, "H Hosts  S Slots  X Mount  Q Quit");
  clear_line(2);
  text_at(0, 2, "----------------------------------------");
}

static void status_line(const char *s)
{
  clear_line(22);
  put_fixed(s ? s : "", BBC_WIDTH);
  clear_line(23);
}

static void pause_line(const char *s)
{
  status_line(s);
  (void) cgetc();
}

static int key_is_quit(int key)
{
  return key == 'q' || key == 'Q' || key == 27;
}

static int prompt_line(const char *label, char *buf, uint16_t cap)
{
  uint16_t n;

  if (!buf || cap == 0)
    return 0;
  clear_line(22);
  put_fixed(label ? label : "Value", 12);
  cputs(": ");
  if (!cgets(buf, cap)) {
    buf[0] = 0;
    return 0;
  }
  n = (uint16_t) strlen(buf);
  while (n > 0 && (buf[n - 1] == '\r' || buf[n - 1] == '\n'))
    buf[--n] = 0;
  return 1;
}

static void copy_limited(char *dst, uint16_t cap, const char *src)
{
  uint16_t n;

  if (!dst || cap == 0)
    return;
  n = src ? (uint16_t) strlen(src) : 0;
  if (n >= cap)
    n = (uint16_t) (cap - 1);
  if (n)
    memcpy(dst, src, n);
  dst[n] = 0;
}

static void parent_path(char *path)
{
  uint16_t len;

  len = (uint16_t) strlen(path);
  while (len > 0 && path[len - 1] == '/')
    path[--len] = 0;
  while (len > 0 && path[len - 1] != '/')
    path[--len] = 0;
}

static int enter_dir(config_nio_state_t *state, const char *name)
{
  uint16_t len;
  uint16_t nlen;

  len = (uint16_t) strlen(state->browse_path);
  nlen = (uint16_t) strlen(name);
  if ((uint16_t) (len + nlen + 2) > CONFIG_NIO_PATH_MAX) {
    config_nio_set_status(state, "Path too long");
    return 0;
  }
  if (len > 0 && state->browse_path[len - 1] != '/')
    state->browse_path[len++] = '/';
  memcpy(&state->browse_path[len], name, nlen);
  len = (uint16_t) (len + nlen);
  state->browse_path[len++] = '/';
  state->browse_path[len] = 0;
  return 1;
}

static void browse_cb(uint8_t is_dir, const char *name, uint32_t size,
                      uint32_t mtime, void *ctx)
{
  config_nio_state_t *state;
  config_nio_entry_t *entry;

  (void) size;
  (void) mtime;
  state = (config_nio_state_t *) ctx;
  if (state->entry_count >= CONFIG_NIO_MAX_ENTRIES)
    return;
  entry = &state->entries[state->entry_count++];
  entry->is_dir = is_dir;
  copy_limited(entry->name, sizeof(entry->name), name);
}

static int fetch_browse_page(config_nio_state_t *state)
{
  uint8_t ok;

  state->entry_count = 0;
  state->entries_truncated = 0;
  if (!config_nio_compose_uri(state->hosts[browse_host], state->browse_path,
                              "", uri_buf, sizeof(uri_buf))) {
    config_nio_set_status(state, "Path too long");
    return 0;
  }

  ok = (uint8_t) fnsvc_list_directory_page(uri_buf, browse_start,
                                           FNSVC_LIST_MAX_PAYLOAD,
                                           CONFIG_NIO_MAX_ENTRIES,
                                           browse_cb, state,
                                           &browse_next, &browse_more);
  if (!ok) {
    config_nio_set_status(state, "Error connecting to host");
    return 0;
  }

  selected_entry = 0;
  config_nio_set_status(state, browse_more ? "More entries" : "End of list");
  return 1;
}

static void show_hosts(config_nio_state_t *state)
{
  uint8_t i;

  clrscr();
  header("Hosts");
  text_at(0, 3, "Hosts");
  for (i = 0; i < CONFIG_NIO_MAX_HOSTS; i++) {
    gotoxy(0, (uint8_t) (5 + i));
    cputc(i == selected_host ? '>' : ' ');
    cputc((char) ('0' + i));
    cputc(' ');
    if (i < state->host_count)
      put_tail(state->hosts[i], 35);
    else
      put_fixed("", 35);
  }
  status_line("Enter browse  A add  E edit  C clear");
}

static void draw_browse(config_nio_state_t *state)
{
  uint8_t i;

  clrscr();
  header("Browse");
  text_at(0, 3, "Host: ");
  put_tail(state->hosts[browse_host], 33);
  text_at(0, 4, "Path: /");
  put_tail(state->browse_path, 33);
  for (i = 0; i < CONFIG_NIO_MAX_ENTRIES; i++) {
    gotoxy(0, (uint8_t) (6 + i));
    if (i < state->entry_count) {
      cputc(i == selected_entry ? '>' : ' ');
      cputc(state->entries[i].is_dir ? '/' : ' ');
      cputc(' ');
      put_tail(state->entries[i].name, 36);
    } else {
      put_fixed("", BBC_WIDTH);
    }
  }
  status_line("W/S move  Enter dir  A assign  N/P pg");
}

static void draw_slots(config_nio_state_t *state)
{
  uint8_t i;
  char label[8];

  clrscr();
  header("Slots");
  text_at(0, 3, "Drive mappings");
  for (i = 0; i < FNCTL_MAX_UNITS; i++) {
    gotoxy(0, (uint8_t) (5 + i));
    cputc(i == selected_drive ? '>' : ' ');
    config_nio_ui_drive_label(i, label, sizeof(label));
    put_fixed(label, 7);
    if (state->mappings[i].valid) {
      cputs("S");
      cputc((char) ('0' + state->mappings[i].slot));
      cputc(' ');
      cputs(state->mappings[i].readonly ? "RO " : "RW ");
      put_tail(state->slots[state->mappings[i].slot].uri, 24);
    } else {
      cputs("--");
    }
  }
  text_at(0, 15, "Slots");
  for (i = 0; i < FNCTL_MAX_UNITS && i < 7; i++) {
    gotoxy(0, (uint8_t) (16 + i));
    cputc((char) ('0' + i));
    cputc(' ');
    put_tail(state->slots[i].uri, 37);
  }
  status_line("Drive W/S  0-7 assign  E edit  C clear");
}

static void edit_host(config_nio_state_t *state)
{
  if (selected_host < state->host_count)
    copy_limited(edit_buf, sizeof(edit_buf), state->hosts[selected_host]);
  else
    edit_buf[0] = 0;
  if (!prompt_line("Host", edit_buf, sizeof(edit_buf)) || !edit_buf[0])
    return;
  copy_limited(state->hosts[selected_host], sizeof(state->hosts[selected_host]), edit_buf);
  if (selected_host >= state->host_count)
    state->host_count = (uint8_t) (selected_host + 1);
  (void) config_nio_save_hosts(state);
  config_nio_set_status(state, "Host saved");
}

static void clear_host(config_nio_state_t *state)
{
  uint8_t i;

  if (selected_host >= state->host_count)
    return;
  for (i = selected_host; i + 1 < state->host_count; i++)
    strcpy(state->hosts[i], state->hosts[i + 1]);
  state->host_count--;
  state->hosts[state->host_count][0] = 0;
  if (selected_host >= state->host_count && selected_host > 0)
    selected_host--;
  (void) config_nio_save_hosts(state);
  config_nio_set_status(state, "Host cleared");
}

static void edit_slot(config_nio_state_t *state)
{
  uint8_t slot;

  clear_line(22);
  cputs("Slot 0-7: ");
  slot = (uint8_t) cgetc();
  cputc((char) slot);
  if (slot < '0' || slot > '7') {
    config_nio_set_status(state, "Bad slot");
    return;
  }
  slot = (uint8_t) (slot - '0');
  copy_limited(edit_buf, sizeof(edit_buf), state->slots[slot].uri);
  if (!prompt_line("URI", edit_buf, sizeof(edit_buf)))
    return;
  if (edit_buf[0]) {
    if (!fnsvc_set_mount(slot, edit_buf, "rw", 1)) {
      config_nio_set_status(state, "Slot save failed");
      return;
    }
  } else {
    if (!fnsvc_set_mount(slot, "", "rw", 0)) {
      config_nio_set_status(state, "Slot clear failed");
      return;
    }
  }
  (void) config_nio_refresh_slots(state);
  config_nio_set_status(state, "Slot saved");
}

static void assign_selected_file(config_nio_state_t *state)
{
  uint8_t slot;

  if (state->entry_count == 0 || state->entries[selected_entry].is_dir) {
    config_nio_set_status(state, "Pick a file");
    return;
  }
  clear_line(22);
  cputs("Assign slot 0-7: ");
  slot = (uint8_t) cgetc();
  cputc((char) slot);
  if (slot < '0' || slot > '7') {
    config_nio_set_status(state, "Bad slot");
    return;
  }
  slot = (uint8_t) (slot - '0');
  if (!config_nio_compose_uri(state->hosts[browse_host], state->browse_path,
                              state->entries[selected_entry].name,
                              uri_buf, sizeof(uri_buf))) {
    config_nio_set_status(state, "URI too long");
    return;
  }
  if (!fnsvc_set_mount(slot, uri_buf, "rw", 1)) {
    config_nio_set_status(state, "Slot save failed");
    return;
  }
  (void) config_nio_refresh_slots(state);
  config_nio_set_status(state, "File assigned");
}

static void handle_hosts(config_nio_state_t *state, int key)
{
  if ((key == 'w' || key == 'W') && selected_host > 0)
    selected_host--;
  else if (key == 's' && selected_host + 1 < CONFIG_NIO_MAX_HOSTS)
    selected_host++;
  else if (key == 'a' || key == 'A') {
    if (state->host_count < CONFIG_NIO_MAX_HOSTS)
      selected_host = state->host_count;
    edit_host(state);
  } else if (key == 'e' || key == 'E') {
    edit_host(state);
  } else if (key == 'c' || key == 'C') {
    clear_host(state);
  } else if (key == '\r' || key == '\n') {
    if (selected_host < state->host_count) {
      browse_host = selected_host;
      state->browse_path[0] = 0;
      browse_start = 0;
      if (fetch_browse_page(state))
        current_screen = SCREEN_BROWSE;
      else
        pause_line("Error connecting to host. Press key.");
    }
  }
}

static void handle_browse(config_nio_state_t *state, int key)
{
  if (key == 'H') {
    current_screen = SCREEN_HOSTS;
    return;
  }
  if ((key == 'w' || key == 'W') && selected_entry > 0)
    selected_entry--;
  else if (key == 's' && selected_entry + 1 < state->entry_count)
    selected_entry++;
  else if ((key == 'n' || key == 'N') && browse_more) {
    browse_start = browse_next;
    (void) fetch_browse_page(state);
  } else if ((key == 'p' || key == 'P') && browse_start > 0) {
    if (browse_start > CONFIG_NIO_MAX_ENTRIES)
      browse_start = (uint16_t) (browse_start - CONFIG_NIO_MAX_ENTRIES);
    else
      browse_start = 0;
    (void) fetch_browse_page(state);
  } else if (key == 'u' || key == 'U') {
    parent_path(state->browse_path);
    browse_start = 0;
    (void) fetch_browse_page(state);
  } else if (key == 'a' || key == 'A') {
    assign_selected_file(state);
  } else if ((key == '\r' || key == '\n') && state->entry_count > 0) {
    if (state->entries[selected_entry].is_dir) {
      if (enter_dir(state, state->entries[selected_entry].name)) {
        browse_start = 0;
        (void) fetch_browse_page(state);
      }
    } else {
      assign_selected_file(state);
    }
  }
}

static void handle_slots(config_nio_state_t *state, int key)
{
  if ((key == 'w' || key == 'W') && selected_drive > 0)
    selected_drive--;
  else if (key == 's' && selected_drive + 1 < FNCTL_MAX_UNITS)
    selected_drive++;
  else if (key >= '0' && key <= '7') {
    state->mappings[selected_drive].valid = 1;
    state->mappings[selected_drive].slot = (uint8_t) (key - '0');
    (void) config_nio_save_mappings(state);
    config_nio_set_status(state, "Mapping saved");
  } else if (key == 'r' || key == 'R') {
    state->mappings[selected_drive].readonly =
      (uint8_t) !state->mappings[selected_drive].readonly;
    if (state->mappings[selected_drive].valid)
      (void) config_nio_save_mappings(state);
  } else if (key == 'c' || key == 'C') {
    memset(&state->mappings[selected_drive], 0, sizeof(state->mappings[selected_drive]));
    (void) config_nio_save_mappings(state);
    config_nio_set_status(state, "Mapping cleared");
  } else if (key == 'e' || key == 'E') {
    edit_slot(state);
  }
}

void config_nio_run(config_nio_state_t *state)
{
  int done;

  mode7();
  current_screen = SCREEN_HOSTS;
  selected_host = 0;
  selected_drive = 0;
  done = 0;

  while (!done) {
    int key;

    if (current_screen == SCREEN_HOSTS)
      show_hosts(state);
    else if (current_screen == SCREEN_BROWSE)
      draw_browse(state);
    else
      draw_slots(state);

    key = cgetc();
    if (key_is_quit(key)) {
      done = 1;
    } else if (key == 'H') {
      current_screen = SCREEN_HOSTS;
    } else if (key == 'S') {
      current_screen = SCREEN_SLOTS;
    } else if (key == 'x' || key == 'X') {
      (void) config_nio_mount_mappings(state);
      done = 1;
    } else if (current_screen == SCREEN_HOSTS) {
      handle_hosts(state, key);
    } else if (current_screen == SCREEN_BROWSE) {
      handle_browse(state, key);
    } else {
      handle_slots(state, key);
    }
  }
  clrscr();
}

int config_nio_ui_run(config_nio_state_t *state)
{
  (void) state;
  return 0;
}

void config_nio_ui_clear(void)
{
  clrscr();
}

void config_nio_ui_header(const char *title, const char *hint)
{
  header(title ? title : "");
  if (hint && *hint)
    status_line(hint);
}

void config_nio_ui_status(const char *status)
{
  status_line(status);
}

void config_nio_ui_pause(void)
{
  (void) cgetc();
}

int config_nio_ui_get_key(void)
{
  return cgetc();
}

int config_nio_ui_prompt(const char *label, char *buf, uint16_t cap)
{
  return prompt_line(label, buf, cap);
}

void config_nio_ui_putc(char c)
{
  cputc(c);
}

void config_nio_ui_print(const char *s)
{
  cputs(s ? s : "");
}

void config_nio_ui_println(const char *s)
{
  config_nio_ui_print(s);
  nl();
}

void config_nio_ui_print_uint(unsigned value)
{
  put_uint(value);
}

void config_nio_ui_print_ulong(unsigned long value)
{
  put_uint((unsigned) value);
}

void config_nio_ui_print_padded(const char *s, uint8_t width)
{
  put_fixed(s, width);
}

const char *config_nio_ui_platform_name(void)
{
  return "BBC Mode 7";
}

uint8_t config_nio_ui_screen_width(void)
{
  return BBC_WIDTH;
}

uint8_t config_nio_ui_screen_height(void)
{
  return BBC_ROWS;
}

void config_nio_ui_drive_label(uint8_t unit, char *buf, uint8_t cap)
{
  if (!buf || cap < 7)
    return;
  strcpy(buf, "Drive0");
  buf[5] = (char) ('0' + unit);
}

int config_nio_ui_show_mappings(config_nio_state_t *state)
{
  (void) state;
  return 0;
}
