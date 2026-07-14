/*
 * IRQMON - foreground MS-DOS interrupt monitor.
 *
 * Hooks hardware IRQ vectors while running, counts interrupt activity, and
 * displays vector ownership/PIC masks.  Interrupt handlers only update RAM
 * counters and immediately chain to the original handler.
 */

#include <conio.h>
#include <dos.h>
#include <i86.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#define IRQ_COUNT 16
typedef void (__interrupt __far *isr_t)(void);

static const unsigned char irq_vectors[IRQ_COUNT] = {
  0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f,
  0x70, 0x71, 0x72, 0x73, 0x74, 0x75, 0x76, 0x77
};

static const char *irq_names[IRQ_COUNT] = {
  "Timer", "Keyboard", "Cascade", "COM2", "COM1", "LPT2/Sound",
  "Floppy", "LPT1", "RTC", "IRQ9", "IRQ10", "PCI/IRQ11",
  "Mouse", "FPU", "IDE1", "IDE2"
};

static volatile unsigned long irq_counts[IRQ_COUNT];
static unsigned long last_counts[IRQ_COUNT];
static unsigned long rates[IRQ_COUNT];
static isr_t old_handlers[IRQ_COUNT];
static int installed;

static void screen_goto(unsigned col, unsigned row)
{
  union REGS regs;

  regs.h.ah = 0x02;
  regs.h.bh = 0;
  regs.h.dh = (unsigned char) (row - 1);
  regs.h.dl = (unsigned char) (col - 1);
  int86(0x10, &regs, &regs);
}

static void screen_clear(void)
{
  union REGS regs;

  regs.h.ah = 0x06;
  regs.h.al = 0;
  regs.h.bh = 0x07;
  regs.h.ch = 0;
  regs.h.cl = 0;
  regs.h.dh = 24;
  regs.h.dl = 79;
  int86(0x10, &regs, &regs);
  screen_goto(1, 1);
}

static void screen_putc(int ch)
{
  union REGS regs;

  regs.h.ah = 0x0e;
  regs.h.al = (unsigned char) ch;
  regs.h.bh = 0;
  regs.h.bl = 0x07;
  int86(0x10, &regs, &regs);
}

static void screen_write(unsigned col, unsigned row, unsigned width,
                         const char *text)
{
  unsigned i;
  int ended = 0;

  screen_goto(col, row);
  for (i = 0; i < width; i++) {
    if (!ended && text && text[i])
      screen_putc(text[i]);
    else {
      ended = 1;
      screen_putc(' ');
    }
  }
}

static void screen_printf(unsigned col, unsigned row, unsigned width,
                          const char *fmt, ...)
{
  char buf[96];
  va_list ap;

  va_start(ap, fmt);
  vsprintf(buf, fmt, ap);
  va_end(ap);
  screen_write(col, row, width, buf);
}

static unsigned long bios_ticks(void)
{
  unsigned long far *ticks = (unsigned long far *) MK_FP(0x40, 0x6c);
  return *ticks;
}

static void chain_irq(unsigned irq)
{
  irq_counts[irq]++;
  old_handlers[irq]();
}

void __interrupt __far irq0_handler(void) { chain_irq(0); }
void __interrupt __far irq1_handler(void) { chain_irq(1); }
void __interrupt __far irq2_handler(void) { chain_irq(2); }
void __interrupt __far irq3_handler(void) { chain_irq(3); }
void __interrupt __far irq4_handler(void) { chain_irq(4); }
void __interrupt __far irq5_handler(void) { chain_irq(5); }
void __interrupt __far irq6_handler(void) { chain_irq(6); }
void __interrupt __far irq7_handler(void) { chain_irq(7); }
void __interrupt __far irq8_handler(void) { chain_irq(8); }
void __interrupt __far irq9_handler(void) { chain_irq(9); }
void __interrupt __far irq10_handler(void) { chain_irq(10); }
void __interrupt __far irq11_handler(void) { chain_irq(11); }
void __interrupt __far irq12_handler(void) { chain_irq(12); }
void __interrupt __far irq13_handler(void) { chain_irq(13); }
void __interrupt __far irq14_handler(void) { chain_irq(14); }
void __interrupt __far irq15_handler(void) { chain_irq(15); }

