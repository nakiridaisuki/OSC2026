# Lab 3

## Basic Exercise 1 / Advance Exercise 1

In the first exercise, we need to build a buddy system and a page allocator base on it.

The buddy system is a algorithm to solve the external fragment problem.
It divides the memory region into power of 2 chunks. Looks like this:

```
|                  32k                  |
|        16k        |        16k        |
|    8k   |    8K   |    8K   |    8K   |
| 4K | 4k | 4K | 4k | 4K | 4k | 4K | 4k |
```

(4K is the size of one page.)

Every time a requester need some memory, the system will pick a minimum available chunk for it.
The word "available" here means the chunk is large enough and in the *free list*.
If the allocated chunk is too large for requester, the system will cut it in half and save the other half chunk into the *free list*.
And keep do it until it can't.
The advantage of this system is it can recycle the free small chunks and merge them into larger chunk
since the chunk size is always the power of 2.
[More information.](https://en.wikipedia.org/wiki/Buddy_system)

To implement this system, we first need a page array and a free list.

The page array store the information of a page,
like the chunk size, if it's a part of a chunk or is allocated.

```c
struct Page {
    int8_t order; // >=0 for chunk size, -1 for merged
    uint8_t allocated;
    LinkedListNode list; // for free list
};
```

The free list is a doubly circular linked list store free chunks of each size.
The number of free lists if $O(logn)$.

```text
-> head0 <-> chunk <-> chunk ... chunk <-
-> head1 <-> chunk ... chunk <-
-> head2 <-
...
-> head63 <-> chunk <-> chunk ... chunk <-
```

The following pseudo code for allocator:

```python
palloc(size) {
    order <- min exp for 4KB * 2^exp >= size.
    for i from order to MAX_ORDER:
        if free_list[i] is not empty:
            take a chunk C from free_list[i].

    if didn't find C:
        EXIT

    while C.order > order:
        R <- right half of chunk C.
        L <- left  half of chunk C.
        add R into free_list[C.order - 1].
        C <- L.

    M <- real memory region of chunk C.
    return M
}
```

This function can allocate minimum available chunk in $O(logn)$ time.

How to free the page chunk is what buddy system truly valuable.
We can use XOR to find the buddy chunk of current chunk.
For example, the same graph as above but with page index this time:

```
|0                                      |
|0                  |4                  |
|0        |2        |4        |6        |
|0   |1   |2   |3   |4   |5   |6   |7   |
```

If page `0` and `1` need to merge. The chunk order is 0.
For any page of them, the index XOR $2^0$ will become the other one's index.

$0 xor 1 = 1$ \
$1 xor 1 = 0$

Another example, if two chunks page `4` and `6` with order 1 need to merge.
The index need to XOR $2^1$:

$100_2 xor 010_2 = 110_2$ \
$110_2 xor 010_2 = 100_2$

Using this method, we can get buddy chunk in $O(1)$ time and merge if we need.
The following pseudo code for freeing page:

```python
pfree(C) {
    while C.order < MAX_ORDER - 1:
        buddy_idx <- C.index ^ (1 << C.order)
        B <- page_arr[buddy_idx]

        if B.allocated || B.order != C.order:
            break;

        remove B from free_list[B.order].
        C <- merged chunk of C and B.
    }
    add C into free_list[C.order].
}
```

This function can merge free chunks in $O(logn)$ time.

## Basic Exercise 2

Next exercise is about small (<4KB) memory allocation.

Buddy system can solve external fragmentation but do nothing about internal fragmentation.
The dynamic memory allocator will slice a page into fix size blocks pools (16, 32, 64 bytes, etc),
and allocate memory from this pools.

In this system I designed, each page will become a fix size blocks pool.
If a page is assigned as a 32-byte pool, it will contain 4096/32 = 128 blocks.
If it's a 256-byte pool, it will contain 4096/256 = 16 blocks... etc.

The advantage is easy to implement,
but the performance may worse then allocate different size in one page
if there are only a few blocks needed for each size.

To implement this, we need to add some data into the page structure.
The "slab" means a block in the page.

```c
struct Page {
    int8_t order;
    uint8_t allocated;
    LinkedListNode list;
    uint16_t slab_count; // How many blocks have been used
    uint16_t slab_size;  // The size of a block
    void *slab_head;     // The block linked list head
};
```

We also use linked list to store our free memory data,
but this time, the data is stored inside the beginning of the unallocated memory region,
not a separate memory region.

```txt
      __    __    __    __    __
     /  \  /  \  /  \  /  \../  \
| 16B | 16B | 16B | 16B | ... | 16B |
\___________________________________/
             4KB page
```

The following pseudo code for dynamic allocator:

```python
dalloc(size) {
    order <- min exp for 16B * 2^exp >= size.

    if no page left for current order:
        P <- palloc(PAGE_SIZE)

        P.slab_count <- 0
        P.slab_size  <- 16 << order
        P.slab_head  <- memory address of P

        total_slabs <- 4069 / P.slab_size;
        for i from 0 to total_slabs - 2:
            save address of next slab into beginning of current slab.
        add P into free_slabs[order].

    avail_page <- page in free_slabs[order].
    Take a slab S from avail_page.
    avail_page.slab_count++;

    if avail_page is full:
        remove avail_page from free_slabs[order].

    M <- real memory region of slab C.
    return M
}
```

The overall process is similar to page allocator but has a slabs initialization.

The following process for freeing a slab:

```python
dfree(S) {
    P <- page of slab S.
    order <- P.slab_size / 16
    if P is full:
        add P back into free_slabs[order].

    Add S back into P's slab list.

    P.slab_count--;
    if (P.slab_count == 0) {
        P.slab_size = 0;
        remove P from free_slabs[order].
        pfree(P);
    }
}
```

## Startup Allocation

We have talked about how to implement memory allocation system.
Next problem is where to store the metadata like `free_slabs`, `free_list` and `page_arr`.

The `free_slabs` and `free_list` store the header node of linked lists.
It only takes 16 bytes per node and has $O(logn)$ nodes so it's fine to declare it as a global variable and store in `.bss` section.

However, the `page_arr` has 32 bytes per entity and has # of pages entities.
If we have a 8GB memory, we will have 8G/4K = 2M pages.
We can't declare it as a global variable since the size of total memory depended on the machine.
And of course we don't have `malloc` before we initialize our allocator.

Thus, we need a early allocator to allocate some memory for us.
Here, I simply take the memory region after our kernel image, just like a heap starting at the stack "top".

```txt
 _____________ Our Kernel ________________
/                                         \
| .text | .bss, .data ... | ... stack ... | ....
                                        ^   ^ -->
                                stack top   start allocate here
```

We can use `extern` to get stack top address from linker script.
The following code is my early allocator.

```c
extern uint8_t _stack_top[];
phys_addr_t early_mem_ptr = (phys_addr_t)_stack_top;

static void *_early_alloc(uint32_t size) {
    early_mem_ptr = ALIGN_UP_8(early_mem_ptr);
    void *mem_ptr = (void *)early_mem_ptr;
    early_mem_ptr += size;
    return mem_ptr;
}
```

Notice the wired declaration:

```c
extern uint8_t _stack_top[];
```

The `_stack_top` in linker script is a symbol of a address.
We can't use `extern uint8_t *_stacktop;` since it will become a "variable" of a address.

If we write the code like following:

```c
extern uint8_t *_stack_top;
phys_addr_t early_mem_ptr = (phys_addr_t)_stack_top;
```

The compiler will first find the address of `_stack_top` (which is the real stack top) and get the value from that address.
For example, if the stack top address is `0x1000`, the result will become:

```c
phys_addr_t early_mem_ptr = (phys_addr_t)(*(0x1000));
// maybe zero or some garbage data in 0x1000
```

By using the array declaration `[]`, the array name `_stack_top` will auto decay to beginning address of array;
The other way to get memory address is use `&`:

```c
extern uint8_t _stack_top; // a variable
phys_addr_t early_mem_ptr = (phys_addr_t)(&_stack_top);
```

## Reserved Memory & Memory Zone

### Reserved Memory

There are some memory region we can't allocate since they have been used or reserved.
For example, the kernel image, DTB, ramdisk and reserved memory in DTB.

We can allocate memory for them after initialized our allocator, but it will cause some internal fragmentation.
The other way is reserving them before initialization and merge left pages using `pfree`.
