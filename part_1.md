# Part 1: Binary Formats

[TOC]

## Chapter 1: Anatomy of A Binary

- The machine code that binary numberical systems execute is called *binary code*.
- Programs consist of binary code and data.
- Binary executable files, or *binaries*, are the self-contained files that store the code and data of programs.

### The C Compilation Process

- Binaries are produced through *compilation*, the process of translating source code into machine code that a processor can execute.
- Compiling C code involves 4 phases, one of which (awkwardly enough) is also called *compilation*. The phases are:
  1. Preprocessing
  2. Compilation
  3. Assembly
  4. Linking

    ```
                   +--------------+
    {file_i.c} --> |C preprocessor| <-- {header.h}
                   +--------------+
                            |
                +-----------+
                |
                |   +--------------+                    +-------------+
                +-> |C/C++ compiler| -----------------> |  Assembler  |
                    +--------------+                    +-------------+
                                                                  |
                    +------+                                      |
   {library.a} ---> |Linker| <----- {file_i.o} (Object files) <---+
                    +------+   
                        |
                        +----> a.out (Binary executable)               

    ```

- In practice, modern compilers often merge some or all of these phases.

#### The Preprocessing Phase

- C source files contain macros (`#define`) and `#include` directives.
- The preprocessing phase expands any `#define` and `#include` directives so all that's left is pure C code ready to be compiled.
- `gcc` automatically executes all phases, to see the output of the preprocessing phase, explicitly tell it to stop after preprocessing and show us the intermediate output: use `gcc -E -P` (`-E`: stop after preprocessing, `-P`: omit debugging information).

#### The Compilation Phase

- Translates the preprocessed code into assembly language.
- Most compilers also perform heavy optimization in this phase, typically cofigurable as an *optimization level* through command line switches such as `-O0` through `-O3` in `gcc`.
- For `gcc`, use `-S` to stop after the compilation phase (`.s` is a conventional extension for assembly files).
- For `gcc`, yse `-masm=intel` to emit assembly in Intel syntax rather than the default AT&T syntax.
- The symbols and functions will be preserved after this phase.

#### The Assembly Phase

- Finally get to generate some real machine code.
- Output: a set of *object files*, sometimes also referred to as *modules*. Object files contain machine instructions that are in principle executable by the processor.
- Typically, each source file corresponds to one assembly file, and each assembly file corresponds to one object file.
- Use `gcc -c example.c` to generate an object file.
- Relocatable files don't rely on being placed at any particular address in memory.
- Object files are compiled independently from each other, so the assembler has no way of knowing the memory addresses of other object files when assembling them. So, object files must be relocatable.

#### The Linking Phase

- Links together all the object files into a single binary executable.
- In modern systems, this phase sometimes incorporates an additional optimization pass, called *Link-Time Optimization (LTO)*.
- The program that performs this phase is called a *linker*, or *link editor*. It is typically separate from the compiler, which usually implements all the preceding phases
- References that rely on a relocation symbol are called *symbolic references*
- The linker's job is to take all the object files and merge them into a single coherent executable, typically intended to be loaded at a particular memory address. Now that the arrangement of all modules in the executable is known, the linker can also resolve most symbolic references. References to libraries may not be completely resolved, depending on the type of library.
- Static library (`.a` extension on Linux) are merged into the binary.
- There are also dynamic (shared) libraries, which are shared in memory among all programs that run on a system. During the linking phase, the addresses at which dynamic libs will reside are not yet known, so references to them cannot be resolved.
- Most compilers, including `gcc`, automatically call the linker at the end of the compilation process.
- A *dynamic linker* will be used to resolve the final dependencies on dynamic libraries when the executable is loaded into memory to be executed.

### Symbols and Stripped Binaries

- Compilers emit *symbols* that keep track of symbolic names in the source code and record which binary code and data correspond to each symbol. E.g., function symbols provide a mapping from symbolic function names to the first address and the size of each function.

#### Viewing Symbolic Information

- For ELF binaries, debugging symbols are typically generated in the DWARF format. DWARF information is usually embedded within the binary.
- For PE binaries, Microsoft Portable Debugging, PDB format. The PDB information comes in the form of a separate symbol file
- `readelf` can be used to parse symbols.
- we can programmatically parse symbols with a library like `libbfd`. There are also libs like `libdwarf` specifically designed for parsing DWARF debug symbols.

#### Another Binary Turns to the Dark Side: Stripping a Binary

- Use `strip` to strip a binary: `strip --strip-all a.out`

### Disassembling a Binary

#### Looking Inside an Object File

- Use a disassembler `objdump` to show how to do all the disassembling (Turns machine code into assembly language).
- In Intel syntax: `-M intel` 

#### Examining a Complete Binary Executable

#### Loading and Executing a Binary

- On-disk and in-memory binary may be different.
- To run a binary, the OS sets up a new process for the program to run in, including a virtual address space.
- Then, the OS maps an *interpreter* into the process's virtual memory. This is a user space program that knows how to load the binary and perform necessary relocations.
- On Linux, the interpreter is typically a shared library called `ld-linux.so`. On Windows, the interpreter functionality is implemented as part of `ntdll.dll`.
- After loading the interpreter, the kernel transfers control to it, and the interpreter begins its work in user space
- Linux ELF binaries come with a special section called `.interp` that specifies the path to the interpreter that is to be used to load the binary
- *Lazy binding*

