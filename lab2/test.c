#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "include/dtb.h"
#include "utils.h"

int main() {

    FILE *file = fopen("../common/x1_orangepi-rv2.dtb", "rb");
    if (file == NULL) {
        perror("Can't open file!");
        return 1;
    }

    fseek(file, 0, SEEK_END);
    long file_size = ftell(file);
    rewind(file);

    uint8_t *buffer = (uint8_t *)malloc(file_size);
    if (buffer == NULL) {
        perror("Memory allocate faild");
        fclose(file);
        return 1;
    }

    size_t bytes_read = fread(buffer, 1, file_size, file);
    if (bytes_read != file_size) {
        printf("Warning: read bytes %zu != file size %ld\n", bytes_read, file_size);
    } else {
        printf("Read %zu bytes data\n", bytes_read);
    }

    struct FDTHeader header       = get_fdt_header(buffer);
    const uint8_t *dt_struct_ptr  = buffer + header.off_dt_struct;
    const uint8_t *dt_strings_ptr = buffer + header.off_dt_strings;

    printf("Find aliases node: \n");
    FDTProp serial0 = fdt_find_prop_by_path(dt_struct_ptr, dt_strings_ptr, "/aliases", "serial0");
    printf("Property find: %s\n", serial0.name_ptr);
    printf("Property len: %d\n", serial0.len);
    printf("Property value: %s\n", serial0.val_ptr);

    printf("Find uart node %s: \n", serial0.val_ptr);
    const char *uart_path = (const char *)serial0.val_ptr;
    FDTProp uart0_reg     = fdt_find_prop_by_path(dt_struct_ptr, dt_strings_ptr, uart_path, "reg");
    printf("Property find: %s\n", uart0_reg.name_ptr);
    printf("Property len: %d\n", uart0_reg.len);
    printf("UART0 base address: 0x%lx\n", BE_uint64(uart0_reg.val_ptr));
    printf("UART0 size: 0x%lx\n", BE_uint64(uart0_reg.val_ptr + 8));

    free(buffer);
    fclose(file);

    return 0;
}
