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

- `EI_CLASS`: denotes whether the binary is for a 32-bit (value: `ELFCLASS32` (`1`)) or 64-bit (value: `ELFCLASS64` (`2`)) architecture
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

- Sections do NOT have any predetermined structure
- Every section is described by a *section header*
- Section header table
- Sections are intended to provide a convenient organization for use by the linker. The section header table is an optional part of the ELF format
- Another logical organization: **segments**, which are used at execution time (as opposed to sections, which are used at link time)
- Format of an ELF section header as specified in `/usr/include/elf.h`

  ```c
  typedef struct elf64_shdr {
    Elf64_Word sh_name;        /* Section name, index in string tbl */
    Elf64_Word sh_type;        /* Type of section */
    Elf64_Xword sh_flags;        /* Miscellaneous section attributes */
    Elf64_Addr sh_addr;        /* Section virtual addr at execution */
    Elf64_Off sh_offset;        /* Section file offset */
    Elf64_Xword sh_size;        /* Size of section in bytes */
    Elf64_Word sh_link;        /* Index of another section */
    Elf64_Word sh_info;        /* Additional section information */
    Elf64_Xword sh_addralign;    /* Section alignment */
    Elf64_Xword sh_entsize;    /* Entry size if section holds table */
  } Elf64_Shdr;
  ```

#### `sh_name`

- If set, it contains an index into the *string table*
- If zero, the section doesn't have a name

#### `sh_type`

- Every section has a type
  - `SHT_PROGBITS`: sections with type `SHT_PROGBITS` contain program data, such as machine instructions or constants. No particular structure for the linker to parse
  - Types for symbol tables
    - `SHT_SYMTAB` for static symbol tables
    - `SHT_DYNSYM` for symbol tables used by the dynamic linker
    - Symbol tables contain symbols in a well-defined format (`struct Elf64_Sym` in `elf.h`), which describes the symbolic name and type for particular file offsets or addresses, among other things. The static symbol table may not be present if the binary is stripped, for example.
  - `SHT_STRTAB` for string tables
    - String tables simply contain an array of NULL-terminated strings, with the first byte in the string table set to NULL by convention
  - Sections with type `SHT_REL` or `SHT_RELA` are important for the linker (static linking purposes) b/c they contain relocation entries in a well-defined format (`struct Elf64_Rel` and `struct Elf64_Rela` in `elf.h`), which the linker can parse to perform the relocations in other sections
  - Sections with type `SHT_DYNAMIC` contain info for dynamic linking (`struct Elf64_Dyn` in `elf.h`)

#### `sh_flags`

- Section flags: additional info. Most important flags: `SHF_WRITE`, `SHF_ALLOC`, and `SHT_EXECINSTR`
- `SHF_WRITE`: section is **writable at runtime**
- `SHF_ALLOC`: contents of the section are to be loaded into virtual mem when executing the binary (though the actual loading happens using the segment view of the bin)
- `SHT_EXECINSTR`: section contains executable instructions

#### `sh_addr`, `sh_offset`, and `sh_size`

- Virtual address, file offset (in bytes), and size (in bytes) of the section, respectively
- When `sh_addr` is set to zero: this section is not intended to be loaded into virtual mem

#### `sh_link`

- Relationships between sections that the linker needs to know about. E.g., an `SHT_SYMTAB`, `SHT_DYNSYM`, or `SHT_DYNAMIC` has an associated string table section, which contains the symbolic names for the symbols in question. Similarly, relocation sections are associated with a symbol table describing the symbols involved in the relocations
- The `sh_link` field denotes the index in the section header table of the related section

#### `sh_info`

- Additional info. The meaning of the info varies depending on the section type. E.g., for reloc. sections, `sh_info` denotes the index of the section to which the relocations are to be applied

#### `sh_addralign`

- Some sections need to be aligned in mem for efficiency of mem access. E.g., loaded at some address that is a multiple of 8 or 16 bytes, set `sh_addralign` to 8 and 16, respectively
- Values 0 and 1: no special alignment needs

#### `sh_entsize`

- Some sections, such as symbol tables or relocation tables, contain a table of data structures. For such sections, the `sh_entsize` field indicates the size (in bytes) of each entry in the table
- Set to zero when not used

### Sections

- `readelf --sections --wide a.out`

