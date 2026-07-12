#include "string.h"

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

#define LSR_TDRQ 0x20 // Transmit Data Request
#define LSR_DR   0x01 // Data Ready

#define write_reg(offset, val) (*(volatile unsigned int *)(UART_BASE + offset) = val)
#define read_reg(offset)       (*(volatile unsigned int *)(UART_BASE + offset))

void uart_init(unsigned int baudrate) {
    unsigned int divisor = (UART_CLK + (baudrate * 8)) / (baudrate * 16);
    unsigned char dll    = divisor & 0xff;
    unsigned char dlh    = (divisor >> 8) & 0xff;

    // Disable all interrupt and enable UART unit
    write_reg(UART_IER, 0x40); // UUE at bit 6

    // Disable FIFO, clean FIFO first
    write_reg(UART_FCR, 0x06); // reset transmit/receive FIFO at bit 1, 2
    write_reg(UART_FCR, 0x00); // disable FIFO

    // Setting Baud Rate
    write_reg(UART_LCR, read_reg(UART_LCR) | 0x80);
    write_reg(UART_DLL, dll);
    write_reg(UART_DLH, dlh);
    write_reg(UART_LCR, 0x03);
}

void uart_putchar(char c) {
    while ((read_reg(UART_LSR) & LSR_TDRQ) == 0) {
    }

    write_reg(UART_THR, (unsigned int)c);
}

char uart_getchar() {
    while ((read_reg(UART_LSR) & LSR_DR) == 0) {
    }

    return read_reg(UART_RBR) & 0xff;
}

void uart_puts(const char *str) {
    while (*str) {
        if (*str == '\n')
            uart_putchar('\r');
        uart_putchar(*str);
        str++;
    }
}

int main() {
    uart_init(115200);

    char buf[256];

    int idx = 0;
    while (1) {
        uart_puts("opi-rv2> ");
        memset(buf, 0, sizeof(buf));
        idx = 0;

        while (1) {
            char c = uart_getchar();

            if (c == '\r' || c == '\n') {
                buf[idx] = 0;
                uart_puts("\n");
                break;
            } else if (c == 127 || c == '\b') {
                if (idx > 0) {
                    idx--;
                    uart_puts("\b \b");
                }
            } else if (idx < 255) {
                buf[idx++] = c;
                uart_putchar(c);
            }
        }

        if (strcmp(buf, "hello") == 0)
            uart_puts("Hello World.\n");
        else if (strcmp(buf, "help") == 0) {
            uart_puts("Avaliable commands:\n");
            uart_puts("  help - show this help.\n");
            uart_puts("  hello - print Hello World.\n");
            uart_puts("  info - print system info.\n");
        } else if (strlen(buf) > 0) {
            uart_puts("Unknow command: ");
            uart_puts(buf);
            uart_puts("\n");
        }
    }
}