static isr_t new_handlers[IRQ_COUNT] = {
  irq0_handler, irq1_handler, irq2_handler, irq3_handler,
  irq4_handler, irq5_handler, irq6_handler, irq7_handler,
  irq8_handler, irq9_handler, irq10_handler, irq11_handler,
  irq12_handler, irq13_handler, irq14_handler, irq15_handler
};

static void install_handlers(void)
{
  unsigned i;

  if (installed)
    return;

  _disable();
  for (i = 0; i < IRQ_COUNT; i++) {
    old_handlers[i] = _dos_getvect(irq_vectors[i]);
    _dos_setvect(irq_vectors[i], new_handlers[i]);
  }
  _enable();
  installed = 1;
}

static void uninstall_handlers(void)
{
  unsigned i;

  if (!installed)
    return;

  _disable();
  for (i = 0; i < IRQ_COUNT; i++) {
    if (old_handlers[i])
      _dos_setvect(irq_vectors[i], old_handlers[i]);
  }
  _enable();
  installed = 0;
}

static unsigned inp8(unsigned port)
{
  return inp(port) & 0xff;
}

static void draw_static(void)
{
  screen_clear();
  screen_write(1, 1, 79, "IRQMON - MS-DOS interrupt monitor");
  screen_write(1, 2, 79, "Q/Esc quit  C clear counters");
  screen_write(1, 4, 79, "IRQ Vec  Handler   Name          Total      /sample");
  screen_write(1, 5, 79, "--- ---- --------- ------------- ---------- ----------");
  screen_write(1, 23, 79, "Watch IRQ0(timer), IRQ4(COM1), IRQ8(RTC), IRQ11(PCI), IRQ14(IDE).");
}

static void draw_vectors(void)
{
  unsigned i;

  for (i = 0; i < IRQ_COUNT; i++) {
    isr_t old = old_handlers[i] ? old_handlers[i] : _dos_getvect(irq_vectors[i]);
    screen_printf(1, 6 + i, 79,
                  "%2u  %02Xh  %04X:%04X %-13s %10lu %10lu",
                  i, irq_vectors[i], FP_SEG(old), FP_OFF(old),
                  irq_names[i], irq_counts[i], rates[i]);
  }
}

static void draw_status(void)
{
  screen_printf(1, 21, 79,
                "BIOS ticks=%lu  PIC mask master=%02X slave=%02X",
                bios_ticks(), inp8(0x21), inp8(0xa1));
  screen_printf(1, 22, 79,
                "sample IRQ0=%lu IRQ3=%lu IRQ4=%lu IRQ8=%lu IRQ11=%lu IRQ12=%lu IRQ14=%lu",
                rates[0], rates[3], rates[4], rates[8],
                rates[11], rates[12], rates[14]);
}

static void sample_counts(void)
{
  unsigned i;
  unsigned long current;

  for (i = 0; i < IRQ_COUNT; i++) {
    current = irq_counts[i];
    rates[i] = current - last_counts[i];
    last_counts[i] = current;
  }

}

static void clear_counters(void)
{
  unsigned i;

  _disable();
  for (i = 0; i < IRQ_COUNT; i++) {
    irq_counts[i] = 0;
    last_counts[i] = 0;
    rates[i] = 0;
  }
  _enable();
}

int main(void)
{
  unsigned long next_tick;
  int ch;
  int done = 0;

  install_handlers();
  draw_static();
  next_tick = bios_ticks();

  while (!done) {
    if ((long) (bios_ticks() - next_tick) >= 0) {
      sample_counts();
      draw_vectors();
      draw_status();
      next_tick = bios_ticks() + 18;
    }

    if (kbhit()) {
      ch = getch();
      if (ch == 'q' || ch == 'Q' || ch == 27)
        done = 1;
      else if (ch == 'c' || ch == 'C')
        clear_counters();
    }
  }

  uninstall_handlers();
  screen_clear();
  return 0;
}
