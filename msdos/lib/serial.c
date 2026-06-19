#include "serial.h"

#include <conio.h>
#include <dos.h>

enum {
  UART_RBR = 0,
  UART_THR = 0,
  UART_IER = 1,
  UART_FCR = 2,
  UART_LCR = 3,
  UART_MCR = 4,
  UART_LSR = 5,

  LSR_DR = 0x01,
  LSR_THRE = 0x20,

  LCR_DLAB = 0x80,
  LCR_8N1 = 0x03,

  MCR_DTR = 0x01,
  MCR_RTS = 0x02,
  MCR_OUT2 = 0x08
};

static uint16_t uart_base = SERIAL_COM1;

static unsigned long bios_ticks(void)
{
  volatile unsigned long far *ticks = (volatile unsigned long far *) MK_FP(0x40, 0x6C);
  return *ticks;
}

static unsigned long timeout_to_ticks(unsigned timeout_ms)
{
  unsigned long ticks = ((unsigned long) timeout_ms * 182UL) / 10000UL;
  return ticks ? ticks : 1UL;
}

uint16_t serial_com_base(unsigned com_number)
{
  switch (com_number) {
  case 1:
    return SERIAL_COM1;
  case 2:
    return SERIAL_COM2;
  case 3:
    return SERIAL_COM3;
  case 4:
    return SERIAL_COM4;
  default:
    return 0;
  }
}

void serial_init(uint16_t base, uint16_t divisor)
{
  uart_base = base;

  outp(uart_base + UART_IER, 0x00);
  outp(uart_base + UART_LCR, LCR_DLAB);
  outp(uart_base + 0, divisor & 0xFF);
  outp(uart_base + 1, (divisor >> 8) & 0xFF);
  outp(uart_base + UART_LCR, LCR_8N1);
  outp(uart_base + UART_FCR, 0x07);
  outp(uart_base + UART_MCR, MCR_DTR | MCR_RTS | MCR_OUT2);
}

void serial_put_byte(uint8_t value)
{
  while ((inp(uart_base + UART_LSR) & LSR_THRE) == 0)
    ;
  outp(uart_base + UART_THR, value);
}

int serial_get_byte(uint8_t *value, unsigned timeout_ms)
{
  unsigned long start = bios_ticks();
  unsigned long limit = timeout_to_ticks(timeout_ms);

  while ((bios_ticks() - start) <= limit) {
    if (inp(uart_base + UART_LSR) & LSR_DR) {
      *value = (uint8_t) inp(uart_base + UART_RBR);
      return 1;
    }
  }

  return 0;
}

void serial_drain_rx(void)
{
  uint8_t ignored;

  while (serial_get_byte(&ignored, 1))
    ;
}
