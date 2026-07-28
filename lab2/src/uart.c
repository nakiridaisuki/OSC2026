#include "uart.h"

uint64_t UART_CLK       = 14750000;
uint64_t UART_BASE      = 0x10000000;
uint32_t UART_REG_SHIFT = 0;

void uart_init(UARTInit init_data) {
    /*
     * TODOs
     * 1. bits setting
     * 2. flow control setting
     * 3. parity setting
     */
    UART_CLK       = init_data.clock;
    UART_BASE      = init_data.base_addr;
    UART_REG_SHIFT = init_data.reg_shift;

    unsigned int divisor = (UART_CLK + (init_data.baudrate * 8)) / (init_data.baudrate * 16);
    unsigned char dll    = divisor & 0xff;
    unsigned char dlh    = (divisor >> 8) & 0xff;

    // Disable all interrupt and enable UART unit
    write_reg(UART_IER, 0x40); // UUE at bit 6

    if (init_data.enable_fifo) {
        write_reg(UART_FCR, 0x07);
    } else {
        write_reg(UART_FCR, 0x06); // reset transmit/receive FIFO at bit 1, 2
        write_reg(UART_FCR, 0x00); // disable FIFO
    }

    // Setting Baud Rate
    write_reg(UART_LCR, read_reg(UART_LCR) | 0x80);
    write_reg(UART_DLL, dll);
    write_reg(UART_DLH, dlh);
    // Setting 8N1 data format
    write_reg(UART_LCR, 0x03);
}

void uart_putchar(char c) {
    while ((read_reg(UART_LSR) & LSR_TDRQ) == 0) {
    }

    if (c == '\n') {
        write_reg(UART_THR, (unsigned int)'\r');
        while ((read_reg(UART_LSR) & LSR_TDRQ) == 0) {
        }
    }

    write_reg(UART_THR, (unsigned int)c);
}

char uart_getchar() {
    while ((read_reg(UART_LSR) & LSR_DR) == 0) {
    }

    return read_reg(UART_RBR) & 0xff;
}

int uart_getuint32() {
    int result = 0;
    for (int i = 0; i < 4; i++) {
        char tmp = uart_getchar();
        result |= (tmp & 0xff) << (8 * i);
    }
    return result;
}
