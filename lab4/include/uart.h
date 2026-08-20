#ifndef _UART_H_
#define _UART_H_

#include "types.h"
#include <stdbool.h>
#include <stdint.h>

extern bool UART_INIT_DONE;

// UART base info
extern uint64_t UART_CLK;
extern uint64_t UART_BASE;
extern uint32_t UART_REG_SHIFT;
extern uint32_t UART_IRQ;

// Standard 16550 UART register logic index
#define UART_RBR 0 // Receive Buffer
#define UART_THR 0 // Transmit Holding
#define UART_IER 1 // Interrupt Enable
#define UART_IIR 2 // Interrupt Identification
#define UART_FCR 2 // FIFO Control
#define UART_LCR 3 // Line Control
#define UART_MCR 4 // Modem Control
#define UART_LSR 5 // Line Status
#define UART_DLL 0 // Divisor Latch Low Byte
#define UART_DLH 1 // Divisor Latch High Byte

// Register bits
#define LSR_TDRQ 0x20 // Transmit Data Request
#define LSR_DR   0x01 // Data Ready

// Utils functions
#define write_reg(offset, val) \
    (*(volatile uint8_t *)(UART_BASE + (offset << UART_REG_SHIFT)) = (uint8_t)(val))
#define read_reg(offset)        (*(volatile uint8_t *)(UART_BASE + (offset << UART_REG_SHIFT)))
#define set_reg(offset, mask)   write_reg(offset, read_reg(offset) | (mask))
#define clear_reg(offset, mask) write_reg(offset, read_reg(offset) & ~(mask))

void uart_init_from_fdt(const uint8_t *fdt_ptr);
void uart_putchar(char c);
char uart_getchar();
int uart_getuint32();

void uart_intr_handle();

#endif // !_UART_H_
