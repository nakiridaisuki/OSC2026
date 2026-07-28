#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "include/dtb.h"
#include "utils.h"

#include "string.h"

int main() {

    char *str    = "124";
    uint32_t tmp = strtou32(str, NULL, 10);
    printf("%d\n", tmp);

    return 0;
}
