#ifndef _SBI_H_
#define _SBI_H_

#include <stdint.h>

struct sbiret {
    long error;
    long value;
};

#define SBI_EXT_BASE 0x10
#define SBI_EXT_SRST 0x53525354

#define SBI_BASE_GET_SPEC_VERSION 0x0
#define SBI_BASE_GET_IMPL_ID      0x1
#define SBI_BASE_GET_IMPL_VERSION 0x2

#define SBI_SRST_SYSTEM_RESET 0x0

struct sbiret sbi_ecall(
    int ext,
    int fid,
    unsigned int arg0,
    unsigned int arg1,
    unsigned int arg2,
    unsigned int arg3,
    unsigned int arg4,
    unsigned int arg5
);
struct sbiret sbi_get_spec_version(void);
struct sbiret sbi_get_impl_id(void);
struct sbiret sbi_get_impl_version(void);

struct sbiret sbi_warm_reboot(void);

#endif // !_SBI_H_
