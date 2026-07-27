#include <stdint.h>

void __fastcall__ bbc_oscli(const char *cmd);

static char load_cmd[] = "LOAD XXXXXXX FFFF\r";

static char hex_digit(uint8_t value)
{
  value = (uint8_t) (value & 0x0f);
  return (char) (value < 10 ? ('0' + value) : ('A' + value - 10));
}

void config_nio_bbc_load_template(const char *asset_name)
{
  uint8_t i;
  uint16_t base;

  for (i = 0; i < 7; i++)
    load_cmd[5 + i] = asset_name[i] ? asset_name[i] : ' ';

  base = 0x7c00;
  load_cmd[13] = hex_digit((uint8_t) (base >> 12));
  load_cmd[14] = hex_digit((uint8_t) (base >> 8));
  load_cmd[15] = hex_digit((uint8_t) (base >> 4));
  load_cmd[16] = hex_digit((uint8_t) base);
  bbc_oscli(load_cmd);
}
