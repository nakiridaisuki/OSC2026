#ifndef _UART_H_
#define _UART_H_

#include <stdbool.h>

// UART base info
#define UART_CLK  14750000
#define UART_BASE 0xd4017000

// Register offsets
#define UART_RBR 0x00 // Receive Buffer
#define UART_THR 0x00 // Transmit Holding
#define UART_IER 0x04 // Interrupt Enable
#define UART_LSR 0x14 // Line Status
#define UART_FCR 0x08 // FIFO Control
#define UART_LCR 0x0c // Line Control
#define UART_DLL 0x00 // Divisor Latch Low Byte
#define UART_DLH 0x04 // Divisor Latch High Byte

// Register bits
#define LSR_TDRQ 0x20 // Transmit Data Request
#define LSR_DR   0x01 // Data Ready

// Utils functions
#define write_reg(offset, val) (*(volatile unsigned int *)(UART_BASE + offset) = val)
#define read_reg(offset)       (*(volatile unsigned int *)(UART_BASE + offset))

// Macro for printf
#define _putchar uart_putchar

void uart_init(unsigned int baudrate, bool enable_fifo);
void uart_putchar(char c);
char uart_getchar();
int uart_getuint32();

#endif // !_UART_H_
