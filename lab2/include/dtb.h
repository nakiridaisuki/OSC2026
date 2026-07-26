#ifndef _H_DEB_
#define _H_DEB_

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
enum {
    FDT_BEGIN_NODE = 0x1,
    FDT_END_NODE   = 0x2,
    FDT_PROP       = 0x3,
    FDT_NOP        = 0x4,
    FDT_END        = 0x9,
};

// FDT property struct
struct fdt_prop {
    uint32_t len;
    uint32_t nameoff;
};

struct fdt_header {
#define X(field_name, idx) uint32_t field_name;
    FDT_HEADER_FIELDS
#undef X
};

void dtb_parsing(const uint8_t *dtb);
void dtb_find_node(const uint8_t *dtb, const char *path);
void dtb_get_prop(const uint8_t *dtb, const char *path);

#endif // !_H_DEB_
