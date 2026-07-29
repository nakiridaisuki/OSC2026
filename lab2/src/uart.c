#include "uart.h"
#include "dtb.h"
#include "string.h"

uint64_t UART_CLK       = 14750000;
uint64_t UART_BASE      = 0x10000000;
uint32_t UART_REG_SHIFT = 0;

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

    return uart_info;
}

void uart_init_from_fdt(const uint8_t *fdt_ptr) {
    /*
     * TODOs
     * 1. bits setting
     * 2. flow control setting
     * 2.
     */
    UARTInit init_data = _get_info_from_fdt(fdt_ptr);

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
