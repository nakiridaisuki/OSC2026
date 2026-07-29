#include "cpio.h"
#include "utils.h"
#include <stdbool.h>
#include <string.h>

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
    CPIOFile result = {.name = NULL, .data = NULL, .is_end = false};

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
        result.is_end = true;
    }

    return result;
}
