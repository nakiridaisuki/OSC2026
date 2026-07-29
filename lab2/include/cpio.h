#ifndef _CPIO_H_
#define _CPIO_H_

#include "types.h"
#include <stdint.h>

extern phys_addr_t CPIO_START_ADDR;
extern phys_addr_t CPIO_END_ADDR;

#define CPIO_NEWC_HEADER_FIELDS \
    X(inode)                    \
    X(mode)                     \
    X(uid)                      \
    X(gid)                      \
    X(nlink)                    \
    X(mtime)                    \
    X(filesize)                 \
    X(devmajor)                 \
    X(devminor)                 \
    X(rdevmajor)                \
    X(rdevminor)                \
    X(namesize)                 \
    X(check)

typedef struct {
#define X(f_name) uint32_t f_name;
    CPIO_NEWC_HEADER_FIELDS
    uint8_t avail;
#undef X
} CPIONewcHeader; // CPIO New ASCII Format Header

typedef struct {
    CPIONewcHeader header;
    const char *name;
    const uint8_t *data;
} CPIOFile;

const char *cpionewc_init_from_fdt(const uint8_t *fdt_ptr);
CPIONewcHeader cpionewc_read_header(const char *ptr);
CPIOFile cpionewc_next_file(const char **ptr);

#endif // !_CPIO_H_