#### The `.init` and `.fini` Sections

- `.init` contains executable code for initialization tasks, run before any other code. The system executes the code in `.init` before transferring control to the main entry point of the bin
- `.fini` is analogous to `.init`, except that it runs after main program completes

#### The `.text` Section

- Main code of the program resides. Has type `SHT_PROGBITS` b/c it contains user-defined code
- Executable, not writable. In general, executable sections should almost never be writable b/c that would make it vulnerable
- `.text` of a typical binary compiled by `gcc` contains a number of standard functions that perform initialization and finalization, such as `_start`, `register_tm_clones`, and `frame_dummy`
- `_start` will call `__libc_start_main` residing in `.plt` (which means the function is part of a shared lib). `__libc_start_main` will finally call to the address of `main` to begin execution of user-defined code.

#### The `.bss`, `.data` and `.rodata` Sections

- Variables are kept in one or more dedicated sections b/c code sections are generally not writable.
- Constant data is usually also kept in its own section (modern versions of `gcc` and `clang`), though some compilers do output constant data in code sections (Visual Studio sometimes does)
- `.rodata`: read-only data, not writable. 
- The default values of initialized variables are stored in `.data`, which is writable.
- `.bss` reserves space for uninitialized variables. The name historically stands for *block started by symbol*, referring of blocks of memory for symbolic variables.
- `.rodata` and `.data` have type `SHT_PROGBITS`, while `.bss` has type `SHT_NOBITS`, because `.bss` doesn't occupy any bytes in the bin as it exists on disk, it's simply a directive to allocate a properly sized block of mem for uninitialized vars when setting up an execution env for the bin
- Typically, vars in `.bss` are zero initialized, and the section is writable

#### Lazy Binding and the `.plt`, `.got`, and `.got.plt` Sections

##### Lazy Binding and the PLT

- On Linux, lazy binding is the default behavior of the dynamic linker. Force the linker to perform all relocs right away by exporting an env var `LD_BIND_NOW`.
- Lazy binding in Linux ELF bins is implemented with 2 sections: 
  1. The *Procedure Linkage Table* (`.plt`)
  2. The *Global Offset Table* (`.got`)
- `.got.plt` is a separate GOT section in ELF bins for use in conjunction w/ `.plt` in the lazy binding process
- `.plt` is a code section
- `.got.plt` is a data section
- The PLT consists entirely of stubs of a well-defined format, dedicated to directing calls from the `.text` section to the appropriate lib location.
- Disassemble the PLT of a binary `objdump -M intel --section .plt -d a.out`

##### Dynamic Resolving a Lib Function Using the PLT

- If we want to call the `puts` function in the `libc` library, instead of calling it directly, we can make a call to the corresponding PLT stub, `puts@plt`.
- The PLT stub begins with an indirect jump instr., which jumps to an addr. stored in the `.got.plt` section. Initially, before the lazy binding has happened, this address is simply the address of the next instr. in the function stub, which is a `push` instr. Thus, the indirect jump simply transfers control to the instr. directly after it. That's a rather roundabout way of getting to the next instr., but there's a good reason for that.
- The `push` instr. pushes an integer (in this case, 0x0) onto the stack. This integer serves as an identifier for the PLT stub in question. Subsequently, the next instr. jumps to the common default stub shared among all PLT function stubs
- The default stub pushes another identifier (taken from the GOT), identifying the executable itself, and then jumps (indirectly, again through the GOT) to the dynamic linker
- Using the identifiers pushed by the PLT stubs, the dynamic linker figures out that it should resolve the addr. of `puts` and should do so on behalf of the main executable loaded into the process. 
- The dynamic linker then looks up the addr. at which the `puts` is located and plugs the address of that func. into the GOT entry associated with `puts@plt`
- Thus, the GOT entry no longer points back into the PLT stub, but now points to the actual addr. of `puts`. At this point, lazying binding process is complete.
- Finally, the dynamic linker satisfies the original intention of calling `puts` by transferring control to it. For any subsequent calls to `puts@plt`, the GOT entry already contains the appropriate (pathed) addr. of `puts`, so that the jump at the start of the PLT stub goes directly yo `puts` w/o involving the dynamic linker.

##### Why Use a GOT

