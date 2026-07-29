#include "cpio.h"
#include "dtb.h"
#include "string.h"
#include "utils.h"
#include <stdbool.h>

phys_addr_t CPIO_START_ADDR = 0;
phys_addr_t CPIO_END_ADDR   = 0;

const char *cpionewc_init_from_fdt(const uint8_t *fdt_ptr) {
    FDTHeader fdt_header          = get_fdt_header(fdt_ptr);
    const uint8_t *dt_struct_ptr  = fdt_ptr + fdt_header.off_dt_struct;
    const uint8_t *dt_strings_ptr = fdt_ptr + fdt_header.off_dt_strings;
    FDTProp initrd_start_p =
        fdt_find_prop_by_path(dt_struct_ptr, dt_strings_ptr, "/chosen", "linux,initrd-start");
    FDTProp initrd_end_p =
        fdt_find_prop_by_path(dt_struct_ptr, dt_strings_ptr, "/chosen", "linux,initrd-end");

    CPIO_START_ADDR = fdt_read_u64_save(initrd_start_p, 0);
    CPIO_END_ADDR   = fdt_read_u64_save(initrd_end_p, 0);

    return (const char *)CPIO_START_ADDR;
}

CPIONewcHeader cpionewc_read_header(const char *ptr) {
    CPIONewcHeader header;

    if (strncmp(ptr, "070701", 6) != 0) {
        header.avail = false;
        return header;
    }
    header.avail = true;
    ptr += 6;

#define X(f_name)                                \
    header.f_name = strntou32(ptr, NULL, 8, 16); \
    ptr += 8;
    CPIO_NEWC_HEADER_FIELDS
#undef X

    return header;
}

CPIOFile cpionewc_next_file(const char **ptr) {
    CPIOFile result = {.name = NULL, .data = NULL};

    if (ptr == NULL || *ptr == NULL)
        return result;

    result.header = cpionewc_read_header(*ptr);
    if (!result.header.avail)
        return result;
    *ptr += 110;

    result.name = *ptr;
    *ptr += result.header.namesize;
    *ptr = ALIGN_4(*ptr);

    result.data = *ptr;
    *ptr += result.header.filesize;
    *ptr = ALIGN_4(*ptr);

    if (strcmp(result.name, "TRAILER!!!") == 0) {
        result.data = NULL;
    }

    return result;
}
