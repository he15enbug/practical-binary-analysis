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
