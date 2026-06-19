#include "fnctl.h"
#include "fnsvc.h"

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void usage(void)
{
  puts("Usage: FIN [slot] image-uri-or-path");
}

static fnctl_state_t state;
static char uri[FNSVC_MAX_URI + 1];
static char display[FNSVC_MAX_PATH + 1];

static int has_scheme(const char *s)
{
  while (*s) {
    if (*s == ':')
      return 1;
    if (*s == '/' || *s == '\\')
      return 0;
    s++;
  }
  return 0;
}

int main(int argc, char **argv)
{
  uint8_t slot = 0;
  const char *path;

  if (argc < 2 || argc > 3 || (argc > 1 && argv[1][0] == '?')) {
    usage();
    return 1;
  }

  if (argc == 3) {
    slot = (uint8_t) atoi(argv[1]);
    path = argv[2];
  } else {
    path = argv[1];
  }

  if (slot >= FNCTL_MAX_UNITS) {
    puts("Bad slot");
    return 1;
  }

  if (has_scheme(path)) {
    if (!fnsvc_resolve_path(path, "", uri, sizeof(uri), display, sizeof(display))) {
      puts("Unable to resolve image URI");
      return 2;
    }
  } else {
    if (!fnctl_get_state(&state) || state.current_uri_len == 0) {
      puts("No host set. Use FHOST first or pass a full URI.");
      return 2;
    }
    if (!fnsvc_resolve_path(state.current_uri, path, uri, sizeof(uri), display, sizeof(display))) {
      puts("Unable to resolve image path");
      return 2;
    }
  }

  if (!fnsvc_set_mount(slot, uri, "rw", 1)) {
    puts("Unable to persist mount slot");
    return 2;
  }

  printf("Slot %u: %s\n", (unsigned) slot, uri);
  return 0;
}
