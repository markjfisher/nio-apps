#include "fnctl.h"
#include "fnsvc.h"

#include <stdio.h>
#include <string.h>

typedef struct {
  unsigned count;
} list_ctx_t;

static fnctl_state_t state_buf;
static char uri_buf[FNSVC_MAX_URI + 1];
static char path_buf[FNSVC_MAX_PATH + 1];

static void usage(void)
{
  puts("Usage: FLS [path]");
}

static void print_entry(uint8_t is_dir, const char *name, uint32_t size, void *ctx)
{
  list_ctx_t *lc = (list_ctx_t *) ctx;
  if (is_dir)
    printf("<DIR>      %s\n", name);
  else
    printf("%10lu %s\n", size, name);
  lc->count++;
}

static int current_or_resolved(const char *arg, char *uri, uint16_t uri_cap)
{
  if (!fnctl_get_state(&state_buf) || state_buf.current_uri_len == 0) {
    puts("No host set. Use FHOST first.");
    return 0;
  }

  if (!arg || !*arg) {
    if (strlen(state_buf.current_uri) >= uri_cap)
      return 0;
    strcpy(uri, state_buf.current_uri);
    return 1;
  }

  return fnsvc_resolve_path(state_buf.current_uri, arg, uri, uri_cap,
                            path_buf, sizeof(path_buf));
}

int main(int argc, char **argv)
{
  list_ctx_t ctx;

  if (argc > 2 || (argc > 1 && argv[1][0] == '?')) {
    usage();
    return 1;
  }

  if (!current_or_resolved(argc > 1 ? argv[1] : "", uri_buf, sizeof(uri_buf))) {
    puts("Unable to resolve path");
    return 2;
  }

  ctx.count = 0;
  if (!fnsvc_list_directory(uri_buf, print_entry, &ctx)) {
    puts("Unable to list directory");
    return 2;
  }

  printf("%u entr%s\n", ctx.count, ctx.count == 1 ? "y" : "ies");
  return 0;
}
