/*
 * Copyright: Olivier Gruber (olivier dot gruber at acm dot org)
 *
 * This program is free software: you can redistribute it and/or modify it under the terms
 * of the GNU General Public License as published by the Free Software Foundation,
 * either version 3 of the License, or (at your option) any later version.
 * This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY;
 * without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along with this program.
 * If not, see <https://www.gnu.org/licenses/>.
 */

#include "main.h"
#include "isr.h"
#include "uart.h"
#include "isr-mmio.h"

/*
 * Assembly functions:
 */
extern void _irqs_setup(void);
extern void _irqs_enable(void);
extern void _irqs_disable(void);
extern void _wfi(void);

/*
 * Data structure for handlers and cookies
 */
struct handler {
  void (*callback)(uint32_t, void*);
  void *cookie;
};

volatile uint32_t *vicIntEnable = (volatile uint32_t *)(VIC_BASE_ADDR + VICINTENABLE);
volatile uint32_t *vicIntClear  = (volatile uint32_t *)(VIC_BASE_ADDR + VICINTCLEAR);
volatile uint32_t *vicIntSelect = (volatile uint32_t *)(VIC_BASE_ADDR + VICINTSELECT);
volatile uint32_t *vicIrqStatus = (volatile uint32_t *)(VIC_BASE_ADDR + VICIRQSTATUS);

struct handler handlers[NIRQS];

/*
 * Interrupt Service Routine, up-called from assembly.
 * Needs to interogate the VIC about the masked interrupt
 * status and call the corresponding handlers.
 */
void isr() {
  uint32_t irq_status = *vicIrqStatus;
  // for(uint32_t irq = 0; irq < NIRQS; irq ++){
  //   if(irq_status & (1 << irq)){
  //     if(handlers[irq].callback != NULL)
  //       handlers[irq].callback(irq, handlers[irq].cookie);
  //   }
  //   *vicIntClear = (1 << irq);
  // }
  if(irq_status & (1 << UART0_IRQ)) {
    char c;
    uart_receive(UART0, &c);
    uart_send(UART0, c);

    *vicIntClear = (1 << UART0_IRQ);
  }
}

void core_enable_irqs() {
  _irqs_enable();
}

void core_disable_irqs() {
  _irqs_disable();
}

void core_halt() {
  _wfi();
}

/*
 * Initial setup our interrupt support,
 * need to initialize both the hardware and software
 * sides.
 */
void vic_setup_irqs() {
  // TODO
  *vicIntEnable = 0x00000000;
  *vicIntClear = 0xFFFFFFFF;
  *vicIntSelect = 0x00000000;
  for (int i = 0; i < NIRQS; i++) {
    handlers[i].callback = NULL;
    handlers[i].cookie = NULL;
  }
}

/*
 * Enables the given interrupt at the VIC level.
 */
void vic_enable_irq(uint32_t irq, void (*callback)(uint32_t, void*), void *cookie) {
  // TODO
  handlers[irq].callback = callback;
  handlers[irq].cookie = cookie;
  *vicIntEnable |= (1 << irq);
}

/*
 * Disables the given interrupt at the VIC level.
 */
void vic_disable_irq(uint32_t irq) {
  // TODO
  *vicIntEnable &= (1 << irq);
}