- Why not patch the resolved lib addr. directly into the code of the PLT stubs?
- One of the main reasons: **security**. Vulnerabilities in the bin would allow attackers to modify the code of the bin if executable sections like `.plt` were writable. With GOT, this attack model is a lot less powerful (the attacker can change the addresses in the GOT, but can not inject arbitrary code).
- The other reason has to do with **code shareability in shared libs**. Even though the OS loads only a single physical copy of each lib, the same lib will likely be mapped to a completely different virtual address for each process. The implication is that we cannot patch addresses resolved on behalf of a lib directly into the code because the addr. would **work only in the context of one process** and break the others. Patching them into the GOT instead does work b/c **each process has its own private copy of the GOT**. (This part is not so make much sense to me b/c it doesn't clarify the shareability of the PLT, I'll try to figure it out at a later point)
- References from the code to relocatable data symbols (such as vars and constants exported from shared libs) also need to be redirected via the GOT to avoid patching data addresses directly into the code. The difference is that data references go directly through the GOT, w/o the intermediate step of the PLT. This also clarifies the destinction between the `.got` and `.got.plt` sections:
  - `.got` is for refs to data items.
  - `.got.plt` is for storing resolved addresses for lib functions accessed via the PLT.

#### The `.rela.*` Sections

- The `.rela.*` sections, with type `SHT_RELA`, contain info used by the linker for performing relocations
  - Each section of type `SHT_RELA` is a table of relocation entries, with each entry detailing a particular addr. where a reloc. needs to be applied, as well as instructions on how to resolve the particular value that needs to be plugged in at this addr.
- Example types of relocs
  - `R_X86_64_GLOB_DAT`: has its offset in `.got`. Generally, this type of reloc. is used to compute the address of a data symbol and plug it into the correct offset in `.got`.
  - `R_X86_64_JUMP_SLO`: this type of entries are called *jump slots*, which have their offset in the `.got.plt` section and represent slots where the addr. of lib funcs can be plugged in. Each of these slots is used by one of the PLT stubs to retrieve its indirect jump target.
  - For more, refer to the ELF specification.
  - All reloc. types specify an offset at which to apply the reloc. How to compute the value to plug in at that offset differ per reloc. type.

#### The `.dynamic` Section

- A road map for the OS and dynamic linker when loading and setting up an ELF bin for execution.
- Contains a table of `Elf64_Dyn` structures (in `/usr/include/elf.h`), also referred to as *tags*. There are different types of tags, each of which comes with an associated value

    ```c
    typedef struct {
        Elf64_Sxword d_tag;        /* entry tag value */
        union {
            Elf64_Xword d_val;
            Elf64_Addr d_ptr;
        } d_un;
    } Elf64_Dyn;
    ```

- `readelf --dynamic a.out`
- Example types
  - `DT_NEEDED`: info about dependencies of the executable, e.g., the example binary uses the `puts` function from the `libc.so.6` shared library, so it needs to be loaded when executing the binary.
  - `DT_VERNEED` and `DT_VERNEEDNUM` tags specify the starting address and number of entries of the *version dependency table*, which indicates the expected version of the various dependencies of the executable.
- In addition to listing dependencies, the `.dynamic` section also contains pointers to other info required by the dynamic linker (e.g., the dynamic string table, dynamic symbol table, `.got.plt` section, and dynamic relocation section pointed to by tags of type `DT_STRTAB`, `DT_SYMTAB`, `DT_PLTGOT`, and `DT_RELA`, respectively).


#### The `.init_array` and `.fini_array` Sections

- `.init_array`: an array of pointers to functions to use as constructors. Each of these funcs is called in turn when the bin is initialized, before `main` is called
  - While the aforementioned `.init` contains a **single** startup function that performs some crucial init, `.init_array` is a **data section** that can contain as many function pointers as we want, including pointers to our own custom constructors. (In `gcc`, make functions in C source files as constructors by decorating them with `__attribute__((constructor))`).
- `.fini_array` is analogous to `.init_array`, except that `.fini_array` contains pointers to destructors.
- `.init_array` and `.fini_array` are convenient places to insert hooks that add initialization or finalization code to the binary to modify its behavior.
- Note that bins produced by older `gcc` versions may contain sections called `.ctors` and `.dtors` instead of `.init_array` and `.fini_array`.

