#include "config_nio.h"

#include <conio.h>
#include <curses.h>
#include <ctype.h>
#include <stdio.h>
#include <string.h>

#undef getch
#undef ungetch

#define DOS_KEY_TAB 9
#define DOS_KEY_ENTER 13
#define DOS_KEY_ESC 27

static char dos_drive_label[16];
static char dos_uri_edit[CONFIG_NIO_URI_MAX + 1];
static char dos_mode_edit[4];
static uint8_t dos_selected_slot;
static uint8_t dos_selected_unit;
static uint8_t dos_focus;

#define DOS_COLOR_BODY 1
#define DOS_COLOR_FRAME 2
#define DOS_COLOR_TITLE 3
#define DOS_COLOR_SELECT 4
#define DOS_COLOR_STATUS 5
#define DOS_COLOR_WARN 6

#define DOS_SCREEN_DASHBOARD 0
#define DOS_SCREEN_HOSTS 1
#define DOS_SCREEN_BROWSE 2
#define DOS_SCREEN_SLOTS 3
#define DOS_SCREEN_MAP 4

static WINDOW *dos_main_win;
static WINDOW *dos_side_win;
static WINDOW *dos_status_win;
static uint8_t dos_screen;
static int dos_last_screen = -1;
static uint8_t dos_selected_host;
static uint8_t dos_selected_entry;
static uint8_t dos_browser_host;
static uint8_t dos_browser_loaded;
static char dos_uri_buf[FNSVC_MAX_URI + 1];

static void dos_map_selected(config_nio_state_t *state);
static void dos_toggle_mapping_mode(config_nio_state_t *state);
static void dos_clear_mapping(config_nio_state_t *state);
static void dos_clear_slot(config_nio_state_t *state);
static void dos_edit_slot(config_nio_state_t *state);

static void dos_print_clip(const char *s, uint8_t width)
{
  uint8_t n;

  n = 0;
  while (s && *s && n < width) {
    putchar(*s++);
    n++;
  }
  while (n < width) {
    putchar(' ');
    n++;
  }
}

static void dos_curses_print_clip(WINDOW *win, const char *s, int width)
{
  int n;

  n = 0;
  while (s && *s && n < width) {
    waddch(win, *s++);
    n++;
  }
  while (n < width) {
    waddch(win, ' ');
    n++;
  }
}

static void dos_curses_print_tail(WINDOW *win, const char *s, int width)
{
  uint16_t len;

  if (!s)
    s = "";
  len = (uint16_t) strlen(s);
  if (len <= (uint16_t) width) {
    dos_curses_print_clip(win, s, width);
    return;
  }
  if (width <= 1)
    return;
  waddch(win, '~');
  dos_curses_print_clip(win, s + len - (width - 1), width - 1);
}

static int dos_curses_prompt(const char *title, const char *label,
                             char *buf, uint16_t cap)
{
  WINDOW *modal;
  int result;

  modal = newwin(7, 64, 8, 8);
  if (!modal)
    return 0;
  werase(modal);
  wbkgd(modal, COLOR_PAIR(DOS_COLOR_STATUS));
  wattrset(modal, COLOR_PAIR(DOS_COLOR_STATUS));
  box(modal, 0, 0);
  mvwaddnstr(modal, 0, 2, title ? title : " Edit ", 58);
  mvwaddnstr(modal, 2, 2, label ? label : "Value", 14);
  mvwaddnstr(modal, 3, 2, buf ? buf : "", 58);
  mvwaddstr(modal, 5, 2, "Enter accepts");
  wmove(modal, 3, 2);
  echo();
  curs_set(1);
  wgetnstr(modal, buf, (int) cap - 1);
  noecho();
  curs_set(0);
  result = buf && buf[0];
  delwin(modal);
  return result;
}

static void dos_init_colors(void)
{
  if (!has_colors())
    return;
  start_color();
  init_pair(DOS_COLOR_BODY, COLOR_WHITE, COLOR_BLUE);
  init_pair(DOS_COLOR_FRAME, COLOR_CYAN, COLOR_BLUE);
  init_pair(DOS_COLOR_TITLE, COLOR_YELLOW, COLOR_BLUE);
  init_pair(DOS_COLOR_SELECT, COLOR_BLACK, COLOR_CYAN);
  init_pair(DOS_COLOR_STATUS, COLOR_BLACK, COLOR_CYAN);
  init_pair(DOS_COLOR_WARN, COLOR_YELLOW, COLOR_RED);
}

static int dos_curses_start(void)
{
  initscr();
  dos_init_colors();
  cbreak();
  noecho();
  keypad(stdscr, TRUE);
  curs_set(0);
  bkgd(COLOR_PAIR(DOS_COLOR_BODY));
  return 1;
}

