# Lab -1

## Linker

In OSC2026, the linker was introduced in the first class.
They talk about how compilers and linkers translate high-level languages
(like C or C++) into something that can run on machines.
And how linkers work, what an ELF file is, relocation and static/dynamic linking.

### Compilation Flow

Programming languages are divided into two major types, compiled and interpreted.

- Compiled languages like C, C++ or any other languages need to be compiled before execution.
- Interpreted languages like Python use an interpreter to execute the code.

Here, we mainly focus on compiled languages (since the title is Compilation Flow).

High-level languages need to be compiled to machine code to be executed.
Although the word "compile" may represent the entire compilation process today,
it's actually one step in the compilation flow.

The compilation flow basically contains 4 steps:

1. Preprocessing
2. Compilation
3. Assembly
4. Linking

If you need to execute your compiled code, there is another step called "Loading",
meaning the OS loads your compiled code into memory via a loader.

In lab0, we just set up the development environment and there was nothing about compilation flow or other things,
so I did some self-study on this (maybe call Lab -1?).
In this lab0 directory, there is a hello.c file that contains very basic _hello world_ code.

```c
#include <stdio.h>
#define name "OSC2026"
int main() {
    printf("Hello %s", name);
    return 0;
}

```

Let's translate it into an executable file.

#### 1. Preprocessing

In the preprocessing step, compilers handle preprocessing directives like `#include` or `#define`.
Compilers will inject included files into the source file, and expand the macros.

Using the `-E` option in `gcc` generates the preprocessed file.

```shell
gcc -E hello.c -o hello.i
```

We can see a lot of other code is injected into `hello.i` and it comes from `stdio.h`.
Our main function is at the bottom and the `name` macro has been expanded into a string literal.

```c
# 0 "hello.c"
# 0 "<built-in>"
# 0 "<command-line>"
# 1 "/usr/include/stdc-predef.h" 1 3
# 0 "<command-line>" 2
# 1 "hello.c"
# 1 "/usr/include/stdio.h" 1 3
# 28 "/usr/include/stdio.h" 3

// ...

# 229 "/usr/lib/gcc/x86_64-pc-linux-gnu/16.1.1/include/stddef.h" 3
typedef long unsigned int size_t;
# 39 "/usr/include/stdio.h" 2 3

// ...

extern int fprintf (FILE *__restrict __stream,
      const char *__restrict __format, ...) __attribute__ ((__nonnull__ (1)));

// ...

# 2 "hello.c" 2


# 3 "hello.c"
int main() {
    printf("Hello %s", "OSC2026");
    return 0;
}

```

The preprocessed file looks horrible, but there is only one new thing.
The `typedef`, `struct` and `extern` function are things we already know.

The `typedef` and `struct` are for platform independence and standardization.
For instance, `size_t` is defined as `long unsigned int` on 64-bit systems,
and it may be `unsigned int` on 32-bit systems.

The `extern` functions define the prototype for the compiler in the next step.
The function's implementation will be linked in the linking step.

So, what are the # leading lines?

They are **Linemarkers** inserted by the preprocessor for compiler.
They offer line number information for compiler error messages or gdb tools.
The format looks like `# <line number> <file name> [flags]`.
For example:

- `# 1 "hello.c"`: means following code comes from line 1 of the file `hello.c`.
    So the following many lines are from `stdio.h`.
- `# 2 "hello.c" 2`: the flag `2` means return to original file.
    So now we are at file `hello.c` line 2

#### 2. Compilation

In the compilation step, the real "compiler" will compile the preprocessed code into assembly code.
Here, we first compile it into x86-64 assembly code.

Using the `-S` option in `gcc` generates the assembly code file.

```shell
gcc -S hello.i -o hello.s
```

We won't cover how to write the x86 assembly code,
so we will only introduce the data section and function calls here.

In the `hello.s` file, we focus on the following lines:

```asm
    .file "hello.c"
    .text
    .section .rodata
.LC0:
    .string "OSC2026"
.LC1:
    .string "Hello %s"
    .text
    .globl main
    .type main, @function

...
    call printf@PLT
...
```

The directives we care about now are `.rodata` and the function call.
The `.rodata` directive switches to the read only data section and store the two string literals,
then switches back to the text section to store the subsequent code.

