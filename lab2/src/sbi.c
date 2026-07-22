#include "sbi.h"
#include "printf.h"
#include "string.h"
#include <stddef.h>

struct sbiret sbi_ecall(
    unsigned long ext,
    unsigned long fid,
    unsigned long arg0,
    unsigned long arg1,
    unsigned long arg2,
    unsigned long arg3,
    unsigned long arg4,
    unsigned long arg5
) {
    register unsigned long a0 asm("a0") = arg0;
    register unsigned long a1 asm("a1") = arg1;
    register unsigned long a2 asm("a2") = arg2;
    register unsigned long a3 asm("a3") = arg3;
    register unsigned long a4 asm("a4") = arg4;
    register unsigned long a5 asm("a5") = arg5;
    register unsigned long a6 asm("a6") = fid;
    register unsigned long a7 asm("a7") = ext;

    asm volatile("ecall"
                 : "+r"(a0), "+r"(a1)
                 : "r"(a2), "r"(a3), "r"(a4), "r"(a5), "r"(a6), "r"(a7)
                 : "memory");

    struct sbiret ret;
    ret.error = a0;
    ret.value = a1;
    return ret;
}

// Auto generate base extension function implementation
#define X(fid, name_const, func_name)                                                              \
    struct sbiret func_name(void) { return sbi_ecall(SBI_EXT_BASE, fid, 0, 0, 0, 0, 0, 0); }
BASE_FID_LIST
#undef X
struct sbiret sbi_probe_extension(long eid) {
    return sbi_ecall(SBI_EXT_BASE, BASE_PROBE_EXTENSION, eid, 0, 0, 0, 0, 0);
}

struct sbiret sbi_warm_reboot(void) {
    return sbi_ecall(SBI_EXT_SRST, SRST_SYSTEM_RESET, 2, 0, 0, 0, 0, 0);
}

///////////////////////// Utils Functions /////////////////////////

void check_extensions() {
    struct Ext {
        unsigned long eid;
        const char *ename;
    };

    static const struct Ext ext_list[] = {
#define X(eid, name_const, name_str) {eid, name_str},
        SBI_EXT_LIST
#undef X
    };

    size_t num_exts = sizeof(ext_list) / sizeof(ext_list[0]);

    for (size_t i = 0; i < num_exts; i++) {
        long result = sbi_probe_extension(ext_list[i].eid).value;
        printf("%s: %s\n", ext_list[i].ename, result ? "Supported" : "Unsupported");
    }
}
