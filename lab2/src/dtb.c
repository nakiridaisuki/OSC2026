#include "dtb.h"
#include "printf.h"
#include "utils.h"
#include <stddef.h>
#include <stdint.h>
#include <string.h>

void align_4(size_t *addr) { *addr += (4 - *addr % 4) % 4; }

void dtb_parsing(const uint8_t *dtb_ptr) {
    struct fdt_header header;
#define X(field_name, idx) header.field_name = BE_uint32(dtb_ptr + idx * 4);
    FDT_HEADER_FIELDS
#undef X

#define X(field_name, idx) printf("Field %s is: 0x%x\n", #field_name, header.field_name);
    FDT_HEADER_FIELDS
#undef X

    const uint8_t *dt_struct_ptr  = dtb_ptr + header.off_dt_struct;
    const uint8_t *dt_strings_ptr = dtb_ptr + header.off_dt_strings;

    while (BE_uint32(dt_struct_ptr) != FDT_END) {
        uint32_t token = BE_uint32(dt_struct_ptr);
        dt_struct_ptr += sizeof(uint32_t);
        if (token == FDT_NOP) {
            continue;
        }

        if (token == FDT_BEGIN_NODE) {
            // Get node name
            char node_name[32];
            strcpy(node_name, (const char *)dt_struct_ptr);
            dt_struct_ptr += strlen(node_name);
            align_4((size_t *)&dt_struct_ptr);
            printf("##### Parsing node: %s\n", node_name);

            // Get node properties
            while (BE_uint32(dt_struct_ptr) != FDT_BEGIN_NODE) {
                uint32_t token = BE_uint32(dt_struct_ptr);
                dt_struct_ptr += sizeof(uint32_t);
                if (token == FDT_NOP)
                    continue;
                if (token == FDT_END_NODE)
                    break;

                if (token == FDT_PROP) {
                    printf("Find a property\n");
                    struct fdt_prop prop;
                    prop.len = BE_uint32(dt_struct_ptr);
                    dt_struct_ptr += 4;
                    prop.nameoff = BE_uint32(dt_struct_ptr);
                    dt_struct_ptr += 4;

                    char prop_name[32];
                    strcpy(prop_name, (const char *)(dt_strings_ptr + prop.nameoff));
                    printf("Property len: %d\n", prop.len);
                    printf("Property name: %s\n", prop_name);

                    dt_struct_ptr += prop.len;
                    align_4((size_t *)&dt_struct_ptr);
                }
            }
        }
    }
}
