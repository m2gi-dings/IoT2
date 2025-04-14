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
#include "uart.h"
#include "uart-mmio.h"
#include <stdbool.h>

struct uart {
  uint8_t uartno; // the UART numéro
  void* bar;      // base address register for this UART
};

struct cookie {
  uint32_t uartno;
  char line[MAX_CHARS];
  uint32_t head;
  uint32_t tail;
  bool_t processing;
};


static
struct uart uarts[NUARTS];
volatile uint32_t uart_fr = UART_FR;
volatile uint32_t uart_dr = UART_DR;
volatile uint32_t uart_fr_rxfe = UART_FR_RXFE;
volatile uint32_t uart_fr_txff = UART_FR_TXFF;

static
void uart_init(uint32_t uartno, void* bar) {
  struct uart*uart = &uarts[uartno];
  uart->uartno = uartno;
  uart->bar = bar;
  // no hardware initialization necessary
  // when running on QEMU, the UARTs are
  // already initialized, as long as we
  // do not rely on interrupts.
}

void uarts_init() {
  uart_init(UART0,UART0_BASE_ADDRESS);
  uart_init(UART1,UART1_BASE_ADDRESS);
  uart_init(UART2,UART2_BASE_ADDRESS);
}

void uart_enable(uint32_t uartno) {
  struct uart*uart = &uarts[uartno];
  // nothing to do here, as long as
  // we do not rely on interrupts
}

void uart_disable(uint32_t uartno) {
  struct uart*uart = &uarts[uartno];
  // nothing to do here, as long as
  // we do not rely on interrupts
}

bool uart_receive(uint8_t uartno, char *pt) {
  struct uart*uart = &uarts[uartno];
  // while (*(volatile uint32_t*)(uart->bar + uart_fr) & uart_fr_rxfe){
  //   for (volatile int i = 0; i < 1000; i++){}
  // }
  if(*(volatile uint32_t*)(uart->bar + uart_fr) & uart_fr_rxfe){
    return false;
  }
  *pt = *(volatile uint32_t*)(uart->bar + uart_dr);
  return true;
  // volatile uint32_t* uart_sr = (uint32_t*)(uart->bar + UART_SR);
  // volatile uint32_t* uart_fifo = (uint32_t*)(uart->bar + UART_FIFO);
  // while(*uart_sr & UART_SR_REMPTY);
  // *pt = *uart_fifo;
  // TODO: not implemented yet...
  // panic();
}

/**
 * Sends a character through the given uart, this is a blocking call
 * until the character has been sent.
 */
bool uart_send(uint8_t uartno, char s) {
  struct uart* uart = &uarts[uartno];
  // while (*(volatile uint32_t*)(uart->bar + uart_fr) & uart_fr_txff){
  //   for (volatile int i = 0; i < 1000; i++){}
  // }
  if(*(volatile uint32_t*)(uart->bar + uart_fr) & uart_fr_txff){
    return false;
  }
  *(volatile uint32_t*)(uart->bar + uart_dr) = s;
  return true;
  // volatile uint32_t* uart_sr = (uint32_t*)(uart->bar + UART_SR);
  // volatile uint32_t* uart_fifo = (uint32_t*)(uart->bar + UART_FIFO);
  // while (*uart_sr & UART_SR_TFUL);
  // *uart_fifo = s;

  // TODO: not implemented yet...
  // panic();
}

/**
 * This is a wrapper function, provided for simplicity,
 * it sends a C string through the given uart.
 */
void uart_send_string(uint8_t uartno, const char *s) {
  while (*s != '\0') {
    uart_send(uartno, *s);
    s++;
  }
}

void write_amap(struct cookie *cookie) {
  while (cookie->tail < cookie->head) {
    uint8_t code = cookie->line[cookie->tail];
    if (!uart_send(cookie->uartno,code)) 
      return;
    cookie->tail++;
    if (code == '\n') {
      // shell(cookie->line,cookie->head);
      cookie->tail= cookie->head = 0;
      cookie->processing=false;
    }
  }
}

void read_listener(void *addr) {
  struct cookie *cookie = addr;
  uint8_t code; 

  while(!cookie->processing && uart_receive(cookie->uartno,&code)) {
    cookie->line[cookie->head++]=(char)code;
    cookie->processing = (code == '\n');
    write_amap(cookie);
  }
  bool_t dropped=false;
  while (cookie->processing && uart_receive(cookie->uartno,&code))
    dropped=true;
  // if(dropped)
  //   beep(); --Anything, but just no silent failures
}

void write_listener(void *addr) {
  struct cookie *cookie = addr;
  write_amap(cookie);
}

