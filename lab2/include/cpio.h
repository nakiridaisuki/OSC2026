#ifndef _CPIO_H_
#define _CPIO_H_

#include <stdint.h>

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
    const void *data;
    uint8_t is_end;
} CPIOFile;

CPIONewcHeader cpionewc_read_header(const char *ptr);
CPIOFile cpionewc_next_file(const char **ptr);

#endif // !_CPIO_H_
