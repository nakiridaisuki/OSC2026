#include "shell.h"
#include "cpio.h"
#include "printf.h"
#include "sbi.h"
#include "string.h"
#include "syscall.h"
#include "timer.h"
#include "uart.h"
#include <stdarg.h>
#include <stdint.h>
#include <stdlib.h>

typedef struct {
    const char *name;
    int (*func)(char *args);
    const char *desc;
} shell_cmd_t;

#define X(name, desc) {#name, name, desc},
static const shell_cmd_t CMD_TABLE[] = {SHELL_FUNC_LIST};
#undef X

#define X(name, desc) CMD_##name,
enum { SHELL_FUNC_LIST CMD_TABLE_SIZE };
#undef X

static void uprintf(const char *fmt, ...) {
    char buf[512];

    va_list args;
    va_start(args, fmt);

    int len = vsnprintf(buf, sizeof(buf), fmt, args);

    va_end(args);

    if (len > 0)
        // printf("%s", buf);
        uart_write(buf, len);
}

int ls(char *args) {
    const char *cpio_start_addr = (const char *)CPIO_START_ADDR;

    uint32_t total_files = 0;
    CPIOFile cpio_file   = cpionewc_next_file(&cpio_start_addr);
    uprintf("%-10s %-10s\n", "size", "filename");
    while (cpio_file.data != NULL) {
        uprintf("%-10d %s\n", cpio_file.header.filesize, cpio_file.name);
        cpio_file = cpionewc_next_file(&cpio_start_addr);
        total_files++;
    }
    uprintf("Total %d files.\n", total_files);
    return 0;
}
int cat(char *args) {
    char *path = strtok(args, " ");

    if (path == NULL) {
        uprintf("ERROR: Can't get file name.\n");
        return 1;
    }

    CPIOFile cpio_file;
    if (cpionewc_find(&cpio_file, path)) {
        // printf("%s", (char *)cpio_file.data);
        uart_write((char *)cpio_file.data, cpio_file.header.filesize);
    } else {
        uprintf("cat: %s: No such file or directory\n", path);
        return 1;
    }
    return 0;
}

int hello(char *args) {
    uprintf("Hello World.\n");
    return 0;
}

int help(char *args) {
    uprintf("Avaliable commands:\n");
    for (size_t i = 0; i < CMD_TABLE_SIZE; i++) {
        uprintf("  %5s - %s.\n", CMD_TABLE[i].name, CMD_TABLE[i].desc);
    }
    return 0;
}

int info(char *args) {
    struct sbiret spec_ver = sbi_get_spec_version();
    struct sbiret impl_id  = sbi_get_impl_id();
    struct sbiret impl_ver = sbi_get_impl_version();

    uprintf("System information:\n");
    uprintf("  OpenSBI specification version: 0x%016lx\n", spec_ver.value);
    uprintf("  implementation ID: 0x%016lx\n", impl_id.value);
    uprintf("  implementation version: 0x%016lx\n", impl_ver.value);
    check_extensions();

    return 0;
}

static void _timeout_cb(void *arg) {
    char *str = (char *)arg;
    uprintf("%s\n", str);
    free(str);
}

int timeout(char *args) {
    char *str;
    char *time_s = strtok_r(args, " ", &str);

    if (time_s == NULL) {
        uprintf("ERROR: Didn't set time.\n");
        return 1;
    }
    if (str == NULL)
        str = "";

    char *buf = malloc(strlen(str) + 1);
    strcpy(buf, str);

    uint32_t delay_s = strtou32(time_s, NULL, 10);
    Timer *timer     = malloc(sizeof(Timer));
    timer_add(timer, delay_s * 1000, _timeout_cb, buf);

    uprintf("Timer added\n");

    return 0;
}

int shell() {
    int idx = 0;
    char buf[256];
    while (1) {
        uprintf("opi-rv2> ");
        memset(buf, 0, sizeof(buf));
        idx = 0;

        while (1) {
            char c;
            uart_read(&c, 1);
            // c = uart_getchar();

            if (c == '\r' || c == '\n') {
                buf[idx] = 0;
                uprintf("\n");
                break;
            } else if (c == 127 || c == '\b') {
                if (idx > 0) {
                    idx--;
                    uprintf("\b \b");
                }
            } else if (idx < 255) {
                buf[idx++] = c;
                uprintf("%c", c);
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
            uprintf("Unknow command: ");
            uprintf("%s (%d)", buf, strlen(buf));
            uprintf("\n");
        }
    }
}
