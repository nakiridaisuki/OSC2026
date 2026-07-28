#include "dtb.h"
#include "printf.h"
#include "string.h"
#include "uart.h"
#include "utils.h"
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

void _align_4(const uint8_t **addr_ptr) {
    *addr_ptr = (const uint8_t *)(((uint64_t)*addr_ptr + 3) & ~3);
}

uint32_t _get_u32_with_off(const uint8_t **addr_ptr) {
    if (addr_ptr == NULL || *addr_ptr == NULL)
        return 0;

    uint32_t result = BE_uint32((const uint8_t *)*addr_ptr);
    *addr_ptr += sizeof(uint32_t);
    return result;
}

int _node_name_eq(const char *node_name, const char *seg, size_t seg_len) {
    if (strncmp(node_name, seg, seg_len) != 0)
        return 0;

    char next_char = node_name[seg_len];
    return next_char == '\0' || next_char == '@';
}

static inline uint32_t _fdt_read_u32_save(FDTProp prop, uint32_t default_val) {
    if (prop.val_ptr == NULL)
        return default_val;
    return BE_uint32(prop.val_ptr);
}
static inline uint64_t _fdt_read_u64_save(FDTProp prop, uint64_t default_val) {
    if (prop.val_ptr == NULL)
        return default_val;
    return BE_uint64(prop.val_ptr);
}

FDTHeader get_fdt_header(const uint8_t *fdt_ptr) {
    FDTHeader header;
#define X(field_name, idx) header.field_name = BE_uint32(fdt_ptr + idx * 4);
    FDT_HEADER_FIELDS
#undef X

    return header;
}

FDTEvent fdt_next(FDTIterator *iter) {
    while (1) {
        iter->event_start = iter->cursor;

        uint32_t token = _get_u32_with_off(&iter->cursor);

        if (token == FDT_END)
            return FDT_END;
        if (token == FDT_NOP)
            continue;

        if (token == FDT_BEGIN_NODE) {
            const char *node_name = (const char *)iter->cursor;
            iter->cursor += strlen(node_name) + 1;
            _align_4(&iter->cursor);

            iter->name = node_name;
            iter->depth++;

            return FDT_BEGIN_NODE;
        }
        if (token == FDT_END_NODE) {
            iter->depth--;
            return FDT_END_NODE;
        }
        if (token == FDT_PROP) {
            iter->len     = _get_u32_with_off(&iter->cursor);
            iter->nameoff = _get_u32_with_off(&iter->cursor);

            iter->val = iter->cursor;
            iter->cursor += iter->len;
            _align_4(&iter->cursor);

            if (iter->strings != NULL)
                iter->name = (const char *)(iter->strings + iter->nameoff);
            return FDT_PROP;
        }
    }
}

const uint8_t *
fdt_find_node(const uint8_t *dt_struct_ptr, const char *target_name, size_t target_len) {
    FDTIterator iter = {dt_struct_ptr, NULL, 0};
    FDTEvent ev;
    while ((ev = fdt_next(&iter)) != FDT_END) {
        if (ev == FDT_BEGIN_NODE) {
            if (target_len == 0) {
                if (iter.depth == 1 && iter.name[0] == '\0')
                    return iter.event_start;
            } else {
                if (iter.depth == 2 && _node_name_eq(iter.name, target_name, target_len))
                    return iter.event_start;
            }
        }
        if (ev == FDT_END_NODE) {
            if (iter.depth <= 0)
                break;
            continue;
        }
    }
    return NULL;
}
FDTProp
fdt_find_prop(const uint8_t *dt_struct_ptr, const uint8_t *dt_strings_prt, const char *prop_name) {
    FDTProp result   = {0, NULL, "", NULL};
    FDTIterator iter = {dt_struct_ptr, dt_strings_prt, 0};
    FDTEvent ev;
    while ((ev = fdt_next(&iter)) != FDT_END) {
        if (ev == FDT_END_NODE) {
            if (iter.depth <= 0)
                break;
            continue;
        }
        if (ev == FDT_PROP) {
            if (iter.depth == 1 && strcmp(prop_name, iter.name) == 0) {
                result.len      = iter.len;
                result.prop_ptr = iter.event_start;
                result.name_ptr = iter.name;
                result.val_ptr  = iter.val;
                return result;
            }
        }
    }
    return result;
}

const uint8_t *fdt_find_node_by_path(const uint8_t *dt_struct_ptr, const char *path) {
    path += 1;
    while (*path) {
        const char *next_slash = strchr(path, '/');
        size_t seg_len         = next_slash ? next_slash - path : strlen(path);

        if (seg_len > 0) {
            dt_struct_ptr = fdt_find_node(dt_struct_ptr, path, seg_len);
            if (dt_struct_ptr == NULL) {
                return NULL;
            }
        }

        if (next_slash == NULL) {
            break;
        }
        path = next_slash + 1;
    }
    return dt_struct_ptr;
}

FDTProp fdt_find_prop_by_name(
    const uint8_t *dt_struct_ptr,
    const uint8_t *dt_strings_prt,
    const char *node_name,
    const char *prop_name
) {
    dt_struct_ptr = fdt_find_node(dt_struct_ptr, node_name, strlen(node_name));
    return fdt_find_prop(dt_struct_ptr, dt_strings_prt, prop_name);
}
FDTProp fdt_find_prop_by_path(
    const uint8_t *dt_struct_ptr,
    const uint8_t *dt_strings_prt,
    const char *path,
    const char *prop_name
) {
    dt_struct_ptr = fdt_find_node_by_path(dt_struct_ptr, path);
    return fdt_find_prop(dt_struct_ptr, dt_strings_prt, prop_name);
}

//////////////////// Utils Functions /////////////////////////

void fdt_list_all_props(const uint8_t *dt_struct_ptr, const uint8_t *dt_strings_prt) {
    FDTIterator iter = {dt_struct_ptr, dt_strings_prt, 0};
    FDTEvent ev;
    while ((ev = fdt_next(&iter)) != FDT_END) {
        if (ev == FDT_END_NODE) {
            if (iter.depth <= 0)
                break;
            continue;
        }
        if (ev == FDT_PROP) {
            if (iter.depth == 1)
                printf("Property: %s\n", iter.name);
        }
    }
}
void fdt_list_all_subnodes(const uint8_t *dt_struct_ptr) {
    FDTIterator iter = {dt_struct_ptr, NULL, 0};
    FDTEvent ev;
    while ((ev = fdt_next(&iter)) != FDT_END) {
        if (ev == FDT_BEGIN_NODE) {
            if (iter.depth == 1) {
                printf("Node: <%s>\n", iter.name);
            }
            if (iter.depth == 2) {
                printf("SubNode: %s\n", iter.name);
            }
        }
        if (ev == FDT_END_NODE) {
            if (iter.depth == 0)
                break;
            continue;
        }
    }
}

UARTInit fdt_get_uart_info(const uint8_t *fdt_ptr) {
    /*
     * TODOs
     * 1. Try get uart from aliases if no stdout-path
     */
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
    uart_info.reg_shift = _fdt_read_u32_save(reg_shift, uart_info.reg_shift);

    FDTProp clock = GET_PATH_PROP(uart_path, "clock-frequency");
    if (clock.val_ptr == NULL) {
        clock = GET_PATH_PROP(uart_path, "clk-fpga");
    }
    uart_info.clock = _fdt_read_u32_save(clock, uart_info.clock);

    return uart_info;
}
