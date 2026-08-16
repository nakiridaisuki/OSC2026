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

The free list is a linked list store free chunks of each size.
The number of free lists if $O(logn)$.

```c
struct Page {
    int8_t order; // >=0 for chunk size, -1 for merged
    uint8_t allocated;
    LinkedListNode list; // for free list
};
```

The following pseudo code for allocator;

```python
palloc(size) {
    order <- min exp for 2^exp >= size.
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
The following pseudo code for allocator;

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
