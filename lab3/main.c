#include "cpio.h"
#include "malloc.h"
#include "printf.h"
#include "shell.h"
#include "uart.h"
#include <stdint.h>

void test_alloc_1() {
    printf("Testing memory allocation...\n");
    char *ptr1 = (char *)malloc(4000);
    char *ptr2 = (char *)malloc(8000);
    char *ptr3 = (char *)malloc(4000);
    char *ptr4 = (char *)malloc(4000);

    free(ptr1);
    free(ptr2);
    free(ptr3);
    free(ptr4);

    /* Test kmalloc */
    printf("Testing dynamic allocator...\n");
    char *kmem_ptr1 = (char *)malloc(16);
    char *kmem_ptr2 = (char *)malloc(32);
    char *kmem_ptr3 = (char *)malloc(64);
    char *kmem_ptr4 = (char *)malloc(128);

    free(kmem_ptr1);
    free(kmem_ptr2);
    free(kmem_ptr3);
    free(kmem_ptr4);

    char *kmem_ptr5 = (char *)malloc(16);
    char *kmem_ptr6 = (char *)malloc(32);

    free(kmem_ptr5);
    free(kmem_ptr6);

    // Test malloc new page if the cache is not enough
    void *kmem_ptr[102];
    for (int i = 0; i < 100; i++) {
        kmem_ptr[i] = (char *)malloc(128);
    }
    for (int i = 0; i < 100; i++) {
        free(kmem_ptr[i]);
    }

    // Test exceeding the maximum size
    const uint64_t MAX_ALLOC_SIZE = MEM_SIZE;
    char *kmem_ptr7               = (char *)malloc(MAX_ALLOC_SIZE + 1);
    if (kmem_ptr7 == NULL) {
        printf("Allocation failed as expected for size > MAX_ALLOC_SIZE\n");
    } else {
        printf("Unexpected allocation success for size > MAX_ALLOC_SIZE\n");
        free(kmem_ptr7);
    }

    // Test memory access
    char *users[4];
    for (int i = 0; i < 4; i++) {
        users[i] = (char *)malloc(128);
        sprintf(users[i], "I am user %d\n", i);
    }
    for (int i = 0; i < 4; i++) {
        printf("%s", users[i]);
    }
    free(users[1]);
    free(users[2]);
    printf("%s", users[0]);
    printf("%s", users[3]);

    users[1] = (char *)malloc(128);
    users[2] = (char *)malloc(128);
    sprintf(users[1], "I am new user %d\n", 1);
    sprintf(users[2], "I am new user %d\n", 2);
    for (int i = 0; i < 4; i++) {
        printf("%s", users[i]);
    }
    for (int i = 0; i < 4; i++) {
        free(users[i]);
    }
}

int main(unsigned long hartid, const uint8_t *fdt_ptr) {

    uart_init_from_fdt(fdt_ptr);
    printf("UART Initialize successfully!\n");

    cpionewc_init_from_fdt(fdt_ptr);
    printf("initrd start address: 0x%p\n", CPIO_START_ADDR);

    init_palloc();
    init_dalloc();
    test_alloc_1();

    shell();

    return 0;
}
