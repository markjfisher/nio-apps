#include "fnctl.h"
#include "fnsvc.h"

#include <stdio.h>
#include <string.h>

#define FHOST_BUILD_ID "nio-state-v2"

static fnctl_state_t state;
static char uri[FNSVC_MAX_URI + 1];
static char path[FNSVC_MAX_PATH + 1];
#ifdef __ATARI__
static char input_uri[FNSVC_MAX_URI + 1];
#endif

static void usage(void)
{
  puts("FHOST " FHOST_BUILD_ID);
  puts("Usage: FHOST [uri]");
}

#ifdef __ATARI__
static const char *prompt_uri(void)
{
  char *nl;

  printf("URI (blank=keep): ");
  fflush(stdout);
  if (!fgets(input_uri, sizeof(input_uri), stdin))
    input_uri[0] = 0;

  nl = strchr(input_uri, '\n');
  if (nl)
    *nl = 0;
  nl = strchr(input_uri, '\r');
  if (nl)
    *nl = 0;

  return input_uri;
}
#endif

int main(int argc, char **argv)
{
  const char *target_uri;

  if (argc > 2 || (argc > 1 && argv[1][0] == '?')) {
    usage();
    return 1;
  }

  if (argc == 1) {
    if (!fnctl_get_state(&state)) {
      puts("FujiNet state service is not responding");
      return 2;
    }
    if (state.current_uri_len == 0) {
      puts("HOST: (none)");
      puts("PATH: (none)");
    } else {
      printf("HOST: %s\n", state.current_uri);
      printf("PATH: %s\n", state.display_path_len ? state.display_path : "/");
    }
#ifdef __ATARI__
    target_uri = prompt_uri();
    if (!target_uri || !*target_uri)
      return 0;
#else
    return 0;
#endif
  } else {
    target_uri = argv[1];
  }

  if (!fnsvc_resolve_path(target_uri, "", uri, sizeof(uri), path, sizeof(path))) {
    puts("Unable to resolve URI");
    return 2;
  }

  if (!fnctl_set_state(uri, path)) {
    printf("Unable to store current host (error %u, %s)\n",
           fnctl_last_dos_error(), FHOST_BUILD_ID);
    return 2;
  }

  if (!fnctl_get_state(&state) || strcmp(state.current_uri, uri) != 0) {
    puts("Stored host could not be verified (" FHOST_BUILD_ID ")");
    return 2;
  }

  printf("HOST: %s\n", uri);
  printf("PATH: %s\n", path);
  return 0;
}
