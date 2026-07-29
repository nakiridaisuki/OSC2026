#include "cpio.h"
#include "dtb.h"
#include "printf.h"
#include "sbi.h"
#include "string.h"
#include "uart.h"
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

int main(unsigned long hartid, const uint8_t *fdt_ptr) {

    uart_init_from_fdt(fdt_ptr);
    printf("UART Initialize successfully!\n");

    cpionewc_init_from_fdt(fdt_ptr);
    printf("initrd start address: %p\n", CPIO_START_ADDR);

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

        if (strcmp(buf, "hello") == 0)
            printf("Hello World.\n");
        else if (strcmp(buf, "help") == 0) {
            printf("Avaliable commands:\n");
            printf("  info - print system info.\n");
            printf("  hello - print Hello World.\n");
            printf("  ls - list all file in ramdisk.\n");
            printf("  cat - print file contant.\n");
            printf("  help - show this help.\n");
        } else if (strcmp(buf, "ls") == 0) {
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
        } else if (strncmp(buf, "cat", 3) == 0) {
            char *token = strtok(buf, " ");
            token       = strtok(NULL, " ");

            if (token == NULL) {
                printf("ERROR: Can't get file name.\n");
                continue;
            }

            const char *cpio_start_addr = (const char *)CPIO_START_ADDR;
            CPIOFile cpio_file          = cpionewc_next_file(&cpio_start_addr);
            bool finded                 = false;
            while (cpio_file.data != NULL) {
                if (strcmp(token, cpio_file.name) == 0) {
                    finded = true;
                    break;
                }
                cpio_file = cpionewc_next_file(&cpio_start_addr);
            }

            if (finded) {
                for (size_t i = 0; i < cpio_file.header.filesize; i++)
                    uart_putchar(cpio_file.data[i]);
            } else {
                printf("cat: %s: No such file or directory\n", token);
            }
        } else if (strcmp(buf, "info") == 0) {
            struct sbiret spec_ver = sbi_get_spec_version();
            struct sbiret impl_id  = sbi_get_impl_id();
            struct sbiret impl_ver = sbi_get_impl_version();

            printf("System information:\n");

            printf("  OpenSBI specification version: 0x%016lx\n", spec_ver.value);
            printf("  implementation ID: 0x%016lx\n", impl_id.value);
            printf("  implementation version: 0x%016lx\n", impl_ver.value);

            check_extensions();
        } else if (strlen(buf) > 0) {
            printf("Unknow command: ");
            printf("%s", buf);
            printf("\n");
        }
    }
}
