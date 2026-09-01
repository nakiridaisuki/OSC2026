#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "dtb.h"
#include "string.h"

int main() {

    FILE *file       = fopen("../common/x1_orangepi-rv2.dtb", "rb");
    size_t max_size  = 200000;
    char *fdt_ptr    = (char *)malloc(max_size);
    size_t bytesRead = fread(fdt_ptr, 1, max_size, file);
    printf("Successfully read %zu bytes.\n", bytesRead);
    fclose(file);

    FDTHeader fdt_header = get_fdt_header(fdt_ptr);

    // 檢查 Endianness 是否有正確轉換 (Debug 用)
    printf("Debug: off_dt_struct = %u\n", fdt_header.off_dt_struct);
    const uint8_t *dt_struct_ptr = (const uint8_t *)fdt_ptr + fdt_header.off_dt_struct;

    fdt_list_all_subnodes(dt_struct_ptr);

    free(fdt_ptr);
    fclose(file);
    return 0;
}
