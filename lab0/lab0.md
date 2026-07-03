# Lab 0

This section is about Linker and environment setup.

## Linker

In OSC2026, linker is introduced in first class.
They talk about how compiler and linker translate high level language
(like C or C++) to something can work on machine.
And how linker working, what is ELF file,
relocation and static/dynamic linking.

### Translation Flow

Programing languages are divided to two major types, Translation and Interpretation.
Translation language like C, C++ or any language need to compile before execute.
Interpretation language like Python, they have a interpreter to execute your code.
Here, we mainly talk about translation languages
(since the title is Translation Flow).

High level language need to be compiled to machine code to execute.
The "compile" word now may be to represent the whole process of translation,
but it's actually one step in translation flow.
The translation flow basically contain 4 steps:

1. Preprocessing
2. Compilation
3. Assembly
4. Linking

If you want to execute your compiled code, their is another step called "Loading",
meaning OS load your compiled code into memory via loader.

In lab0, we just handle the develop environment
and not about translation flow or other things,
so I have my own study about this (maybe call Lab -1?).
In this lab0 directory, their is a hello.c file contains
very basic _hello world_ code.

```c
#include <stdio.h>
#define name "OSC2026"
int main() {
    printf("Hello %s", name);
    return 0;
}

```

Let's translate it into executable file.

#### 1. Preprocessing

In preprocessing step, compiler handle preprocessing command like `#include` or `#define`.
Compiler will inject included file into the processing file,
and extend the define command.

Using `-E` option in `gcc` can get the preprocessed file.

```shell
gcc -E hello.c -o hello.i
```

You can see a lot of other codes are injected into `hello.i`, they are from `stdio.h`.
And our main function is at the bottom and
the `name` define has been extended into string literal.

The preprocessed file looks horrible, but there are only one new thing.
The `typedef`, `struct` and `extern` function define are things we already know.

The `typedef` and `struct` is for platform independence and standardize.
For instance, `size_t` is defined to `long unsigned int` on 64 bit system,
and it may be `unsigned int` on 32 bit system.

The `extern` function define for compiler in next step,
define the prototype for functions in this header file.
The function's implementation will be linked in linking step.

So, what is the # leading lines?

There are _Linemarkers_ inserted by preprocessor for compiler.
It offers line number informations for compiler error message or gdb tools.
The formate typically like `# <line number> <file name> [flags]`.
For example:

- `# 1 "hello.c"`: means following code from file `hello.c` line one.
    So following many lines are from `stdio.h`.
- `# 2 "hello.c" 2`: the flag `2` means return to original file.
    So now we are at file `hello.c` line 2

#### 2. Compilation

In compilation step, the real "compiler" will compile the preprocessed code into assembly code.
Here we first compile it into x86-64 assembly code.

Using `-S` option in `gcc` can get the assembly code file.

```shell
gcc -S hello.i -o hello.s
```

We don't talk about how to write the x86 assembly code, so only introduce the data section here.
In the `hello.s` file, we focus on following lines:

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

The things we care now are the `.rodata` command and the function call.
The `.rodata` switch to read only data section and store the two string literal,
than switch back to text section to store following codes.

The function `printf` doesn't in our assembly, it's in the c dynamic library `libc.so`,
so here it uses the Procedure Linkage Table (PLT) to dynamic jump.

#### 3. Assembly

In assembly step, the assembler will translate the assembly code further into machine code.
Become pure binary, ELF format, relocatable object file, or simply, the `.o` files.

Using `-c` option in `gcc` can get the object file.

```shell
gcc -c hello.s -o hello.o
```

Now, the object file can't read by normal text editor without some plugins,
so we need some tools like `readelf` on Linux to play this file.

As we know about ELF format file, there are some sections like `.text`, `.data` or `.bss`.
We can use following command to observe these sections.

```shell
readelf -S hello.o
```

The result look like this:

```shell
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

I won't and I can't explain whole things here, so let me skip it.
Beside the sections we know, there are tow other important section we need to talk about here.

1. Symbol Table Section: `.symtab`
2. Relocation Section: `.rela.*`

_Symbol Table_:
Symbol is an abstraction of variables and functions in our code.
Our variables and functions will become some blocks of machine code in object file.
We can consider the symbol point to the first line of that code block.

Symbol table records those symbols in our code.
When we use multiple file to implement a function, we need a tool, Linker,
to combine all code blocks into a single executable file.
Linker relay on symbol table to complete it's job.

Using following command can get symbol table in `.symtab` section from object file.

```shell
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

We first see the `Ndx`. It tell us where this symbol come from.
For example, the `main` symbol (our main function) come from section 1.
By the section table above, we know the section 1 is text, so `main` symbol come from `text` section.
And the `Ndx` of `printf` symbol is `UND`, meaning undefined.
We don't know where is the `printf` defined before linking
since function `printf` is included from `stdio.h`.

Next important value is `Value`, it records symbol's offset in it's section.
Linker will combine all symbol in the same section, each symbol has it's size.
`Value` offset help us find the real symbol position in final linked file.

_Relocation_:
Recall the definition of ELF formate,
`text` section record machine codes,
`data` section record initialized data.
Codes in `text` section record the position of variables in `data` sections for accessing.

After linking, the position of variables in `data` section may differ since linker will combine
all `data` section's variables into single final `data` section.
So we need to tell linker which variables position need to be checks in `text` section.
This is what _Relocation_ section records.

Using following command can get relocation data in `.rela.*` section from object file.

```shell
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

We focus on `.rela.text` section, there are three entries.
First two entries is our string literal data `'OSC2026'` and `'Hello %s'` in `data` section.
The third entry is the `printf` function.
Function call also need to jump to a specific position, so the position also need to be updated.

#### 4. Linking

Finally, we can link our object file into a final executable file.
Creating an executable file is `gcc`'s default behavior, so no options needed.

```shell
gcc hello.o -o hello
gcc hello.o -o hello_static -static
```

There are two way to link, Static and Dynamic.

_Static Linking_:
Static linking generate a self contained executable file,
which means we can execute this file without any dependences.
It combine all needed codes, data into it.

_Dynamic Linking_:
Dynamic linking leave some functions (like `printf`) shared with many executable files outside,
and depends on some `.so` files to execute. Here, our `hello` depends on `libc.so`.
We can use `ldd` tool to check it.

```shell
$ ldd hello
    linux-vdso.so.1 (0x00007fb1dcbf4000)
    libc.so.6 => /usr/lib/libc.so.6 (0x00007fb1dc800000)
    /lib64/ld-linux-x86-64.so.2 => /usr/lib64/ld-linux-x86-64.so.2 (0x00007fb1dcbf6000)
```

Self contained doesn't free, the main purpose of dynamic linking is to reduce the size of executable file.
We can see the difference size of two types of linking:

```shell
$ ll hello hello_static 
-rwxr-xr-x 1 nakiri users  16K Jul  3 22:35 hello*
-rwxr-xr-x 1 nakiri users 824K Jul  3 22:35 hello_static*
```

Dynamic linking _really_ reduce a lot of size.
