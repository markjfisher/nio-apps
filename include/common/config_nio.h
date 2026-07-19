#ifndef CONFIG_NIO_H
#define CONFIG_NIO_H

#include "fnctl.h"
#include "fnsvc.h"

#include <stdint.h>

#define CONFIG_NIO_NS "config-nio"
#define CONFIG_NIO_KEY_HOSTS "hosts"
#define CONFIG_NIO_KEY_MAPPINGS "mappings"
#define CONFIG_NIO_KEY_PREFS "prefs"

#define CONFIG_NIO_PREF_DATE_YMD 0
#define CONFIG_NIO_PREF_DATE_YDM 1
#define CONFIG_NIO_PREF_SIZE_FULL 0
#define CONFIG_NIO_PREF_SIZE_COMPACT 1

#define CONFIG_NIO_COLOR_BODY 0
#define CONFIG_NIO_COLOR_FRAME 1
#define CONFIG_NIO_COLOR_TITLE 2
#define CONFIG_NIO_COLOR_SELECT 3
#define CONFIG_NIO_COLOR_STATUS 4
#define CONFIG_NIO_COLOR_INACTIVE 5
#define CONFIG_NIO_COLOR_MENUBAR 6
#define CONFIG_NIO_COLOR_MENUHOT 7
#define CONFIG_NIO_COLOR_TITLEBAR 8
#define CONFIG_NIO_COLOR_BUTTON 9
#define CONFIG_NIO_COLOR_BUTTON_SELECT 10
#ifdef __CC65__
#define CONFIG_NIO_COLOR_COUNT 1
#else
#define CONFIG_NIO_COLOR_COUNT 11
#endif

#ifdef __CC65__
#define CONFIG_NIO_MAX_HOSTS 4
#define CONFIG_NIO_MAX_ENTRIES 5
#define CONFIG_NIO_URI_MAX 48
#define CONFIG_NIO_NAME_MAX 31
#define CONFIG_NIO_TEXT_MAX 256
#else
#define CONFIG_NIO_MAX_HOSTS 8
#define CONFIG_NIO_MAX_ENTRIES 20
#define CONFIG_NIO_URI_MAX FNSVC_MAX_URI
#define CONFIG_NIO_NAME_MAX 79
#define CONFIG_NIO_TEXT_MAX 1024
#endif

typedef struct {
  uint8_t valid;
  uint8_t slot;
  uint8_t readonly;
} config_nio_mapping_t;

typedef struct {
  uint8_t enabled;
  char uri[CONFIG_NIO_URI_MAX + 1];
  char mode[4];
} config_nio_slot_t;

typedef struct {
  uint8_t is_dir;
  char name[CONFIG_NIO_NAME_MAX + 1];
  uint32_t size;
  uint32_t mtime;
} config_nio_entry_t;

typedef struct {
  uint8_t date_format;
  uint8_t size_format;
  uint8_t color_fg[CONFIG_NIO_COLOR_COUNT];
  uint8_t color_bg[CONFIG_NIO_COLOR_COUNT];
} config_nio_prefs_t;

typedef struct {
  uint8_t host_count;
  char hosts[CONFIG_NIO_MAX_HOSTS][CONFIG_NIO_URI_MAX + 1];
  config_nio_slot_t slots[FNCTL_MAX_UNITS];
  config_nio_mapping_t mappings[FNCTL_MAX_UNITS];
  config_nio_prefs_t prefs;
  config_nio_entry_t entries[CONFIG_NIO_MAX_ENTRIES];
  uint8_t entry_count;
  uint16_t entry_total;
  uint8_t entries_truncated;
  char browse_path[FNSVC_MAX_PATH + 1];
  char status[96];
} config_nio_state_t;

int config_nio_load(config_nio_state_t *state);
int config_nio_save_hosts(const config_nio_state_t *state);
int config_nio_save_mappings(const config_nio_state_t *state);
int config_nio_save_prefs(const config_nio_state_t *state);
int config_nio_refresh_slots(config_nio_state_t *state);
int config_nio_set_status(config_nio_state_t *state, const char *msg);
int config_nio_browse(config_nio_state_t *state, uint8_t host);
int config_nio_compose_uri(const char *host, const char *path,
                           const char *leaf, char *out, uint16_t cap);
int config_nio_mount_mappings(config_nio_state_t *state);
void config_nio_run(config_nio_state_t *state);
int config_nio_ui_run(config_nio_state_t *state);

void config_nio_ui_clear(void);
void config_nio_ui_header(const char *title, const char *hint);
void config_nio_ui_status(const char *status);
void config_nio_ui_pause(void);
int config_nio_ui_get_key(void);
int config_nio_ui_prompt(const char *label, char *buf, uint16_t cap);
void config_nio_ui_putc(char c);
void config_nio_ui_print(const char *s);
void config_nio_ui_println(const char *s);
void config_nio_ui_print_uint(unsigned value);
void config_nio_ui_print_ulong(unsigned long value);
void config_nio_ui_print_padded(const char *s, uint8_t width);
const char *config_nio_ui_platform_name(void);
uint8_t config_nio_ui_screen_width(void);
uint8_t config_nio_ui_screen_height(void);
void config_nio_ui_drive_label(uint8_t unit, char *buf, uint8_t cap);
int config_nio_ui_show_mappings(config_nio_state_t *state);

#endif
