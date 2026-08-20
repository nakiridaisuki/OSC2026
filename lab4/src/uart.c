#include "uart.h"
#include "dtb.h"
#include "printf.h"
#include "string.h"
#include "trap.h"
#include "utils.h"
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

uint64_t UART_CLK       = 14750000;
uint64_t UART_BASE      = 0x10000000;
uint32_t UART_REG_SHIFT = 0;
uint32_t UART_IRQ;

bool UART_INIT_DONE = false;

typedef struct {
    phys_addr_t base_addr;
    uint32_t clock;
    uint32_t reg_shift;
    uint32_t baudrate;
    uint32_t interrupts;
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

#define RX_BUF_SIZE 256

static char rx_buffer[RX_BUF_SIZE];
static int rx_buf_head = 0;
static int rx_buf_tail = 0;

void rx_buf_push(char c) {
    int next = (rx_buf_head + 1) % RX_BUF_SIZE;
    if (next != rx_buf_tail) {
        rx_buffer[rx_buf_head] = c;
        rx_buf_head            = next;
    }
}

int rx_buf_pop(char *c) {
    if (rx_buf_head == rx_buf_tail)
        return 0;
    *c          = rx_buffer[rx_buf_tail];
    rx_buf_tail = (rx_buf_tail + 1) % RX_BUF_SIZE;
    return 1;
}

UARTInit _get_info_from_fdt(const uint8_t *fdt_ptr) {
    FDTHeader fdt_header          = get_fdt_header(fdt_ptr);
    const uint8_t *dt_struct_ptr  = fdt_ptr + fdt_header.off_dt_struct;
    const uint8_t *dt_strings_ptr = fdt_ptr + fdt_header.off_dt_strings;

    UARTInit uart_info = {
        .base_addr        = 0,
        .reg_shift        = 0,
        .clock            = 14745600,
        .baudrate         = 115200,
        .bits             = 8,
        .parity           = PARITY_NO,
        .enable_fifo      = false,
        .enable_flow_ctrl = false
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
    uart_info.base_addr = BE_uint64(reg.val_ptr);

    FDTProp reg_shift   = GET_PATH_PROP(uart_path, "reg-shift");
    uart_info.reg_shift = fdt_read_u32_save(reg_shift, uart_info.reg_shift);

    FDTProp clock = GET_PATH_PROP(uart_path, "clock-frequency");
    if (clock.val_ptr == NULL) {
        clock = GET_PATH_PROP(uart_path, "clk-fpga");
    }
    uart_info.clock = fdt_read_u32_save(clock, uart_info.clock);

    FDTProp intr         = GET_PATH_PROP(uart_path, "interrupts");
    uart_info.interrupts = BE_uint32(intr.val_ptr);

    return uart_info;
}

void uart_init_from_fdt(const uint8_t *fdt_ptr) {
    /*
     * TODOs
     * refactor init from fdt
     */
    UARTInit init_data    = _get_info_from_fdt(fdt_ptr);
    init_data.enable_fifo = true;

    UART_CLK       = init_data.clock;
    UART_BASE      = init_data.base_addr;
    UART_REG_SHIFT = init_data.reg_shift;
    UART_IRQ       = init_data.interrupts;

    // Disable all interrupt and enable UART unit
    write_reg(UART_IER, 0x40); // UUE at bit 6

    if (init_data.enable_fifo) {
        set_reg(UART_IER, 1);        // enable receive intr
        set_reg(UART_IER, (1 << 4)); // enable receiver timeout intr

        set_reg(UART_LCR, 0x80);
        write_reg(UART_FCR, 0x07 | 0x40);
        clear_reg(UART_LCR, 0x80);

        write_reg(UART_MCR, 0x08);

        PLIC_SET_PRIO(init_data.interrupts, 1);
        PLIC_SET_PRIO_THLD(1, 0);
        PLIC_ENABLE(1, init_data.interrupts);
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
    // Setting 8N1 data format
    write_reg(UART_LCR, 0x03);

    UART_INIT_DONE = true;
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
    char c;
    while (!rx_buf_pop(&c))
        asm volatile("wfi");
    return c;
}

void uart_intr_handle() {
    uint32_t iir = read_reg(UART_IIR);

    printf("IIR is 0x%x\n", iir);

    // rx fifo full or timeout
    if ((iir & 0xf) == 0x4 || (iir & 0xf) == 0xc) {
        while ((read_reg(UART_LSR) & LSR_DR)) {
            char c = (char)(read_reg(UART_RBR));
            rx_buf_push(c);
        }
    }
}

int uart_getuint32() {
    int result = 0;
    for (int i = 0; i < 4; i++) {
        char tmp = uart_getchar();
        result |= (tmp & 0xff) << (8 * i);
    }
    return result;
}
