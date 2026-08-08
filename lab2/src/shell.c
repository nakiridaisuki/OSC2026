#include "shell.h"
#include "cpio.h"
#include "printf.h"
#include "string.h"
#include "uart.h"
#include <stdbool.h>
#include <stddef.h>

uint32_t ls() {
    const char *cpio_start_addr = (const char *)CPIO_START_ADDR;

    uint32_t total_files = 0;
    CPIOFile cpio_file   = cpionewc_next_file(&cpio_start_addr);
    printf("%-10s %-10s\n", "size", "filename");
    while (cpio_file.data != NULL) {
        printf("%-10d %s\n", cpio_file.header.filesize, cpio_file.name);
        cpio_file = cpionewc_next_file(&cpio_start_addr);
        total_files++;
    }
    printf("Total %d files.\n", total_files);
    return 0;
}
uint32_t cat(char *path) {
    if (path == NULL) {
        printf("ERROR: Can't get file name.\n");
        return 1;
    }

    const char *cpio_start_addr = (const char *)CPIO_START_ADDR;
    CPIOFile cpio_file          = cpionewc_next_file(&cpio_start_addr);
    bool finded                 = false;
    while (cpio_file.data != NULL) {
        if (strcmp(path, cpio_file.name) == 0) {
            finded = true;
            break;
        }
        cpio_file = cpionewc_next_file(&cpio_start_addr);
    }

    if (finded) {
        for (size_t i = 0; i < cpio_file.header.filesize; i++)
            uart_putchar(cpio_file.data[i]);
    } else {
        printf("cat: %s: No such file or directory\n", path);
        return 1;
    }
    return 0;
}
