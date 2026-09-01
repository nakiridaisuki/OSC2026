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

// FDT property struct
typedef struct {
    uint32_t len;
    const uint8_t *prop_ptr;
    const char *name_ptr;
    const uint8_t *val_ptr;
} FDTProp;

typedef struct {
#define X(field_name, idx) uint32_t field_name;
    FDT_HEADER_FIELDS
#undef X
} FDTHeader;

typedef void (*fdt_node_cb_t)(const uint8_t *node_ptr, const char *node_name, void *opaque_arg);

int fdt_node_name_eq(const char *node_name, const char *seg, size_t seg_len);

FDTHeader get_fdt_header(const uint8_t *fdt_ptr);

const uint8_t *
fdt_find_node(const uint8_t *dt_struct_ptr, const char *node_name, size_t target_len);
FDTProp
fdt_find_prop(const uint8_t *dt_struct_ptr, const uint8_t *dt_strings_prt, const char *prop_name);
void fdt_foreach_subnode(const uint8_t *parent_ptr, fdt_node_cb_t callback, void *opaque_arg);

const uint8_t *fdt_find_node_by_path(const uint8_t *dt_struct_ptr, const char *path);
inline static FDTProp fdt_find_prop_by_path(
    const uint8_t *dt_struct_ptr,
    const uint8_t *dt_strings_prt,
    const char *path,
    const char *prop_name
) {
    dt_struct_ptr = fdt_find_node_by_path(dt_struct_ptr, path);
    return fdt_find_prop(dt_struct_ptr, dt_strings_prt, prop_name);
}

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
static inline uint64_t fdt_read_num_save(FDTProp prop, uint64_t default_val) {
    if (prop.val_ptr == NULL)
        return default_val;
    if (prop.len == 4)
        return BE_uint32(prop.val_ptr);
    return BE_uint64(prop.val_ptr);
}
void fdt_list_all_props(const uint8_t *dt_struct_ptr, const uint8_t *dt_strings_prt);
void fdt_list_all_subnodes(const uint8_t *dt_struct_ptr);

#endif // !_DTB_H_
