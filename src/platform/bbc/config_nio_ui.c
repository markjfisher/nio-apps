#include "config_nio.h"

#include <bbc.h>
#include <conio.h>
#include <string.h>

#define BBC_WIDTH 40
#define BBC_ROWS 24
#define BBC_DRIVE_COUNT 4
#define BBC_BROWSE_PAGE_ROWS 10
#define BBC_URI_WORK_MAX 64
#define BBC_LIST_PAYLOAD 120

enum {
  SCREEN_HOSTS,
  SCREEN_BROWSE,
  SCREEN_SLOTS
};

#ifndef CONFIG_NIO_BBC_LITE
static char num_buf[6];
#endif
static char edit_buf[CONFIG_NIO_URI_MAX + 1];
static char uri_buf[BBC_URI_WORK_MAX];
static uint8_t current_screen;
static uint8_t selected_host;
static uint8_t selected_entry;
static uint8_t selected_drive;
static uint8_t selected_slot;
static uint8_t slots_focus;
static uint8_t browse_host;
static uint16_t browse_start;
static uint16_t browse_next;
static uint8_t browse_more;

#ifndef CONFIG_NIO_BBC_LITE
static void nl(void)
{
  cputc('\n');
}
#endif

static void mode7(void)
{
  cputc(22);
#ifdef CONFIG_NIO_BBC_SHADOW_MODE
  cputc(135);
#else
  cputc(7);
#endif
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

static void put_basename(const char *uri, uint8_t width)
{
  const char *leaf;
  const char *p;

  if (!uri || !*uri) {
    put_fixed("", width);
    return;
  }

  leaf = uri;
  p = uri;
  while (*p) {
    if (*p == '/' && p[1])
      leaf = p + 1;
    p++;
  }
  if (!*leaf)
    leaf = uri;
  put_tail(leaf, width);
}

#ifndef CONFIG_NIO_BBC_LITE
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
#endif

static void header(const char *screen)
{
  uint8_t i;

  text_at(0, 0, "CONFNIO ");
  cputs(screen);
  text_at(0, 1, "H Hosts  S Slots  X Mount  Q Quit");
  gotoxy(0, 2);
  for (i = 0; i < BBC_WIDTH; i++)
    cputc('-');
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

static int key_is_up(int key)
{
  return key == CH_CURS_UP || key == 'w' || key == 'W';
}

static int key_is_down(int key)
{
  return key == CH_CURS_DOWN || key == 's';
}

int bbc_read_line(char *buf, uint8_t size, uint8_t min_char, uint8_t max_char);

static int prompt_slot(uint8_t *slot_out)
{
  int key;

  if (!slot_out)
    return 0;

  clear_line(22);
  cputs("Slot 0-7: ");
  for (;;) {
    key = cgetc();
    if (key == CH_ESC) {
      clear_line(22);
      return 0;
    }
    if (key == CH_EOL || key == '\r' || key == '\n')
      continue;
    if (key >= '0' && key <= '7') {
      cputc((char) key);
      *slot_out = (uint8_t) (key - '0');
      return 1;
    }
  }
}

static int prompt_line(const char *label, char *buf, uint16_t cap)
{
  uint16_t i;
  uint8_t width;

  if (!buf || cap == 0)
    return 0;

  width = (uint8_t) (BBC_WIDTH - 14);
  if (cap <= width)
    width = (uint8_t) (cap - 1);
  if (width == 0)
    return 0;

  buf[0] = 0;
  buf[width] = 0;

  clear_line(22);
  put_fixed(label ? label : "Value", 12);
  cputs(": ");

  if (!bbc_read_line(buf, width, 32, 126)) {
    clear_line(22);
    return 0;
  }

  for (i = 0; i < width; i++) {
    if (buf[i] == 0 || buf[i] == '\r' || buf[i] == '\n') {
      buf[i] = 0;
      break;
    }
  }
  buf[width] = 0;

  clear_line(22);
  return 1;
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
    config_nio_set_status(state, "Path long");
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

static int fetch_browse_page(config_nio_state_t *state)
{
  uint8_t ok;

  state->entry_count = 0;
  state->entries_truncated = 0;
  if (!config_nio_host_get(state, browse_host, edit_buf, sizeof(edit_buf)) ||
      !config_nio_compose_uri(edit_buf, state->browse_path,
                              "", uri_buf, sizeof(uri_buf))) {
    config_nio_set_status(state, "Path long");
    return 0;
  }

  ok = (uint8_t) fnsvc_config_nio_list_directory_page(state, uri_buf,
                                                      browse_start,
                                                      BBC_BROWSE_PAGE_ROWS,
                                                      &browse_next,
                                                      &browse_more);
  if (!ok) {
    config_nio_set_status(state, "Host error");
    return 0;
  }

  selected_entry = 0;
  config_nio_set_status(state, browse_more ? "More" : "End");
  return 1;
}

static void fetch_previous_browse_page(config_nio_state_t *state)
{
  if (browse_start > BBC_BROWSE_PAGE_ROWS)
    browse_start = (uint16_t) (browse_start - BBC_BROWSE_PAGE_ROWS);
  else
    browse_start = 0;
  if (fetch_browse_page(state) && state->entry_count > 0)
    selected_entry = (uint8_t) (state->entry_count - 1);
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
    if (i >= 10)
      cputc('1');
    else
      cputc(' ');
    cputc((char) ('0' + (i % 10)));
    cputc(' ');
    if (i < state->host_count && config_nio_host_get(state, i, edit_buf, sizeof(edit_buf)))
      put_tail(edit_buf, 34);
    else
      put_fixed("", 34);
  }
  status_line("RET open A add E edit D del");
}

static void set_host_marker(uint8_t row, uint8_t selected)
{
  gotoxy(0, (uint8_t) (5 + row));
  cputc(selected ? '>' : ' ');
}

static void draw_browse(config_nio_state_t *state)
{
  uint8_t i;
  config_nio_entry_t entry;

  clrscr();
  header("Browse");
  text_at(0, 3, "Host ");
  if (config_nio_host_get(state, browse_host, edit_buf, sizeof(edit_buf)))
    put_tail(edit_buf, 33);
  text_at(0, 4, "Path /");
  put_tail(state->browse_path, 33);
  for (i = 0; i < BBC_BROWSE_PAGE_ROWS; i++) {
    gotoxy(0, (uint8_t) (6 + i));
    if (i < state->entry_count &&
        config_nio_entry_get(state, i, &entry)) {
      cputc(i == selected_entry ? '>' : ' ');
      cputc(entry.is_dir ? '/' : ' ');
      cputc(' ');
      put_tail(entry.name, 36);
    } else {
      put_fixed("", BBC_WIDTH);
    }
  }
  status_line("Arrows move RET open A slot U up");
}

static void set_browse_marker(uint8_t row, uint8_t selected)
{
  gotoxy(0, (uint8_t) (6 + row));
  cputc(selected ? '>' : ' ');
}

static void draw_slots(config_nio_state_t *state)
{
  uint8_t i;
  config_nio_mapping_t mapping;
  config_nio_slot_t slot;

  (void) state;
  clrscr();
  header("Slots");
  text_at(0, 3, "Drive map");
  for (i = 0; i < BBC_DRIVE_COUNT; i++) {
    gotoxy(0, (uint8_t) (5 + i));
    cputc((!slots_focus && i == selected_drive) ? '>' : ' ');
    cputs("Drive");
    cputc((char) ('0' + i));
    cputc(' ');
    if (config_nio_mapping_get(state, i, &mapping) && mapping.valid) {
      cputs("S");
      cputc((char) ('0' + mapping.slot));
      cputc(' ');
      cputc(mapping.readonly ? 'R' : 'W');
      cputc(' ');
      if (config_nio_slot_get(state, mapping.slot, &slot))
        put_basename(slot.uri, 27);
      else
        put_fixed("", 27);
    } else {
      cputs("--");
    }
  }
  clear_line(9);
  text_at(0, 10, "Slots");
  for (i = 0; i < FNCTL_MAX_UNITS; i++) {
    gotoxy(0, (uint8_t) (11 + i));
    cputc((slots_focus && i == selected_slot) ? '>' : ' ');
    cputc(' ');
    cputc((char) ('0' + i));
    cputc(' ');
    if (config_nio_slot_get(state, i, &slot))
      put_tail(slot.uri, 35);
    else
      put_fixed("", 35);
  }
  status_line(slots_focus ? "Arrows slot E hosts C clr TAB rows" : "Arrows drv 0-7 slot TAB rows");
}

static void set_drive_marker(uint8_t row, uint8_t selected)
{
  gotoxy(0, (uint8_t) (5 + row));
  cputc(selected ? '>' : ' ');
}

static void set_slot_marker(uint8_t row, uint8_t selected)
{
  gotoxy(0, (uint8_t) (11 + row));
  cputc(selected ? '>' : ' ');
}

static void edit_host(config_nio_state_t *state)
{
  if (selected_host >= CONFIG_NIO_MAX_HOSTS) {
    config_nio_set_status(state, "Host full");
    return;
  }
  if (selected_host < state->host_count)
    config_nio_set_status(state, "Re-enter host");
  edit_buf[0] = 0;
  if (!prompt_line("Host", edit_buf, sizeof(edit_buf)) || !edit_buf[0])
    return;
  (void) config_nio_host_set(state, selected_host, edit_buf);
  if (selected_host >= state->host_count)
    state->host_count = (uint8_t) (selected_host + 1);
  (void) config_nio_save_hosts(state);
  config_nio_set_status(state, "Saved");
}

static void clear_host(config_nio_state_t *state)
{
  uint8_t i;

  if (selected_host >= state->host_count)
    return;
  for (i = selected_host; i + 1 < state->host_count; i++) {
    if (config_nio_host_get(state, (uint8_t) (i + 1), edit_buf, sizeof(edit_buf)))
      (void) config_nio_host_set(state, i, edit_buf);
  }
  state->host_count--;
  (void) config_nio_host_clear(state, state->host_count);
  if (selected_host >= state->host_count && selected_host > 0)
    selected_host--;
  (void) config_nio_save_hosts(state);
  config_nio_set_status(state, "Cleared");
}

static void assign_selected_file(config_nio_state_t *state)
{
  uint8_t slot;
  config_nio_entry_t entry;

  if (state->entry_count == 0 ||
      !config_nio_entry_get(state, selected_entry, &entry) ||
      entry.is_dir) {
    config_nio_set_status(state, "Pick file");
    return;
  }
  if (!prompt_slot(&slot)) {
    config_nio_set_status(state, "Bad slot");
    return;
  }
  if (!config_nio_host_get(state, browse_host, edit_buf, sizeof(edit_buf)) ||
      !config_nio_compose_uri(edit_buf, state->browse_path,
                              entry.name,
                              uri_buf, sizeof(uri_buf))) {
    config_nio_set_status(state, "URI long");
    return;
  }
  if (!fnsvc_set_mount(slot, uri_buf, "rw", 1)) {
    config_nio_set_status(state, "Save fail");
    return;
  }
  (void) config_nio_refresh_slots(state);
  config_nio_set_status(state, "Assigned");
}

static uint8_t handle_hosts(config_nio_state_t *state, int key)
{
  uint8_t old;

  old = selected_host;
  if (key_is_up(key) && selected_host > 0) {
    selected_host--;
    set_host_marker(old, 0);
    set_host_marker(selected_host, 1);
    return 0;
  } else if (key_is_down(key) && selected_host + 1 < CONFIG_NIO_MAX_HOSTS) {
    selected_host++;
    set_host_marker(old, 0);
    set_host_marker(selected_host, 1);
    return 0;
  } else if (key == 'a' || key == 'A') {
    if (state->host_count >= CONFIG_NIO_MAX_HOSTS) {
        config_nio_set_status(state, "Host full");
      return 1;
    }
    selected_host = state->host_count;
    edit_host(state);
    return 1;
  } else if (key == 'e' || key == 'E') {
    edit_host(state);
    return 1;
  } else if (key == 'd' || key == 'D' || key == 'c' || key == 'C') {
    clear_host(state);
    return 1;
  } else if (key == '\r' || key == '\n') {
    if (selected_host < state->host_count) {
      browse_host = selected_host;
      state->browse_path[0] = 0;
      browse_start = 0;
      if (fetch_browse_page(state))
        current_screen = SCREEN_BROWSE;
      else
        pause_line("Host error");
      return 1;
    }
  }
  return 0;
}

static uint8_t handle_browse(config_nio_state_t *state, int key)
{
  uint8_t old;

  if (key == 'H') {
    current_screen = SCREEN_HOSTS;
    return 1;
  }
  old = selected_entry;
  if (key_is_up(key)) {
    if (selected_entry > 0) {
      selected_entry--;
      set_browse_marker(old, 0);
      set_browse_marker(selected_entry, 1);
      return 0;
    } else if (browse_start > 0) {
      fetch_previous_browse_page(state);
      return 1;
    }
  } else if (key_is_down(key)) {
    if (selected_entry + 1 < state->entry_count) {
      selected_entry++;
      set_browse_marker(old, 0);
      set_browse_marker(selected_entry, 1);
      return 0;
    } else if (browse_more) {
      browse_start = browse_next;
      (void) fetch_browse_page(state);
      return 1;
    }
  } else if ((key == 'n' || key == 'N') && browse_more) {
    browse_start = browse_next;
    (void) fetch_browse_page(state);
    return 1;
  } else if ((key == 'p' || key == 'P') && browse_start > 0) {
    fetch_previous_browse_page(state);
    return 1;
  } else if (key == 'u' || key == 'U') {
    parent_path(state->browse_path);
    browse_start = 0;
    (void) fetch_browse_page(state);
    return 1;
  } else if (key == 'a' || key == 'A') {
    assign_selected_file(state);
    return 1;
  } else if ((key == '\r' || key == '\n') && state->entry_count > 0) {
    config_nio_entry_t entry;

    if (config_nio_entry_get(state, selected_entry, &entry) &&
        entry.is_dir) {
      if (enter_dir(state, entry.name)) {
        browse_start = 0;
        (void) fetch_browse_page(state);
      }
    } else {
      assign_selected_file(state);
    }
    return 1;
  }
  return 0;
}

static uint8_t handle_slots(config_nio_state_t *state, int key)
{
  uint8_t old;
  config_nio_mapping_t mapping;

  if (key == '	') {
    if (slots_focus)
      set_slot_marker(selected_slot, 0);
    else
      set_drive_marker(selected_drive, 0);
    slots_focus = (uint8_t) !slots_focus;
    if (slots_focus)
      set_slot_marker(selected_slot, 1);
    else
      set_drive_marker(selected_drive, 1);
    status_line(slots_focus ? "Arrows slot E hosts C clr TAB rows" : "Arrows drv 0-7 slot TAB rows");
    return 0;
  }
  if (slots_focus) {
    if (key_is_up(key) && selected_slot > 0) {
      old = selected_slot;
      selected_slot--;
      set_slot_marker(old, 0);
      set_slot_marker(selected_slot, 1);
      return 0;
    } else if (key_is_down(key) && selected_slot + 1 < FNCTL_MAX_UNITS) {
      old = selected_slot;
      selected_slot++;
      set_slot_marker(old, 0);
      set_slot_marker(selected_slot, 1);
      return 0;
    } else if (key == 'c' || key == 'C') {
      uint8_t unit;

      if (!fnsvc_set_mount(selected_slot, "", "rw", 0)) {
        config_nio_set_status(state, "Clear fail");
        return 1;
      }
      for (unit = 0; unit < FNCTL_MAX_UNITS; unit++) {
        if (config_nio_mapping_get(state, unit, &mapping) &&
            mapping.valid && mapping.slot == selected_slot)
          (void) config_nio_mapping_clear(state, unit);
      }
      (void) config_nio_save_mappings(state);
      (void) config_nio_refresh_slots(state);
      config_nio_set_status(state, "Cleared");
      return 1;
    } else if (key == 'e' || key == 'E') {
      config_nio_set_status(state, "Change via Hosts");
      return 1;
    }
    return 0;
  }

  old = selected_drive;
  if (key_is_up(key) && selected_drive > 0) {
    selected_drive--;
    set_drive_marker(old, 0);
    set_drive_marker(selected_drive, 1);
    return 0;
  } else if (key_is_down(key) && selected_drive + 1 < BBC_DRIVE_COUNT) {
    selected_drive++;
    set_drive_marker(old, 0);
    set_drive_marker(selected_drive, 1);
    return 0;
  } else if (key >= '0' && key <= '7') {
    mapping.valid = 1;
    mapping.slot = (uint8_t) (key - '0');
    mapping.readonly = 0;
    (void) config_nio_mapping_set(state, selected_drive, &mapping);
    (void) config_nio_save_mappings(state);
    config_nio_set_status(state, "Map saved");
    return 1;
  } else if (key == 'r' || key == 'R') {
    if (config_nio_mapping_get(state, selected_drive, &mapping) &&
        mapping.valid) {
      mapping.readonly = (uint8_t) !mapping.readonly;
      (void) config_nio_mapping_set(state, selected_drive, &mapping);
      (void) config_nio_save_mappings(state);
    }
    return 1;
  } else if (key == 'c' || key == 'C') {
    (void) config_nio_mapping_clear(state, selected_drive);
    (void) config_nio_save_mappings(state);
    config_nio_set_status(state, "Map clear");
    return 1;
  }
  return 0;
}

void config_nio_run(config_nio_state_t *state)
{
  int done;

  mode7();
  current_screen = SCREEN_HOSTS;
  selected_host = 0;
  selected_drive = 0;
  selected_slot = 0;
  slots_focus = 0;
  done = 0;
  show_hosts(state);

  while (!done) {
    int key;
    uint8_t redraw;

    key = cgetc();
    redraw = 0;
    if (key_is_quit(key)) {
      done = 1;
    } else if (key == 'H') {
      current_screen = SCREEN_HOSTS;
      redraw = 1;
    } else if (key == 'S') {
      current_screen = SCREEN_SLOTS;
      redraw = 1;
    } else if (key == 'x' || key == 'X') {
      (void) config_nio_mount_mappings(state);
      done = 1;
    } else if (current_screen == SCREEN_HOSTS) {
      redraw = handle_hosts(state, key);
    } else if (current_screen == SCREEN_BROWSE) {
      redraw = handle_browse(state, key);
    } else {
      redraw = handle_slots(state, key);
    }

    if (redraw && !done) {
      if (current_screen == SCREEN_HOSTS)
        show_hosts(state);
      else if (current_screen == SCREEN_BROWSE)
        draw_browse(state);
      else
        draw_slots(state);
    }
  }
  clrscr();
}

#ifndef CONFIG_NIO_BBC_LITE
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
  return "BBC M7";
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
#endif