static void dos_curses_stop(void)
{
  if (dos_main_win) {
    delwin(dos_main_win);
    dos_main_win = NULL;
  }
  if (dos_side_win) {
    delwin(dos_side_win);
    dos_side_win = NULL;
  }
  if (dos_status_win) {
    delwin(dos_status_win);
    dos_status_win = NULL;
  }
  endwin();
}

static void dos_app_create_windows(void)
{
  if (!dos_main_win)
    dos_main_win = newwin(17, 51, 2, 1);
  if (!dos_side_win)
    dos_side_win = newwin(17, 26, 2, 53);
  if (!dos_status_win)
    dos_status_win = newwin(5, 78, 19, 1);
  keypad(dos_main_win, TRUE);
  keypad(dos_side_win, TRUE);
  keypad(dos_status_win, TRUE);
}

static void dos_win_title(WINDOW *win, const char *title)
{
  wattrset(win, COLOR_PAIR(DOS_COLOR_FRAME));
  box(win, 0, 0);
  wattrset(win, COLOR_PAIR(DOS_COLOR_TITLE));
  mvwaddnstr(win, 0, 2, title, 22);
}

static void dos_draw_status(const char *help, const char *status)
{
  werase(dos_status_win);
  wbkgd(dos_status_win, COLOR_PAIR(DOS_COLOR_STATUS));
  wattrset(dos_status_win, COLOR_PAIR(DOS_COLOR_STATUS));
  box(dos_status_win, 0, 0);
  mvwaddnstr(dos_status_win, 1, 2, help ? help : "", 74);
  mvwaddnstr(dos_status_win, 2, 2, status && *status ? status : "Ready", 74);
  mvwaddnstr(dos_status_win, 3, 2, "H Hosts  B Browse  S Slots  M Map  X Mount+Exit  Q Quit", 74);
  wnoutrefresh(dos_status_win);
}

static void dos_draw_slot_line(WINDOW *win, int y, uint8_t slot,
                               const config_nio_slot_t *mount, int selected)
{
  if (selected)
    wattrset(win, COLOR_PAIR(DOS_COLOR_SELECT));
  else
    wattrset(win, COLOR_PAIR(DOS_COLOR_BODY));
  mvwprintw(win, y, 2, "%u ", (unsigned) slot);
  dos_curses_print_clip(win, mount->enabled ? mount->mode : "--", 2);
  waddch(win, ' ');
  if (mount->enabled && mount->uri[0]) {
    dos_curses_print_tail(win, mount->uri, 39);
  } else {
    dos_curses_print_clip(win, "(empty)", 39);
  }
}

static void dos_draw_mapping_line(WINDOW *win, int y, config_nio_state_t *state,
                                  uint8_t unit, int selected)
{
  config_nio_mapping_t *mapping;

  mapping = &state->mappings[unit];
  config_nio_ui_drive_label(unit, dos_drive_label, sizeof(dos_drive_label));
  if (selected)
    wattrset(win, COLOR_PAIR(DOS_COLOR_SELECT));
  else
    wattrset(win, COLOR_PAIR(DOS_COLOR_BODY));
  mvwaddnstr(win, y, 2, dos_drive_label, 5);
  if (mapping->valid) {
    wprintw(win, "->%u %s ", (unsigned) mapping->slot,
            mapping->readonly ? "ro" : "rw");
    if (mapping->slot < FNCTL_MAX_UNITS && state->slots[mapping->slot].uri[0])
      dos_curses_print_tail(win, state->slots[mapping->slot].uri, 12);
    else
      dos_curses_print_clip(win, "(empty)", 12);
  } else {
    dos_curses_print_clip(win, "(not mapped)", 17);
  }
}

static void dos_draw_side_mappings(config_nio_state_t *state)
{
  uint8_t i;

  werase(dos_side_win);
  wbkgd(dos_side_win, COLOR_PAIR(DOS_COLOR_BODY));
  dos_win_title(dos_side_win, " Mappings ");
  for (i = 0; i < FNCTL_MAX_UNITS; i++)
    dos_draw_mapping_line(dos_side_win, (int) i + 2, state, i, 0);
  wnoutrefresh(dos_side_win);
}

