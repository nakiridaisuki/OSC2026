#ifndef _SBI_H_
#define _SBI_H_

#define SBI_EXT_LIST                                                  \
    X(0x10, SBI_EXT_BASE, "Base Extension")                           \
    X(0x54494D45, SBI_EXT_TIME, "Timer Extension")                    \
    X(0x735049, SBI_EXT_IPI, "IPI Extension")                         \
    X(0x52464E43, SBI_EXT_RFENCE, "RFENCE Extension")                 \
    X(0x48534D, SBI_EXT_HSM, "Hart State Management Extension")       \
    X(0x53525354, SBI_EXT_SRST, "System Reset Extension")             \
    X(0x504D55, SBI_EXT_PMU, "Performance Monitoring Unit Extension") \
    X(0x4442434E, SBI_EXT_DBCN, "Debug Console Extension")            \
    X(0x53555350, SBI_EXT_SUSP, "System Suspend Extension")           \
    X(0x43505043, SBI_EXT_CPPC, "CPPC Extension")                     \
    X(0x4E41434C, SBI_EXT_NACL, "Nested Acceleration Extension")      \
    X(0x535441, SBI_EXT_STA, "Steal-time Accounting Extension")

enum {
#define X(eid, name_const, name_str) name_const = eid,
    SBI_EXT_LIST
#undef X
};

#define BASE_FID_LIST                                   \
    X(0x0, BASE_GET_SPEC_VERSION, sbi_get_spec_version) \
    X(0x1, BASE_GET_IMPL_ID, sbi_get_impl_id)           \
    X(0x2, BASE_GET_IMPL_VERSION, sbi_get_impl_version) \
    X(0x4, BASE_GET_MVENDORID, sbi_get_mvendorid)       \
    X(0x5, BASE_GET_MARCHID, sbi_get_marchid)           \
    X(0x6, BASE_GET_MIMPID, sbi_get_mimpid)

enum Base {
    BASE_PROBE_EXTENSION = 0x3,
#define X(fid, name_const, func_name) name_const = fid,
    BASE_FID_LIST
#undef X
};

enum SystemReset {
    SRST_SYSTEM_RESET = 0x0,
};

struct sbiret {
    long error;
    long value;
};

struct sbiret sbi_ecall(
    unsigned long ext,
    unsigned long fid,
    unsigned long arg0,
    unsigned long arg1,
    unsigned long arg2,
    unsigned long arg3,
    unsigned long arg4,
    unsigned long arg5
);

// Auto generate base extension function define
#define X(fid, name_const, func_name) struct sbiret func_name(void);
BASE_FID_LIST
#undef X
struct sbiret sbi_probe_extension(long eid);

struct sbiret sbi_warm_reboot(void);

///////////////////////// Utils Functions /////////////////////////

void check_extensions();

#endif // !_SBI_H_
