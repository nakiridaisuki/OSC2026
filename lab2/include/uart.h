#ifndef _UART_H_
#define _UART_H_

#include "types.h"
#include <stdbool.h>
#include <stdint.h>

// UART base info
extern uint64_t UART_CLK;
extern uint64_t UART_BASE;
extern uint32_t UART_REG_SHIFT;

// Standard 16550 UART register logic index
#define UART_RBR 0 // Receive Buffer
#define UART_THR 0 // Transmit Holding
#define UART_IER 1 // Interrupt Enable
#define UART_FCR 2 // FIFO Control
#define UART_LCR 3 // Line Control
#define UART_LSR 5 // Line Status
#define UART_DLL 0 // Divisor Latch Low Byte
#define UART_DLH 1 // Divisor Latch High Byte

// Register bits
#define LSR_TDRQ 0x20 // Transmit Data Request
#define LSR_DR   0x01 // Data Ready

// Utils functions
#define write_reg(offset, val) \
    (*(volatile uint8_t *)(UART_BASE + (offset << UART_REG_SHIFT)) = (uint8_t)val)
#define read_reg(offset) (*(volatile uint8_t *)(UART_BASE + (offset << UART_REG_SHIFT)))

// Macro for printf
#define _putchar uart_putchar

typedef struct {
    phys_addr_t base_addr;
    uint32_t clock;
    uint32_t reg_shift;
    uint32_t baudrate;
    uint8_t parity;
    uint8_t bits;
    uint8_t enable_flow_ctrl;
    uint8_t enable_fifo;
} UARTInit;

enum {
    PARITY_NO   = 0,
    PARITY_ODD  = 1,
    PARITY_EVEN = 2,
};

void uart_init(UARTInit init_data);
void uart_putchar(char c);
char uart_getchar();
int uart_getuint32();

#endif // !_UART_H_
