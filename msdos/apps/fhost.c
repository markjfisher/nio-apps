#include "fnctl.h"
#include "fnsvc.h"

#include <stdio.h>
#include <string.h>

#define FHOST_BUILD_ID "nio-state-v2"

static fnctl_state_t state;
static char uri[FNSVC_MAX_URI + 1];
static char path[FNSVC_MAX_PATH + 1];

static void usage(void)
{
  puts("FHOST " FHOST_BUILD_ID);
  puts("Usage: FHOST [uri]");
}

int main(int argc, char **argv)
{
  if (argc > 2 || (argc > 1 && argv[1][0] == '?')) {
    usage();
    return 1;
  }

  if (argc == 1) {
    if (!fnctl_get_state(&state)) {
      puts("FUJINET.SYS not loaded or not responding");
      return 2;
    }
    if (state.current_uri_len == 0) {
      puts("HOST: (none)");
      puts("PATH: (none)");
    } else {
      printf("HOST: %s\n", state.current_uri);
      printf("PATH: %s\n", state.display_path_len ? state.display_path : "/");
    }
    return 0;
  }

  if (!fnsvc_resolve_path(argv[1], "", uri, sizeof(uri), path, sizeof(path))) {
    puts("Unable to resolve URI");
    return 2;
  }

  if (!fnctl_set_state(uri, path)) {
    printf("Unable to store current host in FUJINET.SYS (DOS error %u, %s)\n",
           fnctl_last_dos_error(), FHOST_BUILD_ID);
    return 2;
  }

  if (!fnctl_get_state(&state) || strcmp(state.current_uri, uri) != 0) {
    puts("Stored host could not be verified in FUJINET.SYS (" FHOST_BUILD_ID ")");
    return 2;
  }

  printf("HOST: %s\n", uri);
  printf("PATH: %s\n", path);
  return 0;
}
