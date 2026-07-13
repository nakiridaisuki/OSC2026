#include "printf.h"
#include "sbi.h"
#include "string.h"
#include "uart.h"

int main() {
    uart_init(115200);

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
            printf("  info - print system info.\n");
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
            printf(buf);
            printf("\n");
        }
    }
}