#### The `.shstrtab`, `.symtab`, `.strtab`, `.dynsym`, and `.dynstr` Sections

- `.shstrtab`: an array of NULL-terminated strings that contains the names of all sections. It's indexed by the section headers to allow tools like `readelf` to find out the names of the sections
- `.symtab`: a symbol table (of `Elf64_Sym` structures), each of which assocates a symbolic name with a piece of code or data elsewhere in the binary, such as a function or a variable. The actual strings containing the symbolic names are located in the `.strtab` section. In stripped binaries, `.symtab` and `.strtab` are removed.
- `.dynsym` and `.dynstr` are analogous to `.symtab` and `.strtab`, except that they contain symbols and strings needed for dynamic linking rather than static linking. Cannot be stripped.

### Program Headers

- The *program header table* provides a **seg,emt view** of the binary, as opposed to the section view provided by the section header table.
- An ELF segment encompasses zero or more sections, bundling these into a single chunk. The program header table encodes the segment view using program headers of type `struct Elf64_Phdr`

    ```c
    typedef struct {
        uint32_t p_type;      /* Segment type */
        uint32_t p_flags;     /* Segment flags */
        uint64_t p_offset;        /* Segment file offset */
        uint64_t p_vaddr;        /* Segment virtual address */

        uint64_t p_paddr;        /* Segment physical address */
        uint64_t p_filesz;        /* Segment size in file */
        uint64_t p_memsz;        /* Segment size in memory */
        uint64_t p_align;        /* Segment alignment, file & memory */
    } Elf64_Phdr;
    ```

- Show program headers and section to segment mapping: `readelf --wide --segments a.out`

#### `p_type`

- `p_type` identifies the type of the segment. Important values:
  - `PT_LOAD`: segments that are intended to be loaded into memory when setting up the process. The *size* of the loadable chunk and the *address* to load it at are described in the rest of the program header. Usually at least 2 `PT_LOAD` segments, one encompassing the nonwritable sections and one containing the writable data sections.
  - `PT_DYNAMIC`: contains the `.dynamic` section, which tells the interpreter how to parse and prepare the bin for execution.
  - `PT_INTERP`: contains the `.interp` section, which provides the name of interpreter that is to be used to load the binary.
  - `PT_PHDR`: encompasses the program header table.

#### `p_flags`

- The flags specify the **runtime access permissions** for the segment. Three important types of flags exist: 
  - `PF_X` (executable, set for code segments, `readelf` display it as an `E` rather than `X`)
  - `PF_W` (writable, normally set only for writable data segments)
  - `PF_R` (readable)

#### `p_offset`, `p_vaddr`, `p_paddr`, `p_filesz`, and `p_memsz`

- The `p_offset`, `p_vaddr`, and `p_filesz` fields are analogous to the `sh_{offset, addr, size}` fields in a section header. They specify the *file offset* at which the segment starts, the *virtual address* at which it is to be loaded, and the *file size* of the segment, respectively. For loadable segments, `p_addr` must be equal to `p_offset`, modulo the *page size* (typically 4096 bytes).
- On some systems, use `p_paddr` to specify at which address in **physical memory** to load the segment. On modern OS such as Linux, this field is unused and set to zero since they execute all binaries in virtual memory.
- `p_filesz` and `p_memsz` are different: some sections only indicate the need to allocate some bytes in memory, but don't actually occupy these bytes in the bin file. E.g., `.bss` section contains zero-initialized data. Since all data in this section is known to be zero, there's no need to actually include all these zeros in the file. However, when loading the segment containing `.bss` into virtual memory, all the bytes in `.bss` should be allocated. Thus, it's possible for `p_memsz` to be larger than `p_filesz`. When this happens, the loader adds the extra bytes **at the end of the segment** when loading and zero-initializing.

#### `p_align`

- This field is analogous to `sh_addralign` in a section header. An alignment value of 0 or 1 indicates that no alignment is required. If not 0 or 1, the value must be a power of 2, and `p_vaddr` must be equal to `p_offset`, modulo `p_align`.

### Summary

- Exercises

## Chapter 3: The PE Format: A Brief Introduction

