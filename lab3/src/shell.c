#include "shell.h"
#include "cpio.h"
#include "dstruc.h"
#include "malloc.h"
#include "printf.h"
#include "sbi.h"
#include "string.h"
#include "uart.h"
#include <stdint.h>

typedef struct {
    const char *name;
    int (*func)(char *args);
    const char *desc;
} shell_cmd_t;

#define X(name, desc) {#name, name, desc},
shell_cmd_t CMD_TABLE[] = {SHELL_FUNC_LIST};
#undef X

#define X(name, desc) CMD_##name,
enum { SHELL_FUNC_LIST CMD_TABLE_SIZE };
#undef X

int ls(char *args) {
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
int cat(char *args) {
    char *path = strtok(args, " ");

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

int hello(char *args) {
    printf("Hello World.\n");
    return 0;
}

int help(char *args) {
    printf("Avaliable commands:\n");
    for (size_t i = 0; i < CMD_TABLE_SIZE; i++) {
        printf("  %5s - %s.\n", CMD_TABLE[i].name, CMD_TABLE[i].desc);
    }
    return 0;
}

int info(char *args) {
    struct sbiret spec_ver = sbi_get_spec_version();
    struct sbiret impl_id  = sbi_get_impl_id();
    struct sbiret impl_ver = sbi_get_impl_version();

    printf("System information:\n");
    printf("  OpenSBI specification version: 0x%016lx\n", spec_ver.value);
    printf("  implementation ID: 0x%016lx\n", impl_id.value);
    printf("  implementation version: 0x%016lx\n", impl_ver.value);
    check_extensions();

    return 0;
}

int test_mem(char *args) {
    char *s_size = strtok(args, " ");

    if (s_size == NULL) {
        printf("ERROR: Can't get size.\n");
        return 1;
    }

    uint32_t size = strtou32(s_size, NULL, 10);
    uint8_t *ptr  = malloc(size);
    if (ptr == NULL) {
        printf("Out of Memory\n");
    } else {
        printf("Success allocate memory at 0x%lx\n", ptr);
    }
    return 1;
}

int shell() {
    int idx = 0;
    char buf[256];
    while (1) {
        printf("opi-rv2> ");
        memset(buf, 0, sizeof(buf));
        idx = 0;

        while (1) {
            char c = uart_getchar();

            if (c == '\r' || c == '\n') {
                buf[idx] = 0;
                uart_putchar('\n');
                break;
            } else if (c == 127 || c == '\b') {
                if (idx > 0) {
                    idx--;
                    printf("\b \b");
                }
            } else if (idx < 255) {
                buf[idx++] = c;
                uart_putchar(c);
            }
        }

        int matched = 0;
        char *cmd   = strtok(buf, " ");
        if (cmd == NULL)
            continue;
        char *args = strtok(NULL, "");
        if (args == NULL)
            args = "";

        for (size_t i = 0; i < CMD_TABLE_SIZE; i++) {
            if (strcmp(cmd, CMD_TABLE[i].name) == 0) {
                CMD_TABLE[i].func(args);
                matched++;
                break;
            }
        }

        if (!matched && strlen(buf) > 0) {
            printf("Unknow command: ");
            printf("%s", buf);
            printf("\n");
        }
    }
}