### Summary

- Exercises

#### The ELF Format

- ELF is used for executable files, object files, shared libs, and core dumps.

## Chapter 2: The ELF Format

- **The Executable and Linkable Format, ELF**: the default binary format on Linux-based systems
- In essence, ELF binaries consist of 4 types of components:
  1. An *executable header*
  2. A series of (optional) *program headers*
  3. A number of *sections*
  4. A series of (optional) *section headers*, one per section

### The Executable Header

- To find out what the format of the executable header is, we can look up its type definition (and the definitions of other ELF-related types and cconstants) in `/usr/include/elf.h` or the ELF specification

#### The `e_ident` Array

- A 16-byte array.
- Use `readelf -h <FILE_NAME>` to inspect `e_ident` of an ELF binary.
- Starts with a 4-byte magic value identifying the file as an ELF binary (`0x7f, 'E', 'L', 'F'`).
- A number of bytes that give more info about the specifics of the type of ELF file. In `elf.h`, the indexes for these bytes (indexes 4 through 15 in the `e_ident` array) are symbolically referred to as `EI_CLASS`, `EI_DATA`, `EI_VERSION`, `EI_OSABI`, `EI_ABIVERSION`, and `EI_PAD`, respectively.

    ```
    +---------+ 0
    |   0x7f  |
    |---------| 1
    |   0x45  |     E
    |---------| 2
    |   0x4C  |     L
    |---------| 3
    |   0x46  |     F
    |---------| EI_CLASS      (4)
    |         |
    |---------| EI_DATA       (5)
    |         |
    |---------| EI_VERSION    (6)
    |         |
    |---------| EI_OSABI       (7)
    |         |
    |---------| EI_ABIVERSION (8)
    |         |
    |---------| EI_PAD        (9)
    |    0    |
    |    0    |
    |    0    |
    |    0    | Currently designated as padding (7 bytes)
    |    0    |
    |    0    |
    |    0    |
    +---------+ 15
    ```

- `EI_CLASS`: denotes whether the binary	is for a 32-bit (value: `ELFCLASS32` (`1`)) or 64-bit (value: `ELFCLASS64` (`2`)) architecture
- `EI_DATA`: indicates the endianness of the binary:
  - Little-endian: `ELFDATA2LSB` (`1`)
  - Big-endian: `ELFDATA2MSB` (`2`)
- `EI_VERSION`: indicates the version of the ELF spec. used when creating the binary. Currently, only valid value is `EV_CURRENT` (`1`)
- `EI_OSABI`: info regarding the ABI (application binary interface) and the OS. **Non-zero value**: some ABI- or OS-specific extensions are used in the ELF file, this can change the meaning of some other fields in the binary or can signal the presence of nonstandard sections. The default value of **zero**: the binary targets the UNIX system V ABI.
- `EI_ABIVERSION`: ABI version. Usually zero when the default `EI_OSABI` is used.
- `EI_PAD`: paddings consists of `0x00`.

#### The `e_type`, `e_machine`, and `e_version` Fields

- Multi-byte integer fields
- `e_type`: the type of the binary, the most common values are: 
  1. `ET_REL`: a relocatable object file
  2. `ET_EXEC`: an executable binary
  3. `ET_DYN`: a dynamic lib, aka. a *shared object file*
- `e_machine`: the architecture that the binary is intended to run on
  1. `EM_X86_64`
  2. `EM_386` (32-bit x86)
  3. `EM_ARM`
- `e_version` (4-byte): serves the same role as the `EI_VERSION` byte in `e_ident`. Specifically, it indicates the version of the ELF spec. that was used when creating the binary. Only possible value is 1 (`EV_CURRENT`)

#### The `e_entry` Field

- Denotes the *entry point* of the binary, this is the **virtual address** at which execution should start. Useful starting point for recursive disassembly.

#### The `e_phoff` and `e_shoff` Fields

- Only the executable header need to be located at a fixed location in an ELF binary.
- `e_phoff`: the *file offset* (i.e., *the number of bytes* we should read into the file to get to the headers) to the beginning of the program header table.
- `e_shoff`: the offset to the begining of the section header table.
- Set to `0`: the file doesn't contain a program header or section header table
- Note that these are *file offsets*, not virtual addresses

#### The `e_flags` Field

- provides room for flags specific to the arch. for which the binary is compiled
- E.g., ARM binaries intended to run on embedded platforms can set ARM-specific flags in the `e_flags` field to indicate additional details about the interface they expect from the embedded OS (file format conventions, stack organization, and so son). For x86 binaries, `e_flag` is typically set to zero

#### The `e_ehsize` Field

- The size (bytes) of the executable header. For x86 64 binaries, always 64. For x86 32 binaries, it's 52.

#### The `e_*entsize` and `e_*num` Fields

- `e_phentsize`, `e_shentsize`: the size of the individual program or section headers in each table
- `e_phnum`, `e_shnum`: Provide the number of program or section headers in each table

#### The `e_shstrndx` Field

- Contains the index (in the section header table) of the header associated with a special *string table* section, called `.shstrtab`. This is a dedicated section that contains a table of null-terminated ASCII strings, which store the names of all the sections in the binary.
- Show the content of `.shstrtab`: `readelf -x .shstrtab <ELF_FILE>`

### Section Headers

### Sections

#### Program Headers

