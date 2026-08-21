#include "uart.h"
#include "dstruc.h"
#include "dtb.h"
#include "plic.h"
#include "string.h"
#include <stdbool.h>
#include <stdint.h>

phys_addr_t UART_BASE   = 0x10000000;
uint64_t UART_CLK       = 14750000;
uint32_t UART_REG_SHIFT = 0;
uint32_t UART_IRQ       = 10;

bool UART_INIT_DONE = false;

typedef struct {
    uint32_t baudrate;
    uint8_t parity;
    uint8_t bits;
    uint8_t enable_flow_ctrl;
} UARTInit;

enum {
    PARITY_NO   = 0,
    PARITY_ODD  = 1,
    PARITY_EVEN = 2,
};

#define BUF_SIZE 256

static char _rx_buf[BUF_SIZE];
static char _tx_buf[BUF_SIZE];
static RingBuffer rx_ring_buf;
static RingBuffer tx_ring_buf;

UARTInit _get_info_from_fdt(const uint8_t *fdt_ptr) {
    FDTHeader fdt_header          = get_fdt_header(fdt_ptr);
    const uint8_t *dt_struct_ptr  = fdt_ptr + fdt_header.off_dt_struct;
    const uint8_t *dt_strings_ptr = fdt_ptr + fdt_header.off_dt_strings;

    UARTInit uart_info = {
        .baudrate = 115200, .bits = 8, .parity = PARITY_NO, .enable_flow_ctrl = false
    };

#define GET_PATH_PROP(path, name) fdt_find_prop_by_path(dt_struct_ptr, dt_strings_ptr, path, name)

    FDTProp stdout_path_p   = GET_PATH_PROP("/chosen", "stdout-path");
    const char *stdout_path = (const char *)stdout_path_p.val_ptr;
    const char *uart_path;

    if (stdout_path != NULL) {
        char buf[128];
        buf[127] = '\0';
        strncpy(buf, stdout_path, 127);

        char *saveptr = NULL;
        uart_path     = strtok_r(buf, ":", &saveptr);
        stdout_path   = saveptr;

        if (stdout_path != NULL && *stdout_path) {
            const char *uart_ctrl = stdout_path;

            uart_info.baudrate = strtou32(stdout_path, &uart_ctrl, 10);
            if (*uart_ctrl) {
                switch (*uart_ctrl) {
                case 'n':
                    uart_info.parity = PARITY_NO;
                    break;
                case 'o':
                    uart_info.parity = PARITY_ODD;
                    break;
                case 'e':
                    uart_info.parity = PARITY_EVEN;
                    break;
                default:
                    uart_info.parity = PARITY_NO;
                }
                uart_ctrl++;
            }
            if (*uart_ctrl) {
                uart_info.bits = *uart_ctrl - '0';
                uart_ctrl++;
            }
            if (*uart_ctrl) {
                uart_info.enable_flow_ctrl = true;
            }
        }
    } else {
        FDTProp uart_path_p = GET_PATH_PROP("/aliases", "serial0");
        uart_path           = (const char *)uart_path_p.val_ptr;
    }

    if (uart_path == NULL || uart_path[0] == '\0') {
        return uart_info;
    }

    if (uart_path[0] != '/') {
        FDTProp uart_path_p = GET_PATH_PROP("/aliases", uart_path);
        uart_path           = (const char *)uart_path_p.val_ptr;
        if (uart_path == NULL)
            return uart_info;
    }

    FDTProp reg = GET_PATH_PROP(uart_path, "reg");
    if (reg.val_ptr == NULL)
        return uart_info;
    UART_BASE = fdt_read_u64_save(reg, 0x10000000);

    FDTProp reg_shift = GET_PATH_PROP(uart_path, "reg-shift");
    UART_REG_SHIFT    = fdt_read_u32_save(reg_shift, 0);

    FDTProp clock = GET_PATH_PROP(uart_path, "clock-frequency");
    if (clock.val_ptr == NULL) {
        clock = GET_PATH_PROP(uart_path, "clk-fpga");
    }
    UART_CLK = fdt_read_u32_save(clock, 14745600);

    FDTProp intr = GET_PATH_PROP(uart_path, "interrupts");
    UART_IRQ     = fdt_read_u32_save(intr, 10);

    return uart_info;
}

