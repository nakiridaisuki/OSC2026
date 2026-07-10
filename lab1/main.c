#define UART_BASE 0xd4017000

void uart_init(void) { *(volatile unsigned int *)(UART_BASE + 0x04) = 0x40; }

void uart_putchar(char c) {
    while ((*(volatile unsigned int *)(UART_BASE + 0x14) & 0x20) == 0) {
    }

    *(volatile unsigned int *)(UART_BASE + 0x00) = (unsigned int)c;
}

char uart_getchar() {
    while ((*(volatile unsigned int *)(UART_BASE + 0x14) & 0x01) == 0) {
    }

    return *(volatile unsigned int *)(UART_BASE + 0x00) & 0xff;
}

int main() {
    uart_init();

    uart_putchar('H');
    uart_putchar('e');
    uart_putchar('l');
    uart_putchar('l');
    uart_putchar('o');
}