static void dos_draw_dashboard(config_nio_state_t *state)
{
  uint8_t i;

  werase(dos_main_win);
  wbkgd(dos_main_win, COLOR_PAIR(DOS_COLOR_BODY));
  dos_win_title(dos_main_win, " Slots ");
  for (i = 0; i < FNCTL_MAX_UNITS; i++)
    dos_draw_slot_line(dos_main_win, (int) i + 2, i, &state->slots[i], 0);
  wnoutrefresh(dos_main_win);

  dos_draw_side_mappings(state);
  dos_draw_status("Dashboard: choose H/B/S/M, or X to mount mapped drives", state->status);
}

static void dos_draw_hosts(config_nio_state_t *state)
{
  uint8_t i;

  werase(dos_main_win);
  wbkgd(dos_main_win, COLOR_PAIR(DOS_COLOR_BODY));
  dos_win_title(dos_main_win, " Hosts ");
  for (i = 0; i < state->host_count; i++) {
    wattrset(dos_main_win, i == dos_selected_host ? COLOR_PAIR(DOS_COLOR_SELECT) : COLOR_PAIR(DOS_COLOR_BODY));
    mvwprintw(dos_main_win, (int) i + 2, 2, "%u ", (unsigned) i);
    dos_curses_print_tail(dos_main_win, state->hosts[i], 43);
  }
  if (state->host_count == 0) {
    wattrset(dos_main_win, COLOR_PAIR(DOS_COLOR_BODY));
    mvwaddstr(dos_main_win, 2, 2, "No hosts");
  }
  wnoutrefresh(dos_main_win);

  werase(dos_side_win);
  wbkgd(dos_side_win, COLOR_PAIR(DOS_COLOR_BODY));
  dos_win_title(dos_side_win, " Commands ");
  wattrset(dos_side_win, COLOR_PAIR(DOS_COLOR_BODY));
  mvwaddstr(dos_side_win, 2, 2, "Enter Browse");
  mvwaddstr(dos_side_win, 3, 2, "A Add");
  mvwaddstr(dos_side_win, 4, 2, "E Edit");
  mvwaddstr(dos_side_win, 5, 2, "D Delete host");
  mvwaddstr(dos_side_win, 6, 2, "+/- Reorder");
  wnoutrefresh(dos_side_win);

  dos_draw_status("Hosts: arrows move  Enter browse  A add  E edit  D delete  +/- move", state->status);
}

static void dos_draw_slots_screen(config_nio_state_t *state)
{
  uint8_t i;

  werase(dos_main_win);
  wbkgd(dos_main_win, COLOR_PAIR(DOS_COLOR_BODY));
  dos_win_title(dos_main_win, " Slots ");
  for (i = 0; i < FNCTL_MAX_UNITS; i++)
    dos_draw_slot_line(dos_main_win, (int) i + 2, i, &state->slots[i],
                       dos_selected_slot == i);
  wnoutrefresh(dos_main_win);

  dos_draw_side_mappings(state);
  dos_draw_status("Slots: arrows/W/S move  E edit URI  D clear selected slot  R refresh", state->status);
}

static void dos_draw_map_screen(config_nio_state_t *state)
{
  uint8_t i;

  werase(dos_main_win);
  wbkgd(dos_main_win, COLOR_PAIR(DOS_COLOR_BODY));
  dos_win_title(dos_main_win, dos_focus == 0 ? " Slots ACTIVE " : " Slots ");
  for (i = 0; i < FNCTL_MAX_UNITS; i++)
    dos_draw_slot_line(dos_main_win, (int) i + 2, i, &state->slots[i],
                       dos_selected_slot == i);
  wattrset(dos_main_win, COLOR_PAIR(DOS_COLOR_TITLE));
  mvwaddstr(dos_main_win, 12, 2, "Selected slot");
  wattrset(dos_main_win, COLOR_PAIR(DOS_COLOR_BODY));
  mvwprintw(dos_main_win, 13, 2, "%u: ", (unsigned) dos_selected_slot);
  if (state->slots[dos_selected_slot].enabled &&
      state->slots[dos_selected_slot].uri[0])
    dos_curses_print_tail(dos_main_win, state->slots[dos_selected_slot].uri, 42);
  else
    dos_curses_print_clip(dos_main_win, "(empty)", 42);
  wnoutrefresh(dos_main_win);

  werase(dos_side_win);
  wbkgd(dos_side_win, COLOR_PAIR(DOS_COLOR_BODY));
  dos_win_title(dos_side_win, dos_focus == 1 ? " Drives ACTIVE " : " Drives ");
  for (i = 0; i < FNCTL_MAX_UNITS; i++)
    dos_draw_mapping_line(dos_side_win, (int) i + 2, state, i,
                          dos_selected_unit == i);
  wattrset(dos_side_win, COLOR_PAIR(DOS_COLOR_TITLE));
  mvwaddstr(dos_side_win, 11, 2, "Actions");
  wattrset(dos_side_win, COLOR_PAIR(DOS_COLOR_BODY));
  mvwaddstr(dos_side_win, 12, 2, "Enter: map pair");
  mvwaddstr(dos_side_win, 13, 2, "C: clear drive map");
  mvwaddstr(dos_side_win, 14, 2, "D: clear slot URI");
  mvwaddstr(dos_side_win, 15, 2, "R: ro/rw");
  wnoutrefresh(dos_side_win);

  dos_draw_status("Map: Tab/Left/Right pane  Arrows/W/S move  Enter maps slot->drive", state->status);
}