static void uart_intr_handle(uint32_t irq, void *context) {
    while (1) {
        uint32_t iir = read_reg(UART_IIR) & 0xf;

        if (iir & 1) // no interrupt is pending
            break;

        // rx fifo avail or timeout
        if (iir == 0x4 || iir == 0xc) {
            while ((read_reg(UART_LSR) & LSR_DR)) {
                char c = (char)(read_reg(UART_RBR));
                ring_buf_push(c, &rx_ring_buf);
            }
        }

        // tx fifo avail
        if (iir == 0x2) {
            char c;
            while ((read_reg(UART_LSR) & LSR_TDRQ) && ring_buf_pop(&c, &tx_ring_buf))
                write_reg(UART_THR, (unsigned int)c);

            if (ring_buf_empty(&tx_ring_buf)) // no data need to transmit
                clear_reg(UART_IER, 2);       // disable transmit intr
        }
    }
}

void init_uart(const uint8_t *fdt_ptr, const bool enable_fifo) {
    /*
     * TODOs
     * refactor init from fdt
     */
    UARTInit init_data = _get_info_from_fdt(fdt_ptr);

    // Disable all interrupt and enable UART unit
    write_reg(UART_IER, 0x40); // UUE at bit 6

    if (enable_fifo) {
        set_reg(UART_IER, 1);        // enable receive intr
        set_reg(UART_IER, (1 << 4)); // enable receiver timeout intr

        set_reg(UART_LCR, 0x80);
        write_reg(UART_FCR, 0x07 | 0x40);
        clear_reg(UART_LCR, 0x80);

        write_reg(UART_MCR, 0x08);

        plic_enable(UART_IRQ, 1);
        plic_register(UART_IRQ, uart_intr_handle, NULL);
    } else {
        write_reg(UART_FCR, 0x06); // reset transmit/receive FIFO at bit 1, 2
        write_reg(UART_FCR, 0x00); // disable FIFO
    }

    // Setting Baud Rate
    unsigned int divisor = (UART_CLK + (init_data.baudrate * 8)) / (init_data.baudrate * 16);
    unsigned char dll    = divisor & 0xff;
    unsigned char dlh    = (divisor >> 8) & 0xff;
    set_reg(UART_LCR, 0x80);
    write_reg(UART_DLL, dll);
    write_reg(UART_DLH, dlh);
    clear_reg(UART_LCR, 0x80);

    // Setting 8N1 data format
    set_reg(UART_LCR, init_data.bits == 8 ? 3 : 0);
    if (init_data.parity == PARITY_NO)
        clear_reg(UART_LCR, (1 << 3));
    else
        set_reg(UART_LCR, (1 << 3) | ((init_data.parity == PARITY_EVEN) << 4));
    clear_reg(UART_LCR, 4);

    // init fifo buffer
    ring_buf_init(_rx_buf, BUF_SIZE, &rx_ring_buf);
    ring_buf_init(_tx_buf, BUF_SIZE, &tx_ring_buf);

    UART_INIT_DONE = true;
}

void uart_putchar(char c) {
    while (ring_buf_full(&tx_ring_buf))
        asm volatile("wfi");

    if (c == '\n') {
        ring_buf_push('\r', &tx_ring_buf);
        set_reg(UART_IER, 2); // enable transmit intr
        while (ring_buf_full(&tx_ring_buf))
            asm volatile("wfi");
    }
    ring_buf_push(c, &tx_ring_buf);
    set_reg(UART_IER, 2); // enable transmit intr
}

char uart_getchar() {
    char c;
    while (!ring_buf_pop(&c, &rx_ring_buf))
        asm volatile("wfi");
    return c;
}
