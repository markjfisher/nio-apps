#ifndef SERIAL_H
#define SERIAL_H

#include <stdint.h>

enum {
  SERIAL_COM1 = 0x3F8,
  SERIAL_COM2 = 0x2F8,
  SERIAL_COM3 = 0x3E8,
  SERIAL_COM4 = 0x2E8
};

uint16_t serial_com_base(unsigned com_number);
void serial_init(uint16_t base, uint16_t divisor);
void serial_put_byte(uint8_t value);
int serial_get_byte(uint8_t *value, unsigned timeout_ms);
void serial_drain_rx(void);

#endif