static void dos_list_cb(uint8_t is_dir, const char *name, uint32_t size,
                        uint32_t mtime, void *ctx)
{
  config_nio_state_t *state;
  config_nio_entry_t *entry;
  uint16_t n;

  (void) mtime;
  state = (config_nio_state_t *) ctx;
  state->entry_total++;
  if (state->entry_count >= CONFIG_NIO_MAX_ENTRIES) {
    state->entries_truncated = 1;
    return;
  }
  entry = &state->entries[state->entry_count++];
  entry->is_dir = is_dir;
  entry->size = size;
  n = (uint16_t) strlen(name);
  if (n > CONFIG_NIO_NAME_MAX)
    n = CONFIG_NIO_NAME_MAX;
  memcpy(entry->name, name, n);
  entry->name[n] = 0;
}

static int dos_refresh_entries(config_nio_state_t *state)
{
  state->entry_count = 0;
  state->entry_total = 0;
  state->entries_truncated = 0;
  if (!config_nio_compose_uri(state->hosts[dos_browser_host],
                              state->browse_path, "",
                              dos_uri_buf, sizeof(dos_uri_buf))) {
    config_nio_set_status(state, "Path is too long");
    return 0;
  }
  if (!fnsvc_list_directory(dos_uri_buf, dos_list_cb, state)) {
    config_nio_set_status(state, "List failed");
    return 0;
  }
  if (dos_selected_entry >= state->entry_count)
    dos_selected_entry = 0;
  config_nio_set_status(state, state->entries_truncated ? "Entries loaded (more)" : "Entries loaded");
  return 1;
}

static void dos_parent_path(char *path)
{
  uint16_t len;

  len = (uint16_t) strlen(path);
  while (len > 0 && path[len - 1] == '/')
    path[--len] = 0;
  while (len > 0 && path[len - 1] != '/')
    path[--len] = 0;
}