- The *Portable Executable* (PE) format
- Main binary format used on Windows
- PE is a modified version of the *Common Object File Format* (COFF), which was also used on Unix-based systems before being replaced by ELF. For this historical reason, PE is sometimes also referred to as PE/COFF
- Confusingly, the 64-bit version of PE is called PE32+, because PE32+ has only minor differences compared to the original PE format
- Refer to `WinNT.h`, which is included in the Microsoft Windows Software Developer Kit

### The MS-DOS Header and MS-DOS Stub

- One of the main differences to ELF is the presence of an MS-DOS header. For backward compatibility.
- With an MS-DOS header, a PE binary can also be interpreted as an MS-DOS binary, at least in a limited sense.
- The main function of the MS-DOS header is to describe how to load and execute an MS-DOS stub, which comes right after the MS-DOS header. This stub is usually just a small MS-DOS program, which is run instead of the main program when the user executes a PE binary in MSDOS.
- tHE MS-DOS stub program typically prints a string like "This program cannot be run in DOS mode" and then exits. However, in principle, it can be a full-fledged MS-DOS version of the program!
- The MS-DOS header starts with a magic value, which consists of the ASCII characters "MZ". For this reason, it is also sometimes referred to as an MZ header.
- Another important field in the header is the last field, `e_lfanew`. It contains the file offset at which the real PE binary begins. Thus, when a PE-aware program loader opens the binary, it can read the MS-DOS header and then skip past it and the MS-DOS stub to go right to the start of the PE headers.

### The PE Signature, File Header, and Optional Header

- We can consider the PE headers analogous to ELF's executable header, except that in PE, the "executable header" is split into three parts: a 32-bit signature, a PE file header, and a PE optional header.
- A `struct` called `IMAGE_NT_HEADERS64` encompasses all three of these parts.

#### The PE Signature

- Simply a string containing the ASCII characters "PE", followed by 2 NULL characters (zero bytes). It's analogous to the magic bytes in the `e_ident` field in ELF's executable header.

#### The PE File Header

