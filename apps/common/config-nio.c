#include "config_nio.h"
#include "fujinet-nio.h"

static config_nio_state_t state;

int main(void)
{
  uint8_t result;

  result = fn_init();
  if (result != FN_OK) {
    config_nio_ui_println("FujiNet init failed");
    config_nio_ui_pause();
    return 2;
  }

  if (!fn_is_ready()) {
    config_nio_ui_println("FujiNet is not ready");
    config_nio_ui_pause();
    return 2;
  }

  if (!config_nio_load(&state)) {
    config_nio_ui_println("Unable to load config-nio state");
    config_nio_ui_pause();
    return 2;
  }

  config_nio_run(&state);
  return 0;
}