The function `printf` is not defined in our assembly (it's in the C dynamic library `libc.so`)
so it uses the Procedure Linkage Table (PLT) to dynamically jump to it.

#### 3. Assembly

In the assembly step, the assembler will translate the assembly code further into machine code.
This results in a pure binary, ELF format, relocatable object file, or simply, the `.o` files.

Using the `-c` option in `gcc` generates the object file.

```shell
gcc -c hello.s -o hello.o
```

Now, we can't read the object file in a normal text editor without some plugins,
so we need some tools, like `readelf` on Linux, to play this file.

As we know about the ELF format file, there are some sections like `.text`, `.data` or `.bss`.

- `text` store your code, your logic.
- `data` store initialized data in your code.
- `bss` store uninitialized data in your code.
- ... and some other sections

We can use the following command to observe these sections.

```shell
readelf -S hello.o
```

The result looks like this:

```text
$ readelf -S hello.o
There are 14 section headers, starting at offset 0x280:

Section Headers:
  [Nr] Name              Type             Address           Offset
       Size              EntSize          Flags  Link  Info  Align
  [ 0]                   NULL             0000000000000000  00000000
       0000000000000000  0000000000000000           0     0     0
  [ 1] .text             PROGBITS         0000000000000000  00000040
       0000000000000029  0000000000000000  AX       0     0     1
  [ 2] .rela.text        RELA             0000000000000000  000001a8
       0000000000000048  0000000000000018   I      11     1     8
  [ 3] .data             PROGBITS         0000000000000000  00000069
       0000000000000000  0000000000000000  WA       0     0     1
  [ 4] .bss              NOBITS           0000000000000000  00000069
       0000000000000000  0000000000000000  WA       0     0     1
  [ 5] .rodata           PROGBITS         0000000000000000  00000069
       0000000000000011  0000000000000000   A       0     0     1
  [ 6] .comment          PROGBITS         0000000000000000  0000007a
       000000000000001c  0000000000000001  MS       0     0     1
  [ 7] .note.GNU-stack   PROGBITS         0000000000000000  00000096
       0000000000000000  0000000000000000           0     0     1
  [ 8] .note.gnu.pr[...] NOTE             0000000000000000  00000098
       0000000000000030  0000000000000000   A       0     0     8
  [ 9] .eh_frame         PROGBITS         0000000000000000  000000c8
       0000000000000038  0000000000000000   A       0     0     8
  [10] .rela.eh_frame    RELA             0000000000000000  000001f0
       0000000000000018  0000000000000018   I      11     9     8
  [11] .symtab           SYMTAB           0000000000000000  00000100
       0000000000000090  0000000000000018          12     4     8
  [12] .strtab           STRTAB           0000000000000000  00000190
       0000000000000015  0000000000000000           0     0     1
  [13] .shstrtab         STRTAB           0000000000000000  00000208
       0000000000000074  0000000000000000           0     0     1
Key to Flags:
  W (write), A (alloc), X (execute), M (merge), S (strings), I (info),
  L (link order), O (extra OS processing required), G (group), T (TLS),
  C (compressed), x (unknown), o (OS specific), E (exclude),
  D (mbind), l (large), p (processor specific)
```

I can't explain everything in detail here, so let's skip some of the output.
Besides the sections we know, there are two other important sections we need to focus on.

1. Symbol Table Section: `.symtab`
2. Relocation Section: `.rela.*`

**Symbol Table**:

Symbols are abstraction of the variables and functions in our code.
Our variables and functions will become some blocks of machine code or data in the object file.
We can consider a symbol as pointing to the first line of the code block it represents.

The symbol table records these symbols in our code.
When we use multiple files to implement a program, we need a tool, the **Linker**,
to combine all code blocks into a single executable file.
The linker relies on the symbol table to complete its job.

Using the following command shows the symbol table in the `.symtab` section from the object file.

```text
$ readelf -s hello.o

Symbol table '.symtab' contains 6 entries:
   Num:    Value          Size Type    Bind   Vis      Ndx Name
     0: 0000000000000000     0 NOTYPE  LOCAL  DEFAULT  UND 
     1: 0000000000000000     0 FILE    LOCAL  DEFAULT  ABS hello.c
     2: 0000000000000000     0 SECTION LOCAL  DEFAULT    1 .text
     3: 0000000000000000     0 SECTION LOCAL  DEFAULT    5 .rodata
     4: 0000000000000000    41 FUNC    GLOBAL DEFAULT    1 main
     5: 0000000000000000     0 NOTYPE  GLOBAL DEFAULT  UND printf
```

We first look at `Ndx`. It tells us where these symbols come from.
For example, the `main` symbol (our main function) comes from section 1.
From the section table above, we know that section 1 is `text`, so the `main` symbol comes from the `text` section.
In addition, the `Ndx` of the `printf` symbol is `UND`, meaning "undefined".
We don't know where `printf` is defined before linking, since `stdio.h` only declares the function,
and its actual implementation resides in the C library.

Another key field is the `Value`, which records the symbol's offset in its section.
The linker will combine all symbols in the same section, and since each symbol has its size,
this offset helps us find the real symbol position in the final linked file.

**Relocation**:

Recall the definition of the ELF format:

- `text` section record machine code.
- `data` section record initialized data.

Code in the `text` section records the address of variables in the `data` section for accessing.

After linking, the positions of variables in the `data` section may change
since the linker will combine the variables from all `data` sections into a single, final `data` section.
Therefore, we need to tell the linker which variable positions need to be updated in the `text` section.
This is what the relocation section records.

Using the following command shows relocation data in the `.rela.*` sections from the object file.

```text
$ readelf -r hello.o

Relocation section '.rela.text' at offset 0x1a8 contains 3 entries:
  Offset          Info           Type           Sym. Value    Sym. Name + Addend
000000000007  000300000002 R_X86_64_PC32     0000000000000000 .rodata - 4
00000000000e  000300000002 R_X86_64_PC32     0000000000000000 .rodata + 4
00000000001e  000500000004 R_X86_64_PLT32    0000000000000000 printf - 4

Relocation section '.rela.eh_frame' at offset 0x1f0 contains 1 entry:
  Offset          Info           Type           Sym. Value    Sym. Name + Addend
000000000020  000200000002 R_X86_64_PC32     0000000000000000 .text + 0
```

We focus on the `.rela.text` section, which contains three entries.
The first two entries correspond to our string literal data `'OSC2026'` and `'Hello %s'` in the `rodata` section (read-only data, kind of `data`).
The third entry is the `printf` function.
Function calls also need to jump to a specific address, so this target location must be updated as well.

#### 4. Linking

Finally, we can link our object file into a final executable file.
Creating an executable file is `gcc`'s default behavior, so no options needed.

```shell
gcc hello.o -o hello
gcc hello.o -o hello_static -static
```

There are two ways to link: Static and Dynamic.

**Static Linking**:

Static linking generates a self-contained executable file,
which means we can execute this file without any dependencies.
It bundles all the necessary code and data into the executable.

**Dynamic Linking**:

Dynamic linking leaves some functions, like `printf`, shared among multiple executables externally,
and depends on some `.so` files to execute. Here, our `hello` depends on `libc.so`.
We can use the `ldd` tool to check it.

```text
$ ldd hello
    linux-vdso.so.1 (0x00007fb1dcbf4000)
    libc.so.6 => /usr/lib/libc.so.6 (0x00007fb1dc800000)
    /lib64/ld-linux-x86-64.so.2 => /usr/lib64/ld-linux-x86-64.so.2 (0x00007fb1dcbf6000)
```

Being self-contained isn't free. The main purpose of dynamic linking is to reduce the size of the executable.
We can see the difference in size between the two types of linking:

```shell
$ ll hello hello_static 
-rwxr-xr-x 1 nakiri users  16K Jul  3 22:35 hello*
-rwxr-xr-x 1 nakiri users 824K Jul  3 22:35 hello_static*
```

Dynamic linking **really** reduces the file size significantly.

The linking has three main steps, section merging, symbol resolution and address allocation.

**Section Merging**:

As we said before, if we have multiple `.o` files,
the linker needs to combine all sections in every object file into one section respectively.

For example, using the following command tells linker to generate linking report.

```shell
gcc -Wl,-Map=output.map hello.o -o hello
```

Let's see following lines in `output.map`:

```text
.text           0x0000000000001040      0x122
 *(.text.unlikely .text.*_unlikely .text.unlikely.*)
 *(.text.exit .text.exit.*)
 *(.text.startup .text.startup.*)
 *(.text.hot .text.hot.*)
 *(SORT_BY_NAME(.text.sorted.*))
 *(.text .stub .text.* .gnu.linkonce.t.*)
 .text          0x0000000000001040       0x26 /usr/lib/gcc/x86_64-pc-linux-gnu/16.1.1/../../../../lib/Scrt1.o
                0x0000000000001040                _start
 .text          0x0000000000001066        0x0 /usr/lib/gcc/x86_64-pc-linux-gnu/16.1.1/../../../../lib/crti.o
 *fill*         0x0000000000001066        0xa 
 .text          0x0000000000001070       0xc9 /usr/lib/gcc/x86_64-pc-linux-gnu/16.1.1/crtbeginS.o
 .text          0x0000000000001139       0x29 hello.o
                0x0000000000001139                main
 .text          0x0000000000001162        0x0 /usr/lib/gcc/x86_64-pc-linux-gnu/16.1.1/crtendS.o
 .text          0x0000000000001162        0x0 /usr/lib/gcc/x86_64-pc-linux-gnu/16.1.1/../../../../lib/crtn.o
 *(.gnu.warning)
```

We can see our final `text` section contains `text` sections from `Scrt1.o`, `crti.o`... and our `hello.o`.

**Symbol Resolution**:

After merge all sections, the linker will update the undefined symbols.
If you have seen some error messages like `Undefined reference`, that means the linker can't find the definition after merging.
Typically because you loss some object file to the linker.

Our `printf` function is undefined before linking.
Let's check if it's defined in static linked executable.

Using the following command to find `printf` definition:

```shell
$ readelf -s hello_static | grep printf

...
   822: 000000000043aac0   257 FUNC    GLOBAL HIDDEN     5 __printf_buffer_[...]
   824: 0000000000406150   199 FUNC    GLOBAL DEFAULT    5 __printf
   841: 0000000000406150   199 FUNC    GLOBAL DEFAULT    5 printf
   843: 00000000004384f0   195 FUNC    GLOBAL HIDDEN     5 __printf_buffer_write
   855: 000000000043da40    59 FUNC    GLOBAL HIDDEN     5 __printf_buffer_[...]
...
```

We can see our `printf` has its definition now.

**Address Allocation**:

After find all needed sections and merge them, we need to decide the address of these sections.
We have seen the linking report in `output.map`, the address `0x00....1040` and other things are the final address of the sections.

Once the address has been given, linker can relocate all variables and function calls in the text section,
and generate the final executable file we need.