static int dos_enter_dir(config_nio_state_t *state, const char *name)
{
  uint16_t len;
  uint16_t nlen;

  len = (uint16_t) strlen(state->browse_path);
  nlen = (uint16_t) strlen(name);
  if ((uint16_t) (len + nlen + 2) > FNSVC_MAX_PATH) {
    config_nio_set_status(state, "Path is too long");
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

static void dos_draw_browser(config_nio_state_t *state)
{
  uint8_t i;
  uint8_t start;
  uint8_t row;

  werase(dos_main_win);
  wbkgd(dos_main_win, COLOR_PAIR(DOS_COLOR_BODY));
  dos_win_title(dos_main_win, " Browse ");
  wattrset(dos_main_win, COLOR_PAIR(DOS_COLOR_TITLE));
  wmove(dos_main_win, 1, 2);
  dos_curses_print_tail(dos_main_win, state->hosts[dos_browser_host], 22);
  waddch(dos_main_win, '/');
  dos_curses_print_tail(dos_main_win, state->browse_path, 20);
  start = 0;
  if (dos_selected_entry >= 12)
    start = (uint8_t) (dos_selected_entry - 11);
  row = 0;
  for (i = start; i < state->entry_count && row < 12; i++, row++) {
    config_nio_entry_t *entry;

    entry = &state->entries[i];
    wattrset(dos_main_win, i == dos_selected_entry ? COLOR_PAIR(DOS_COLOR_SELECT) : COLOR_PAIR(DOS_COLOR_BODY));
    mvwaddch(dos_main_win, (int) row + 3, 2, entry->is_dir ? '/' : ' ');
    dos_curses_print_clip(dos_main_win, entry->name, 31);
    if (!entry->is_dir)
      wprintw(dos_main_win, " %lu", (unsigned long) entry->size);
  }
  if (state->entry_count == 0) {
    wattrset(dos_main_win, COLOR_PAIR(DOS_COLOR_BODY));
    mvwaddstr(dos_main_win, 3, 2, "(empty)");
  }
  wnoutrefresh(dos_main_win);

  werase(dos_side_win);
  wbkgd(dos_side_win, COLOR_PAIR(DOS_COLOR_BODY));
  dos_win_title(dos_side_win, " Slots ");
  for (i = 0; i < FNCTL_MAX_UNITS; i++) {
    config_nio_slot_t *mount;

    mount = &state->slots[i];
    wattrset(dos_side_win, i == dos_selected_slot ? COLOR_PAIR(DOS_COLOR_SELECT) : COLOR_PAIR(DOS_COLOR_BODY));
    mvwprintw(dos_side_win, (int) i + 2, 2, "%u ", (unsigned) i);
    if (mount->enabled && mount->uri[0])
      dos_curses_print_tail(dos_side_win, mount->uri, 18);
    else
      dos_curses_print_clip(dos_side_win, "(empty)", 18);
  }
  wnoutrefresh(dos_side_win);
  dos_draw_status("Browse: arrows move  Enter dir  U up  R reload  A assign file to slot", state->status);
}

static void dos_draw_shell(void)
{
  if (dos_last_screen != (int) dos_screen) {
    erase();
    bkgd(COLOR_PAIR(DOS_COLOR_BODY));
    dos_last_screen = dos_screen;
  }
  attrset(COLOR_PAIR(DOS_COLOR_TITLE));
  mvaddstr(0, 2, "config-nio");
  clrtoeol();
  wnoutrefresh(stdscr);
}

static void dos_redraw_app(config_nio_state_t *state)
{
  dos_draw_shell();

  if (dos_screen == DOS_SCREEN_HOSTS)
    dos_draw_hosts(state);
  else if (dos_screen == DOS_SCREEN_BROWSE)
    dos_draw_browser(state);
  else if (dos_screen == DOS_SCREEN_SLOTS)
    dos_draw_slots_screen(state);
  else if (dos_screen == DOS_SCREEN_MAP)
    dos_draw_map_screen(state);
  else
    dos_draw_dashboard(state);
  doupdate();
}

static void dos_open_browser(config_nio_state_t *state, uint8_t host)
{
  if (host >= state->host_count) {
    config_nio_set_status(state, "No host selected");
    return;
  }
  dos_browser_host = host;
  dos_selected_host = host;
  dos_selected_entry = 0;
  state->browse_path[0] = 0;
  dos_browser_loaded = 1;
  (void) dos_refresh_entries(state);
  dos_screen = DOS_SCREEN_BROWSE;
}

static void dos_add_host(config_nio_state_t *state)
{
  if (state->host_count >= CONFIG_NIO_MAX_HOSTS) {
    config_nio_set_status(state, "Host list is full");
    return;
  }
  dos_uri_edit[0] = 0;
  if (dos_curses_prompt(" Add host ", "Host", dos_uri_edit, sizeof(dos_uri_edit)) &&
      dos_uri_edit[0]) {
    strcpy(state->hosts[state->host_count], dos_uri_edit);
    dos_selected_host = state->host_count;
    state->host_count++;
    (void) config_nio_save_hosts(state);
    config_nio_set_status(state, "Host added");
  }
}

static void dos_edit_host(config_nio_state_t *state)
{
  if (state->host_count == 0 || dos_selected_host >= state->host_count) {
    config_nio_set_status(state, "No host selected");
    return;
  }
  strcpy(dos_uri_edit, state->hosts[dos_selected_host]);
  if (dos_curses_prompt(" Edit host ", "Host", dos_uri_edit, sizeof(dos_uri_edit)) &&
      dos_uri_edit[0]) {
    strcpy(state->hosts[dos_selected_host], dos_uri_edit);
    (void) config_nio_save_hosts(state);
    config_nio_set_status(state, "Host updated");
  }
}

static void dos_delete_host(config_nio_state_t *state)
{
  uint8_t i;

  if (state->host_count == 0 || dos_selected_host >= state->host_count) {
    config_nio_set_status(state, "No host selected");
    return;
  }
  for (i = dos_selected_host; i + 1 < state->host_count; i++)
    strcpy(state->hosts[i], state->hosts[i + 1]);
  state->host_count--;
  if (dos_selected_host >= state->host_count && dos_selected_host > 0)
    dos_selected_host--;
  (void) config_nio_save_hosts(state);
  config_nio_set_status(state, "Host deleted");
}

static void dos_move_host(config_nio_state_t *state, int delta)
{
  uint8_t other;

  if (state->host_count == 0)
    return;
  if (delta < 0 && dos_selected_host == 0)
    return;
  if (delta > 0 && dos_selected_host + 1 >= state->host_count)
    return;
  other = (uint8_t) (dos_selected_host + delta);
  strcpy(dos_uri_buf, state->hosts[dos_selected_host]);
  strcpy(state->hosts[dos_selected_host], state->hosts[other]);
  strcpy(state->hosts[other], dos_uri_buf);
  dos_selected_host = other;
  (void) config_nio_save_hosts(state);
  config_nio_set_status(state, "Host moved");
}

static void dos_assign_selected_entry(config_nio_state_t *state)
{
  if (state->entry_count == 0 || dos_selected_entry >= state->entry_count)
    return;
  if (state->entries[dos_selected_entry].is_dir) {
    config_nio_set_status(state, "Pick a file, not a directory");
    return;
  }
  if (!config_nio_compose_uri(state->hosts[dos_browser_host], state->browse_path,
                              state->entries[dos_selected_entry].name,
                              dos_uri_buf, sizeof(dos_uri_buf))) {
    config_nio_set_status(state, "URI is too long");
    return;
  }
  if (!fnsvc_set_mount(dos_selected_slot, dos_uri_buf, "rw", 1)) {
    config_nio_set_status(state, "Unable to save slot");
    return;
  }
  (void) config_nio_refresh_slots(state);
  config_nio_set_status(state, "Assigned file to selected slot");
}

static void dos_browser_enter(config_nio_state_t *state)
{
  if (state->entry_count == 0 || dos_selected_entry >= state->entry_count)
    return;
  if (state->entries[dos_selected_entry].is_dir) {
    if (dos_enter_dir(state, state->entries[dos_selected_entry].name)) {
      dos_selected_entry = 0;
      (void) dos_refresh_entries(state);
    }
  } else {
    dos_assign_selected_entry(state);
  }
}

static void dos_handle_up(config_nio_state_t *state)
{
  (void) state;
  if (dos_screen == DOS_SCREEN_HOSTS && dos_selected_host > 0)
    dos_selected_host--;
  else if (dos_screen == DOS_SCREEN_BROWSE && dos_selected_entry > 0)
    dos_selected_entry--;
  else if ((dos_screen == DOS_SCREEN_SLOTS ||
            (dos_screen == DOS_SCREEN_MAP && dos_focus == 0)) &&
           dos_selected_slot > 0)
    dos_selected_slot--;
  else if (dos_screen == DOS_SCREEN_MAP && dos_focus == 1 && dos_selected_unit > 0)
    dos_selected_unit--;
}

static void dos_handle_down(config_nio_state_t *state)
{
  if (dos_screen == DOS_SCREEN_HOSTS && dos_selected_host + 1 < state->host_count)
    dos_selected_host++;
  else if (dos_screen == DOS_SCREEN_BROWSE && dos_selected_entry + 1 < state->entry_count)
    dos_selected_entry++;
  else if ((dos_screen == DOS_SCREEN_SLOTS ||
            (dos_screen == DOS_SCREEN_MAP && dos_focus == 0)) &&
           dos_selected_slot + 1 < FNCTL_MAX_UNITS)
    dos_selected_slot++;
  else if (dos_screen == DOS_SCREEN_MAP && dos_focus == 1 &&
           dos_selected_unit + 1 < FNCTL_MAX_UNITS)
    dos_selected_unit++;
}

static int dos_handle_key(config_nio_state_t *state, int key)
{
  if (key == 'q' || key == 'Q' || key == DOS_KEY_ESC) {
    if (dos_screen == DOS_SCREEN_DASHBOARD)
      return 0;
    dos_screen = DOS_SCREEN_DASHBOARD;
    return 1;
  }
  if (key == 'h' || key == 'H') {
    dos_screen = DOS_SCREEN_HOSTS;
    return 1;
  }
  if (key == 's' || key == 'S') {
    if (dos_screen == DOS_SCREEN_HOSTS || dos_screen == DOS_SCREEN_BROWSE ||
        dos_screen == DOS_SCREEN_SLOTS || dos_screen == DOS_SCREEN_MAP)
      dos_handle_down(state);
    else {
      dos_screen = DOS_SCREEN_SLOTS;
      config_nio_set_status(state, "Slots: edit or clear FujiNet slot entries");
    }
    return 1;
  }
  if (key == 'm' || key == 'M') {
    if (dos_screen == DOS_SCREEN_MAP)
      dos_map_selected(state);
    else {
      dos_screen = DOS_SCREEN_MAP;
      config_nio_set_status(state, "Map: pick a slot and drive, then press Enter");
    }
    return 1;
  }
  if (key == 'b' || key == 'B') {
    dos_open_browser(state, dos_selected_host);
    return 1;
  }
  if (key == 'x' || key == 'X') {
    (void) config_nio_mount_mappings(state);
    return 0;
  }
  if (key == KEY_UP || key == 'w' || key == 'W') {
    dos_handle_up(state);
    return 1;
  }
  if (key == KEY_DOWN) {
    dos_handle_down(state);
    return 1;
  }
  if ((key == DOS_KEY_TAB || key == KEY_LEFT || key == KEY_RIGHT) &&
      dos_screen == DOS_SCREEN_MAP) {
    dos_focus = (uint8_t) !dos_focus;
    config_nio_set_status(state, dos_focus ? "Drives pane active" : "Slots pane active");
    return 1;
  }

  if (dos_screen == DOS_SCREEN_HOSTS) {
    if (key == DOS_KEY_ENTER || key == '\n' || key == '\r') {
      dos_open_browser(state, dos_selected_host);
    } else if (key == 'a' || key == 'A') {
      dos_add_host(state);
    } else if (key == 'e' || key == 'E') {
      dos_edit_host(state);
    } else if (key == 'd' || key == 'D') {
      dos_delete_host(state);
    } else if (key == '+') {
      dos_move_host(state, 1);
    } else if (key == '-') {
      dos_move_host(state, -1);
    }
  } else if (dos_screen == DOS_SCREEN_BROWSE) {
    if (key == DOS_KEY_ENTER || key == '\n' || key == '\r' ||
        key == 'e' || key == 'E') {
      dos_browser_enter(state);
    } else if (key == 'u' || key == 'U') {
      dos_parent_path(state->browse_path);
      dos_selected_entry = 0;
      (void) dos_refresh_entries(state);
    } else if (key == 'r' || key == 'R') {
      (void) dos_refresh_entries(state);
    } else if (key == 'a' || key == 'A') {
      dos_assign_selected_entry(state);
    }
  } else if (dos_screen == DOS_SCREEN_SLOTS) {
    if (key == 'e' || key == 'E') {
      dos_edit_slot(state);
    } else if (key == 'd' || key == 'D') {
      dos_clear_slot(state);
    } else if (key == 'r' || key == 'R') {
      (void) config_nio_refresh_slots(state);
      config_nio_set_status(state, "Slots refreshed");
    }
  } else if (dos_screen == DOS_SCREEN_MAP) {
    if (key == DOS_KEY_ENTER || key == '\n' || key == '\r') {
      dos_map_selected(state);
    } else if (key == 'r' || key == 'R') {
      dos_toggle_mapping_mode(state);
    } else if (key == 'c' || key == 'C') {
      dos_clear_mapping(state);
    } else if (key == 'e' || key == 'E') {
      dos_edit_slot(state);
    } else if (key == 'd' || key == 'D') {
      dos_clear_slot(state);
    }
  }
  return 1;
}

static void dos_map_selected(config_nio_state_t *state)
{
  state->mappings[dos_selected_unit].valid = 1;
  state->mappings[dos_selected_unit].slot = dos_selected_slot;
  if (!state->mappings[dos_selected_unit].readonly)
    state->mappings[dos_selected_unit].readonly = 0;
  (void) config_nio_save_mappings(state);
  config_nio_set_status(state, "Mapped selected slot to selected drive");
}

static void dos_toggle_mapping_mode(config_nio_state_t *state)
{
  config_nio_mapping_t *mapping;

  mapping = &state->mappings[dos_selected_unit];
  if (!mapping->valid) {
    config_nio_set_status(state, "Select a mapped drive first");
    return;
  }
  mapping->readonly = (uint8_t) !mapping->readonly;
  (void) config_nio_save_mappings(state);
  config_nio_set_status(state, mapping->readonly ? "Mapping is read-only" : "Mapping is read/write");
}

static void dos_clear_mapping(config_nio_state_t *state)
{
  memset(&state->mappings[dos_selected_unit], 0, sizeof(state->mappings[dos_selected_unit]));
  (void) config_nio_save_mappings(state);
  config_nio_set_status(state, "Mapping cleared");
}

static void dos_clear_slot(config_nio_state_t *state)
{
  if (fnsvc_set_mount(dos_selected_slot, "", "r", 0)) {
    (void) config_nio_refresh_slots(state);
    config_nio_set_status(state, "Slot cleared");
  } else {
    config_nio_set_status(state, "Unable to clear slot");
  }
}

static void dos_edit_slot(config_nio_state_t *state)
{
  strcpy(dos_uri_edit, state->slots[dos_selected_slot].uri);
  strcpy(dos_mode_edit,
         state->slots[dos_selected_slot].mode[0] ? state->slots[dos_selected_slot].mode : "rw");

  if (!dos_curses_prompt(" Edit slot ", "URI", dos_uri_edit, sizeof(dos_uri_edit)) ||
      !dos_uri_edit[0])
    return;
  if (!dos_curses_prompt(" Edit slot ", "Mode ro/rw", dos_mode_edit, sizeof(dos_mode_edit)))
    return;
  if (stricmp(dos_mode_edit, "ro") != 0)
    strcpy(dos_mode_edit, "rw");

  if (fnsvc_set_mount(dos_selected_slot, dos_uri_edit, dos_mode_edit, 1)) {
    (void) config_nio_refresh_slots(state);
    config_nio_set_status(state, "Slot saved");
  } else {
    config_nio_set_status(state, "Unable to save slot");
  }
}

void config_nio_ui_clear(void)
{
  uint8_t i;

  for (i = 0; i < 25; i++)
    fputs("\n", stdout);
}

void config_nio_ui_header(const char *title, const char *hint)
{
  fputs("+--------------------------------------------------------------------------+\n", stdout);
  fputs("| ", stdout);
  dos_print_clip(title ? title : "", 72);
  fputs(" |\n", stdout);
  fputs("+--------------------------------------------------------------------------+\n", stdout);
  if (hint && *hint) {
    fputs(hint, stdout);
    fputs("\n\n", stdout);
  }
}

void config_nio_ui_status(const char *status)
{
  fputs("\n[", stdout);
  fputs(status ? status : "", stdout);
  fputs("]\n", stdout);
}

void config_nio_ui_pause(void)
{
  (void) getch();
}

int config_nio_ui_get_key(void)
{
  return getch();
}

int config_nio_ui_prompt(const char *label, char *buf, uint16_t cap)
{
  char *p;

  if (!buf || cap == 0)
    return 0;
  fputs(label ? label : "?", stdout);
  fputs(": ", stdout);
  fflush(stdout);
  if (!fgets(buf, cap, stdin)) {
    buf[0] = 0;
    return 0;
  }
  p = strchr(buf, '\n');
  if (p)
    *p = 0;
  p = strchr(buf, '\r');
  if (p)
    *p = 0;
  return 1;
}

void config_nio_ui_putc(char c)
{
  putchar(c);
}

void config_nio_ui_print(const char *s)
{
  fputs(s ? s : "", stdout);
}

void config_nio_ui_println(const char *s)
{
  config_nio_ui_print(s);
  putchar('\n');
}

void config_nio_ui_print_uint(unsigned value)
{
  printf("%u", value);
}

void config_nio_ui_print_ulong(unsigned long value)
{
  printf("%lu", value);
}

void config_nio_ui_print_padded(const char *s, uint8_t width)
{
  uint8_t n;

  n = 0;
  while (s && *s && n < width) {
    putchar(*s++);
    n++;
  }
  while (n < width) {
    putchar(' ');
    n++;
  }
}

const char *config_nio_ui_platform_name(void)
{
  return "MS-DOS";
}

uint8_t config_nio_ui_screen_width(void)
{
  return 80;
}

uint8_t config_nio_ui_screen_height(void)
{
  return 25;
}

void config_nio_ui_drive_label(uint8_t unit, char *buf, uint8_t cap)
{
  int drive;

  if (!buf || cap == 0)
    return;
  drive = fnctl_find_drive_for_unit(unit);
  if (drive)
    sprintf(buf, "%c:", 'A' + drive - 1);
  else
    sprintf(buf, "unit%u", (unsigned) unit);
}

int config_nio_ui_show_mappings(config_nio_state_t *state)
{
  (void) state;
  return 0;
}

int config_nio_ui_run(config_nio_state_t *state)
{
  int key;
  int running;

  if (!state)
    return 1;

  if (dos_selected_host >= state->host_count)
    dos_selected_host = 0;
  if (dos_selected_slot >= FNCTL_MAX_UNITS)
    dos_selected_slot = 0;
  if (dos_selected_unit >= FNCTL_MAX_UNITS)
    dos_selected_unit = 0;

  (void) config_nio_refresh_slots(state);
  dos_screen = DOS_SCREEN_DASHBOARD;
  dos_focus = 0;
  dos_browser_loaded = 0;

  dos_curses_start();
  dos_app_create_windows();
  if (!dos_main_win || !dos_side_win || !dos_status_win) {
    dos_curses_stop();
    config_nio_set_status(state, "Unable to create curses windows");
    return 1;
  }

  running = 1;
  while (running) {
    dos_redraw_app(state);
    key = wgetch(stdscr);
    running = dos_handle_key(state, key);
  }

  dos_curses_stop();
  return 1;
}
