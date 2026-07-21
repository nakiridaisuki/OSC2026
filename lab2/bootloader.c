#include "printf.h"
#include "sbi.h"
#include "string.h"
#include "uart.h"

#define MAGIC_NUM   0x544F4F42
#define KERNEL_BASE 0x00200000

void waiting_magic_num() {
    int state = 0;

    while (1) {
        char c = uart_getchar();
        if (state == 0 && c == (MAGIC_NUM & 0xFF)) {
            state = 1;
        } else if (state == 1 && c == (MAGIC_NUM >> 8 & 0xFF)) {
            state = 2;
        } else if (state == 2 && c == (MAGIC_NUM >> 16 & 0xFF)) {
            state = 3;
        } else if (state == 3 && c == (MAGIC_NUM >> 24 & 0xFF)) {
            break;
        } else {
            if (c == (MAGIC_NUM & 0xFF))
                state = 1;
            else
                state = 0;
        }
    }
}

int main() {
    uart_init(115200, true);

    unsigned long current_pc;
    asm volatile("auipc %0, 0" : "=r"(current_pc));

    printf("\n=========================================\n");
    printf("Bootloader initialized successfully!\n");
    printf("Current Program Counter (PC) is: 0x%016lx\n", current_pc);
    printf("=========================================\n\n");

    char buf[256];

    int idx = 0;
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
            printf("  help - show this help.\n");
            printf("  hello - print Hello World.\n");
            printf("  load - load kernel image to 0x00200000.\n");
            printf("  info - print system info.\n");
        } else if (strcmp(buf, "load") == 0) {
            printf("Waiting for connection...\n");

            waiting_magic_num();
            unsigned int kernel_size = uart_getuint32();
            unsigned char *load_addr = (unsigned char *)KERNEL_BASE;
            for (int i = 0; i < kernel_size; i++) {
                load_addr[i] = uart_getchar();
            }
            asm volatile("fence.i");

            printf("Connected! Kernel size is: %u bytes\n", kernel_size);
            printf("Kernel loaded at %p\n", load_addr);
            printf("Jump to kernel...\n");

            for (volatile int d = 0; d < 5000000; d++)
                ;

            void (*kernel_entry)(void) = (void (*)(void))load_addr;
            kernel_entry();

        } else if (strcmp(buf, "info") == 0) {
            struct sbiret spec_ver = sbi_get_spec_version();
            struct sbiret impl_id  = sbi_get_impl_id();
            struct sbiret impl_ver = sbi_get_impl_version();

            printf("System information:\n");

            printf("  OpenSBI specification version: 0x%016lx\n", spec_ver.value);
            printf("  implementation ID: 0x%016lx\n", impl_id.value);
            printf("  implementation version: 0x%016lx\n", impl_ver.value);
        } else if (strlen(buf) > 0) {
            printf("Unknow command: ");
            printf("%s", buf);
            printf("\n");
        }
    }
}
