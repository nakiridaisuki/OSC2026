#ifndef _DTB_H_
#define _DTB_H_

#include "utils.h"
#include <stddef.h>
#include <stdint.h>

// clang-format off
// X(field_name, field_idx)
#define FDT_HEADER_FIELDS \
    X(magic            , 0) \
    X(totalsize        , 1) \
    X(off_dt_struct    , 2) \
    X(off_dt_strings   , 3) \
    X(off_mem_rsvmap   , 4) \
    X(version          , 5) \
    X(last_comp_version, 6) \
    X(boot_cpuid_phys  , 7) \
    X(size_dt_strings  , 8) \
    X(size_dt_struct   , 9)
// clang-format on

// FDT struct block tokens
typedef enum {
    FDT_BEGIN_NODE = 0x1,
    FDT_END_NODE   = 0x2,
    FDT_PROP       = 0x3,
    FDT_NOP        = 0x4,
    FDT_END        = 0x9,
} FDTEvent;

// FDT property struct
typedef struct {
    uint32_t len;
    const uint8_t *prop_ptr;
    const char *name_ptr;
    const uint8_t *val_ptr;
} FDTProp;

typedef struct {
    const uint8_t *cursor;
    const uint8_t *strings;
    int depth;

    const uint8_t *event_start;
    const char *name;
    const uint8_t *val;
    uint32_t len;
    uint32_t nameoff;
} FDTIterator;

typedef struct {
#define X(field_name, idx) uint32_t field_name;
    FDT_HEADER_FIELDS
#undef X
} FDTHeader;

FDTHeader get_fdt_header(const uint8_t *fdt_ptr);

const uint8_t *
fdt_find_node(const uint8_t *dt_struct_ptr, const char *node_name, size_t target_len);
FDTProp
fdt_find_prop(const uint8_t *dt_struct_ptr, const uint8_t *dt_strings_prt, const char *prop_name);

const uint8_t *fdt_find_node_by_path(const uint8_t *dt_struct_ptr, const char *path);
FDTProp fdt_find_prop_by_name(
    const uint8_t *dt_struct_ptr,
    const uint8_t *dt_strings_prt,
    const char *node_name,
    const char *prop_name
);
FDTProp fdt_find_prop_by_path(
    const uint8_t *dt_struct_ptr,
    const uint8_t *dt_strings_prt,
    const char *path,
    const char *prop_name
);

//////////////////// Utils Functions /////////////////////////

static inline uint32_t fdt_read_u32_save(FDTProp prop, uint32_t default_val) {
    if (prop.val_ptr == NULL)
        return default_val;
    return BE_uint32(prop.val_ptr);
}
static inline uint64_t fdt_read_u64_save(FDTProp prop, uint64_t default_val) {
    if (prop.val_ptr == NULL)
        return default_val;
    return BE_uint64(prop.val_ptr);
}
void fdt_list_all_props(const uint8_t *dt_struct_ptr, const uint8_t *dt_strings_prt);
void fdt_list_all_subnodes(const uint8_t *dt_struct_ptr);

#endif // !_DTB_H_