- The most important fields: 
  - `Machine` (use value of `0x8664` for x86-64)
  - `NumberOfSections` (# of headers in the section header table)
  - `SizeOfOptionalHeader`
  - `Characteristics` (contains flags describing things such as the endianness of the bin, whether it is a DLL, and whether it has been stripped)
- The two fields describing the symbol table are deprecated, and PE files should no longer make use of embedded symbols and debugging info. Instead, these symbols are optionally emitted as part of a separate debugging file.

#### The PE Optional Header

- Despite the name suggests, the PE optional header is not really optional for executables (though it might be missing in object files)
- Most important fields:
  - A 16-bit (2 bytes) magic value `0x020b` for 64-bit PE files
  - Several fields describing the major and minor version number of the linker, and the minimal OS version needed to run the binary
  - `ImageBase`: describes the address at which to load the binary (PE binaries are designed to be loaded at a specific virtual address)
  - Other pointer fields contain *relative virtual addresses (RAVs)*, which are intended to be added to the base address. E.g., the `BaseOfCode` field specifies the base address of the code sections (**as an RVA**). So, we can find the base virtual address of the code sections by computing `ImageBase + BaseOfCode`
  - `AddressOfEntryPoint`: the entry point address of the bin, also specified as an RVA
  - The `DataDirectory` array: contains entries of a `struct` type called `IMAGE_DATA_DIRECTORY`, which contains an RVA and a size. Every entry in the array descirbes the starting RVA and size of an important portion of the bin. 
    - The precise interpretation of the entry depends on its index in the array. 
      - The most important entries are the one at index 0, which describes the base RVA and size of the *export directory* (basically a table of exported functions)
      - The entry at index 1 describes the *import directory*
      - The entry at index 5 describes the relocaton table
    - `DataDirectory` serves as a shortcut for the loader, allowing it to quickly look up particular portions of data without having to iterate through the section header table.

### The Section Header Table

- Analogous to ELF's section header table
- An array of `IMAGE_SECTION_HEADER` structures, each of which describes a single section, denoting the size in the file and in memory (`SizeOfRawData` and `VirtualSize`), its file offset and virtual address (`PointerToRawData` and `VirtualAddress`), relocation info, and any flags (`Characteristics`). Among other things, the flags describe whether the section is executable, readable, writable, or some combination of these. Instead of referring to a string table as ELF section headers do, PE section headers specify the section name using a simple character array field, aptly called `Name`. Because the array is only 8 bytes long, PE section names are limited to 8 characters
- Unlike ELF, the PE format doesn't explicitly distinguish between sections and segments. The closest thing PE files have to ELF's execution view is the `DataDirectory`, which provides the loader with a shortcut to certain portions of the binary needed for setting up the execution. Other than that, there is no separate program header table. The section header table is used for both linking and loading.

### Sections

- Many of the sections are directly comparable to ELF sections, often having (almost) the same name
  - `.text`: code
  - `.data`: r/w data
  - `.rdata`: read-only data. **Note that PE compilers like Visual Studio sometimes place read-only data in the `.text` section (mixed in with the code) instead of in .rdata.** Can be problematic during disassembly, b/c it makes it possible to accidentally interpret constant data as instructions
  - `.bss`: zero-initialized data
  - `.reloc`: relocation information

#### `.edata` and `.idata`

- The most important sections, with no direct equivalent in ELF
- Contain tables of exported and imported functions, respectively
  - `.idata`: symbols (functions and data) the binary imports from shared libs, or DLLs in Windows terminology
  - `.edata`: symbols and addresses that the bin exports
- The export directory and import directory entries in the `DataDirectory` array refer to these sections
- To resolve references to external symbols, the loader needs to match up the required imports with the export table of the DLL that provides the required symbols
- When `.edata` and `.idata` are not present in the binary, they're usually merged into `.rdata`
- When the loader resolves dependencies, it writes the resolved addresses into the *Import Address Table (IAT)*. Similarly to the GOT in ELF, the IAT is simply a table of resolved pointers with one slot per pointer. The IAT is also part of the `.idata` section, and it initially contains pointers to the names or identifying numbers of the symbols to be imported. The dynamic loader then replaces these pointers with pointers to the actual imported functions or variables. A call to a lib function is then implemented as a call to a *thunk* for that function, which is nothing more than an indirect jump through the IAT slot for the function

#### Padding in PE Code Section

- When disassembling PE files, there are lots of `int3` instructions. VS emits these instructions as padding (instead of the `nop` instruction used by `gcc`) to align functions and blocks of code in memory such that they can be accessed efficiently
- The `int3` instruction is normally used by debuggers to set breakpoints, it causes the program to trap to the debugger or to crash if no debugger is present. This is okay for padding code since padding instructions are not intended to be executed

### Summary

- Exercises

## Chapter 4: Building A Binary Loader Using `libbfd`

### What Is `libbfd`

- **The Binary File Descriptor library (`libbfd`)** provides a common interface for reading and parsing all popular binary formats, compiled for a wide variety of architectures. This includes ELF and PE files for x86 and x86-64 machines
- `libbfd` is part of the `binutils-dev` package
- The core `libbfd` API is in `/usr/include/bfd.h`

### A Simple Binary-Loading Interface

- Intended for use in static analysis tools, different from the dynamic loader provided by the OS
- The C++ header file describing the basic API that the binary loader will expose (in `~/code/inc/loader.h`)

    ```cpp
    #ifndef LOADER_H
    #define LOADER_H

    #include <stdint.h>
    #include <string>
    #include <vector>

    class Binary;
    class Section;
    class Symbol;

    class Symbol {
        public:
            enum SymbolType {
                SYM_TYPE_UKN = 0,
                SYM_TYPE_FUNC = 1
            };

            Symbol() : type(SYM_TYPE_UKN), name(), addr(0) {}

            SymbolType type;
            std::string name;
            uint64_t addr;
    };

    class Section {
        public:
            enum SectionType {
                SEC_TYPE_NONE = 0,
                SEC_TYPE_CODE = 1,
                SEC_TYPE_DATA = 2
            };

            Section() : binary(NULL), type(SEC_TYPE_NONE), 
                        vma(0), size(0), bytes(NULL) {}

            bool contains(uint64_t addr) {
                return (addr >= vma) && (addr < vma + size);
            }

            Binary *binary;
            std::string name;
            SectionType type;
            uint64_t vma;
            uint64_t size;
            uint8_t *bytes;
    };

    class Binary {
        public:
            enum BinaryType {
                BIN_TYPE_AUTO = 0,
                BIN_TYPE_ELF  = 1,
                BIN_TYPE_PE   = 2
            };

            enum BinaryArch {
                ARCH_NONE = 0,
                ARCH_X86  = 1
            };

            Binary() : type(BIN_TYPE_AUTO), arch(ARCH_NONE), bits(0), entry(0) {}

            Section *get_text_section() {
                for(auto &s : sections) {
                    if(s.name == ".text") {
                        return &s;
                    }
                    return NULL;
                }
            }

            std::string filename;
            BinaryType type;
            std::string type_str;
            BinaryArch arch;
            std::string arch_str;
            unsigned bits;
            uint64_t entry;
            
            std::vector<Section> sections;
            std::vector<Symbol>  symbols;
    };

    int load_binary(std::string &fname, Binary *bin, Binary::BinaryType type);
    void unload_binary(Binary *bin);
    
    #endif  /* LOADER_H */
    ```

- The `Binary` class is the "root" class, representing an abstraction of the entire binary. It contains a `vector` of `Section` objects and a `vector` of `Symbol` objects.
- The whole API centers around two functions:
  1. `load_binary` takes the name of a binary file to load (`fname`), a pointer to a `Binary` object to contain the loaded binary (`bin`), and a descriptor of the binary type `type`. It loads the requested binary into the `bin` parameter and returns an integer value of 0 if the loading process was successful or a value less than 0 if it was not successful.
  2. `unload_binary` simply takes a pointer to a previously loaded `Binary` object and unloads it.

#### The `Binary` Class

- Contains the binary's filename, type, architecture, bit width, entry point address, and sections and symbols
- The binary type has a dual representation: the `type` member contains a numeric type identifier, while `type_str` contains a string representation of the binary type. The same kind of dual representation is used for architecture
- `BIN_TYPE_AUTO`: pass to the `load_binary` function to ask it to automatically determine whether the binary is an ELF or PE file.
- `ARCH_X86`: includes both x86 and x86-64. The distinction between the two is made by the `bits` member of the `Binary` class, which is set to 32 bits for x86 and to 64 bits for x86-64
- `get_text_section`: iterates over the `sections` vector to look up and return the `.text` section

#### The `Section` Class

- A simple wrapper around the main props of a section (name, type, starting addr. (`vma`), size in bytes, and raw bytes contained in the section)
- For convenience, there is also a pointer back to the `Binary` that contains the `Section` object
- `contains` function: takes a code or data address and returns a `bool` indicating whether the address is part of the section

#### The `Symbol` Class

- Binaries contain symbols for many types of components, including local and global variables, functions, relocation expressions, objects, and more. To keep things simple, the loader interface exposes only one kind of symbol: function symbols. These are especially useful because they enable us to easily implement function-level binary analysis tools when function symbols are available
- The `Symbol` class contains symbol type (`enum SymbolType`, the only valid value of `SYM_TYPE_FUNC`). In addition, the class contains the symbolic name and the start address of the function described by the symbol

### Implementing the Binary Loader

- Use `libbfd` to implement the binary loader
- The `libbfd` functions all start with `bfd_` (there are also functions that end with `_bfd`, but they are functions defined by the loader). All programs that use `libbfd` must include `bfd.h`, and link against `libbfd` by specifying the linker flag `-lbfd`. In addition to `bfd.h`, the loader includes the header file that contains the interface created in the previous section
- Load and unload functions `inc/loader.cc` (continued)

    ```cpp

    int load_binary(std::string &fname, Binary *bin, Binary::BinaryType type) {
        return load_binary_bfd(fname, bin, type);
    }

    void unload_binary(Binary *bin) {
        size_t i;
        Section *sec;

        for(i = 0; i < bin->sections.size(); i++) {
            sec = &bin->sections[i];
            if(sec->bytes) {
                free(sec->bytes);
            }
        }
    }

    ```

- Only `bytes` member of each `Section` is allocated dynamically (using `malloc`)

#### Initializing `libbfd` and Opening a Binary

- The code to open a binary (`open_bfd`), `inc/loader.cc` (continued)

    ```cpp

    static bfd* open_bfd(std::string &fname) {
        static int bfd_inited = 0;
        bdf *bfd_h;

        if(!bfd_inited) {
            bfd_init();
            bfd_inited = 1;
        }

        bfd_h = bfd_openr(fname.c_str(), NULL);

        if(!bfd_h) {
            fprintf(stderr, "failed to open binary '%s' (%s)\n", fname.c_str(), bfd_errmsg(bfd_get_error()));
            return NULL;
        }

        if(!bfd_check_format(bfd_h, bfd_object)) {
            fprintf(stderr, "file '%s' does not look like an executable (%s)\n", fname.c_str(), bfd_errmsg(bfd_get_error()));
            return NULL;
        }

        /* Some versions of bfd_check_format pessimistially set a wrong_format 
         * error before detecting the format and then neglect to unset it once
         * the format has been detected. We unset it manually to prevent problems
         */
        bfd_set_error(bfd_error_no_error);

        if(bfd_get_flavour(bfd_h) == bfd_target_unknown_flavour) {
            fprintf(stderr, "unrecognized format for binary '%s' (%s)\n", fname.c_str(), bfd_errmsg(bfd_get_error()));
            return NULL;
        }

        return bfd_h;
    }

    ```

- The `open_bfd` function uses `libbfd` to determine the properties of the binary specified by `fname`, open it, and then return a handle to the binary. Before using `libbfd`, we must call `bfd_init` to initialize `libbfd`'s internal state. This needs to be done only once, so `open_bfd` uses a static variable to keep track of whether the init. has been done already
- Call `bfd_openr` to open the binary by filename. The second parameter of `bfd_openr` allows us to specify a target (the type of the binary), leaving it to NULL makes `libbfd` to automatically determine the binary type. `bfd_openr` returns a pointer to a file handle of type `bfd`. This is `libbfd`'s root data structure, which we can pass to all other function in `libbfd` to perform operations on the binary. In case of error, `bfd_openr` returns NULL
- When an error occurs, we can find the type of the most recent error by calling `bfd_get_error`, which returns a `bfd_error_type` object. We can compare it agains predefined error identifiers such as `bfd_error_no_memory` or `bfd_error_invalid_target`

#### Parsing Basic Binary Properties

- `load_binary_bfd` function (in `inc/loader.cc` (continuted))

    ```cpp
    static int load_binary_bfd(std::string &fname, Binary *bin, Binary::BinaryType type) {
        int ret;
        bfd *bfd_h;
        const bfd_arch_info_type *bfd_info;

        bfd_h = NULL;
        
        /* Open the binary and get a bfd handle to it */
        bfd_h = open_bfd(fname);
        if(!bfd_h) {
            goto fail;
        }

        bin->filename = std::string(fname);
        /* Get the entry point address, which is a bfd_vma (a 64-bit unsigned integer) */
        bin->entry    = bfd_get_start_address(bfd_h);

        bin->type_str = std::string(bfd_h->xvec->name);
        switch(bfd_h->xvec->flavour) {
            case bfd_target_elf_flavour:
                bin->type = Binary::BIN_TYPE_ELF;
                break;
            case bfd_target_coff_flavour:
                bin->type = Binary::BIN_TYPE_PE;
                break;
            case bfd_target_unknown_flavour:
            default:
                fprint(stderr, "unsupported binary type (%s)\n", bfd_h->xvec->name);
                goto fail;
        }

        /* Get the architecture */
        bfd_info = bfd_get_arch_info(bfd_h);
        bin->arch_str = std::string(bfd_info->printable_name);
        switch(bfd_info->mach) {
            case bfd_mach_i386_i386:
                bin->arch = Binary::ARCH_X86;
                bin->bits = 32;
                break;
            case bfd_mach_x86_64:
                bin->arch = Binary::ARCH_X86;
                bin->bits = 64;
                break;
            default:
                fprint(stderr, "unsupported architecture (%s)\n", bfd_info->printable_name);
                goto fail;
        }

        /* Symbol handling is best-effort only (they may not even be present) */
        load_symbols_bfd(bfd_h, bin);
        load_dynsym_bfd(bfd_h, bin);

        if(load_sections_bfd(bfd_h, bin) < 0) {
            goto fail;
        }

        ret = 0;
        goto cleanup;

        fail:
        return -1;

        cleanup:
        if(bfd_h) {
            bfd_close(bfd_h);
        }

        return ret;
    }
    ```

#### Loading Symbols



#### Loading Sections
