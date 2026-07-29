#include "dtb.h"
#include "printf.h"
#include "string.h"
#include "uart.h"
#include "utils.h"
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

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
            iter->cursor = ALIGN_4(iter->cursor);

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
            iter->cursor = ALIGN_4(iter->cursor);

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
