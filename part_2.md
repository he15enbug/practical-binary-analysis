# Part 2: Binary Analysis Fundamentals

## Chapter 5: Basic Binary Analysis In Linux

- Fundamental tools we'll need to perform binary analysis on Linux
- Learning by solving CTF challenges

### Resolving Identity Crises Using `file`

- There is no hints about the content of `payload`, a good first step is to figure out what we can about the file type and its contents. `file` utility was designed for this purpose. `file` isn't fooled by extensions, it searches for other telltale patterns in the file, such as magic bytes like the `0x7f E L F` sequence at the start of ELF files, to find out the file type.

    ```shell
    $ file payload
    payload: ASCII text
    ```

- `payload` contains ASCII text, use `head` or `tail` utility to examine the text in detail. (`head` shows the first few lines, and `tail` shows the last few lines)
- The content in `payload` is not human-readable, but it consists of only alphanumeric characters and `+` and `/`, it's usually safe to assume file that looks like this is a *Base64* file
- *Base64* is a widely used method of encoding binary data as ASCII text. Commonly used in email and on the web to ensure that binary data transmitted over a network isn't accidentally malformed by services that can handle only text
- Use `base64` on Linux (typically as part of GNU `coreutils`) to encode and decode *Base64*
- `base64 -d payload decoded_payload`. Then, we can `file decoded_payload` to check its type, it is `gzip compressed data`
- Another feature of `file` is to see what is inside zipped files without extracting it using `-z` option: `file -z decoded_payload`. We can see there is a `tar` archieve inside. To extract a `.tar.gz` file, use `tar` with `-xvzf` options. (`-z` for `gzip`, `-x` to extract files from the archieve, `-v` verbose mode, `-f` to specify the file name of the archieve)
- We extracted 2 files, an ELF file (`ctf`), and a bitmap (BMP) file of `512 * 512` pixels (`67b8601`). `67b8601` depicts a black square, with some irregularly colored pixels at the bottom of the figure

### Using `ldd` to Explore Dependencies

- It's not wise to run unknown binaries, but we are in a VM. Try to run it, we get an error caused by a missing shared object file `lib5ae9b7f.so`
- Check whether `ctf` has any more unresolved dependencies using `ldd`. We can see that `lib5ae9b7f.so` is the only missing library
- This lib is not in any standard repository, it must reside somewhere in the files we've been given so far. Since all ELF binaries and libs begin with the magic sequence `\x7fELF` (4 bytes), and the library is not encrypted, we should be able to find the ELF header this way
- `grep` for the string `ELF`: 
    
    ```shell
    $ grep ELF . -rl
    ./67b8601
    ./levels.db
    ./ctf
    ./oracle
    ```

- The "BMP" file `67b8601` also contains the `ELF` string, there might be a shared lib hidden within it.

### Viewing File Contents with `xxd`

- To analyze the file at the byte level, use `xxd`
- `xxd 67b8601 | head -n 15` to display the first 15 lines of the output of `xxd`. We can use `-s` (seek) option to specify a file offset at which to start. `-l` (length) option to specify the number of bytes to dump
- The magic bytes `\x7fELF` appear at offset `0x34`, this is where the suspected ELF library begins. Finding out where is ends is not trivial. It is easier to extract only the ELF header, which is 64 bytes for 64-bit ELF headers. Then we can figure out the length of the complete ELF file by examining the header
- `dd skip=52 count=64 if=67b8601 of=elf_header bs=1`: `bs` specifies the block size, `skip` specifies how many **blocks** to skip, and `count` specifies how many **blocks** to copy

### Parsing the Extracted ELF with `readelf`

- `readelf -h elf_header`
- The last part of an ELF file is typically the section header table and the offset to the section header table is given in the executable header (`8568` bytes). The executable header also tells us the size of each section header (`64` bytes) and the number of section headers in the table (`27`). The size of the complete ELF lib is `10296` bytes
- Get the lib: `dd skip=52 count=10296 if=67b8601 of=lib5ae9b7f.so bs=1`

### Parsing Symbols with `nm`

- C++ allows functions to be overloaded, but the linker doesn't know anything about C++. E.g., if there are multiple functions with the name `foo`, the linker has no idea how to resolve references to `foo`. To eliminate duplicate names, C++ compilers emit *mangled* function names. A mangled name is essentially a combination of the original function name and an encoding of its parameters
- Mangled names are relatively easy to *demangle*. We can use `nm`, which lists symbols in a given binary, object file, or shared object
- `nm -D <FILE_NAME>`: parse the dynamic symbol table
- Demangle symbol names: `nm -D --demangle lib5ae9b7f.so`
- The function names appear human-readable. Some interesting functions, which appear to be cryptographic functions implementing the **RC4** encryption algorithm

    ```cpp
    rc4_decrypt(rc4_state_t*, unsigned char*, int)
    rc4_decrypt(
        rc4_state_t*, 
        std::__cxx11::basic_string<
            char, 
            std::char_traits<char>, 
            std::allocator<char> >&)
    rc4_encrypt(rc4_state_t*, unsigned char*, int)
    rc4_encrypt(
        rc4_state_t*, 
        std::__cxx11::basic_string<
            char, 
            std::char_traits<char>, 
            std::allocator<char> >&)
    rc4_init(rc4_state_t*, unsigned char*, int)
    ```

- The first parameter of `rc4_init` is presumably a data structure where the cryptographic state is kept, while the next two are probably a string representing a key and an integer specifying the length of the key
- An alternative way of demangling function names is to use a specialized utility called `c++filt`, which takes a mangled name as the input and outputs the demangled equivalent. `c++filt` supports several mangling formats and automatically detects the correct mangling format for the given input
- Run `ctf`. When running a binary, the linker resolves dependencies by searching a number of standard directories such as `/lib`. Since we extracted `lib5ae9b7f.so` to a non-standard directory, we need to tell the linker to search that directory too by setting an env variable called `LD_LIBRARY_PATH`: 

    ```shell
    $ export LD_LIBRARY_PATH=`pwd`
    $ ./ctf
    $ echo $?
    1
    ```

- The exit status of `ctf` contained in the `$?` variable is 1, indicating an error. Now we need to bypass the error to reach the flag we're trying to capture

### Looking for Hints with `strings`

- Use `strings` to check for strings in a binary (or any other file) on Linux
- By default, `strings` output all printable strings of 4 characters or more in the given files
- Options: 
  - `-d`: print only strings in data sections in a binary
  - `-n`: specify the minimum string length (that will be printed out, the default value is 4)
- Strings that might be useful

    ```
    ...
    DEBUG: argv[1] = %s
    checking '%s'
    show_me_the_flag
    ...
    It's kinda like Louisiana. Or Dagobah. Dagobah - Where Yoda lives!
    ...
    ```

- From the debug message, we can know that the binary may expect a command line option, and then it performs some sort of checks on an input string. Try any arbitrary string: we still get an error

    ```shell
    $ ./ctf hello
    checking 'hello'
    $ echo $?
    1
    $ ./ctf show_me_the_flag
    checking 'show_me_the_flag'
    ok
    $ echo $?
    1
    ```

- When we use `show_me_the_flag`, the check appears to succeed as it prints out `ok`, but the exit status is still 1, there must be something else missing. To take a more detail look at `ctf`'s behavior, trace the system and lib calls it makes

### Tracing System Calls and Library Calls with `strace` and `ltrace`

- Investigate the reason that `ctf` exits with an error code by looking at its behavior just before exits. Use `strace` and `ltrace`, which show the system calls and library calls, respectively, executed by a binary.
- In some cases, we need to attach `strace` to a running process, use `-p <PID>` option.
- `strace ./ctf show_me_the_flag`
- When tracing a program from the start, `strace` includes all the system calls used by the program interpreter to set up the process, making the ouput quite verbose
- The first system call in the output is `execve`, which is called by the shell to launch the program
- After that, the program interpreter takes over and starts setting up the execution env. This involves setting up memory regions and setting the correct memory access permissions using `mprotect`. Additionally, there are system called used to look up and load the required dynamic libs
- Since we set `LD_LIBRARY_PATH` to the current working directory, the dynamic linker will search for the `lib5ae9b7f.so` in a number of **standard subfolders in our current working directory** util it finally finds the lib in the root of the current working directory. Then, the dynamic linker reads it and maps it into memory. The setup process is repeated for other required libs
- The last 3 sys calls are application-specific behavior. The first sys call used by `ctf` itself is `write`, which is used to print `checking 'show_me_the_flag'`. Another `write`, which prints `ok`. Finally, there's a call to `exit_group`, which leads to the exit with status code 1
- In this case, `strace` didn't reveal anything helpful. Try library calls
- Use `ltrace` with `-i` option to print the instruction pointer at every lib call, `-C` option to automatically demangle C++ function names: `ltrace -i -C ./ctf show_me_the_flag`
- We can see something new: the RC4 cryptography is initialized through a call to `rc4_init`. After that, there is an `assign` to a C++ string, presumably initializing it with an encrypted message. This message is then decrypted with a call to `rc4_decrypt`, and the decrypted message is assigned to a new C++ string. Finally, there's a call to `getenv`, which is a standard lib function used to look up env variables. `ctf` expects an env variable called `GUESSME`. The name of this var may well be the string that was decrypted earlier. Try a dummy value for this env variable

    ```shell
    $ GUESSME='hello' ./ctf show_me_the_flag
    checking 'show_me_the_flag'
    ok
    guess again!    
    ```

- It seems that `ctf` expects `GUESSME` to be set to another specific value. Use another `ltrace`
- After calling `getenv`, `ctf` goes on to `assign` and `decrypt` another C++ string. Unfortunately, between the decryption and the moment that `guess again` is printed, we don't see any hints regarding the expected value of `GUESSME`. This tells us the comparison of `GUESSME` to its expected value is implemented without the use of any lib functions. We'll need to examining instruction-level behavior of `ctf`

### Examining Instruction-Level Behavior Using `objdump`

- Since we know that the value of the `GUESSME` env variable is checked without using any lib functions, a logical next step is to use `objdump` to examine `ctf` at the instruction level to find out what's going on
- From the `ltrace` output, we know that `guess again` string is printed by a call to `puts` at address `0x400dd7`, focus the `objdump` investigation around this address. It will also help to know the address of the string to find the first instruction that loads it, look at the `.rodata` section using `objdump -s --section .rodata ctf` (`-s`: display full contents of sections requested)
- Find the address of the string `guess again` by looking at the `.rodata` section: `objdump -s --section .rodata ctf`. The address is `0x4011af`
- Now, look at the instructions around the `puts` call 

    ```
    $ objdump -d ctf
    400dc0: movzx edx, BYTE PTR [rbx+rax*1]
    400dc4: test dl, dl <------ Check the end of the input
    400dc6: je 400dcd <----- The failure case is reached from a loop
    400dc8: cmp dl, BYTE PTR [rcx+rax*1] <------ Compare the input to the answer
    400dcb: je 400de0 <_Unwind_Resume@plt+0x240>
    400dcd: mov edi, 0x4011af <----- the string is loaded
    400dd2: call 400ab0 <puts@plt>
    ...
    400de0: add rax, 0x1
    400de4: cmp rax, 0x15 <---- The length of the correct string is 0x15
    400de8: 75 d6 jne 400dc0 <_Unwind_Resume@plt+0x220>
    ...
    ```

- The base address of the expected string is stored at `rcx`. Since this string is decrypted at runtime, and is not available statically, we'll need to use dynamic analysis to recover it

### Dumping a Dynamic String Buffer Using `gdb`

- `gdb`, the GNU Debugger
- [An extensive manual covering all the supported `gdb` commands](http://www.gnu.org/software/gdb/documentation/)
- Run `ctf` with `gdb` from the start, and then:
  1. `b *0x400dc8`: set a break point at `0x400dc8` (or somewhere around, ensure that the address of the expected value is already loaded to `rcx`)
  2. `set env GUESSME=abc`: set environment variable `GUESSME`
  3. `run show_me_the_flag`: start `ctf` with a parameter `show_me_the_flag`
  4. `info registers rcx`: check the address stored in `rcx`, it's `0x615050`
  5. `x/s 0x615050`: print the string at `0x615050`, the expected string is `Crackers Don't Matter`
- Get the flag

    ```shell
    $ export GUESSME="Crackers Don't Matter"
    $ ./ctf show_me_the_flag
    checking 'show_me_the_flag'
    ok
    flag = 84b34c124b2ba5ca224af8e33b077e9e
    ```
- Tips: do not trust any quotes you typed in the given VM

### Another challenge

- Feed the flag to another program in this directory, 'oracle', to unlock the next challenge
- And there is a hint from the oracle: `Combine the parts`
- I'll do this later

## Chapter 6: Disassembly and Binary Analysis Fundamentals

- The goal of this chapter is to get familiar with the main algorithms behind disassembly and learn what disassemblers can and cannot do
- A recommended guid to reverse engineering: *Chris Eagle's The IDA Pro Book (No Starch Press, 2011)*

### Static Disassembly

- Disassembly usually means *static disassembly*. In contrast, *dynamic disassembly*, more commonly known as *execution tracing*, logs each executed instruction as the binary runs
- Static disassemblers need to perform the following steps:
  1. Load a binary for processing using a binary loader
  2. Find all the machine instructions in the binary
  3. Disassemble these instructions into a human- or machine-readable form
- Step 2 is often very difficult in practice, resulting in disassembly errors
- There are 2 major approaches to static disassembly, each of which tries to avoid disassembly errors in its own way:
  1. *linear disassembly*
  2. *recursive disassembly*
- Unfortunately, neither approach is perfect in every case

#### Linear Disassembly

- Conceptually the simplest approach
- Iterate through all code segments in a binary, decoding all bytes consecutively and parsing them into a list of instructions
- Many simple disassemblers, including `objdump`, use this approach
- The risk: not all bytes may be instructions. E.g., some compilers, such as Visual Studio, intersperse data such as jump tables with the code, without leaving any clues as to where exactly that data is
- *Disassembler Desynchronization*
- On `x86`, the disassembled instruction stream tends to automatically resynchronize itself just a few instructions
- It's safe to use `objdump` for disassembling ELF binaries compiled with recent versions of compilers such as `gcc` or LLVM's `clang`. The `x86` versions of these compilers don't typically emit inline data.
- Visual Studio does emit inline data, so keep an eye out for disassembly errors when using `objdump` to look at PE binaries. The same it true when analyzing ELF bins for other arch., such as ARM
- When analyzing malicious code, note that it may include obfuscations far worse than inline data!

#### Recursive Disassembly

- Recursive disassembly is sensitive to control flow
- Starts from known entry points into the binary, e.g., the main entry point and exported function symbols
- Follows control flow (e.g., jumps and calls) to discover code
- Can work around data bytes in all but a handful of corner cases
- Downside: not all control flow is easy to follow. May miss blocks of code
- Recursive disassembly is the de facto standard in many rev-engineering applications, such as malware analysis
- IDA (*Interactive DisAssembler*) Pro is one of the most advanced and widely used recursive disassemblers

- Jump table is commonly emitted by modern compilers to avoid the need for a complicated tangle of conditional jumps. 
- While efficient, jump tables make recursive disassembly more difficult because they use **indirect control flow**
- Any instructions that the indirect jump may target remain undiscovered unless the disassembler implements specific (compiler-dependent) heuristics to discover and parse jump tables

- In the case where disassembly correctness is paramount, even at the expense of completeness, we can use **dynamic disassembly**

### Dynamic Disassembly

- Also known as *execution tracers* or *instruction tracers*
- A rich set of runtime info at its disposal, such as concrete register and memory contents
- Simply dump instructions (and possibly memory/register contents) as the program executes
- The main downside is the *code coverage problem*: dynamic disassemblers don't see all instructions but only those they execute

#### Example: Tracing a Binary Execution with `gdb`

- There's no widely accepted standard tool on Linux to do "fire-and-forget" execution tracing (unlike on Windows, where excellent tools such as OllyDbg are available)
- The easiest way using only standard tools is with a few `gdb` commands

- After loading `/bin/ls` into `gdb`, `info files` shows the binary's entry point address

#### Code Coverage Strategies

- Code coverage problem
- Many malware samples even try to actively hide from dynamic analysis tools or debuggers like `gdb`
  - Virtually all such tools produce some kind of detectable artifact in the environment; 
  - If nothing else, the analysis inevitably slows down execution, typically enough to be detectable. 
  - Malware detects these artifacts and hides its true behavior if it knows it's being analyzed
    - To enable dynamic analysis on these samples, we must reverse engineer and then disable the malware's anti-analysis checks
    - These anti-analysis tricks are the reason why it's usually a good idea to at least augment our dynamic analysis with static analysis methods
- There are several methods to improve the coverage of dynamic analysis tools, though in general none of them achieves the level of completeness provided by static analysis.

##### Test Suites

- Run the binary with *known test inputs*
- Downside: a ready-made test suite isn't always available, for instance, for proprietary software or malware

##### Fuzzing

- *Fuzzers* try to automatically generate inputs to cover new code paths in a given binary
- Well-knwon fuzzers include AFL, Microsoft's Project Springfield, and Google's OSS-Fuzz
- Broadly speaking, fuzzers fall into 2 categories based on the way they generate inputs:
  1. Gneration-based fuzzers: generate inputs from scratch (possibly with knowledge of the expected input format)
  2. Mutation-based fuzzers: generate new inputs by mutating known valid inputs in some way, for instance, starting from an existing test suite

##### Symbolic Execution

- Symbolic execution allows us to execute an application not with *concrete values*, but with *symbolic values*
- A symbolic execution is essentially an emulation of a program, where all or some of the variables (or register and memory states) are represented using such symbols
- *Path constraints*, restrictions on the concrete values that the symbols could take
- Given the list of path constraints, we can check whether there's any concrete input that would satisfy all these constraints
- There are special programs, called **Constraint Solvers**, that check, given a list of constraints, whether there's any way to satisfy these constraints

### Structuring Disassembled Code and Data

- Most disassemblers structure the disassembled code in some way that's easier to analyze

#### Structuring Code

- Various ways of structuring disassembled code
  1. Compartmentalizing
  2. Revealing control flow

##### Functions

- In most high-level programming languages, functions are the Fundamental building blocks used to group logically connected pieces of code
- *Function detection*: recover the original program's function structure and use it to group disassembled instructions by function
- For binaries with symbolic info, function detection is trivial
- *Overlapping code blocks*: chunks of code that is shared between functions
- In practice, most disassemblers make the assumption that functions are contiguous and don't share code
  - Not true in some cases, especially for **firmware** or **code for embedded systems**
- The predominant strategy is based on *function signatures*, which are patterns of instructions often used at the start or end of a function
  - Used in all well-known recursive disassemblers
  - Function signature patterns include:
    - Well-known *function prologues*: instructions used to set up the function's stack frame

      ```assembly
      Example of x86 function prologue
      push ebp;
      move ebp, esp;
      ```

    - Well-known *function epilogues*: instructions used to tear down the stack frame

      ```assembly
      Example of x86 function epilogue
      leave;
      ret;
      ```

- signature-based function detection algorithm:
  1. pass over the disassembled binary to locate functions that are directly addressed by a `call` instruction
  2. For functions that are called indirectly or tail-called, consult a database of known function signatures
- Be wary of errors: in practice, function patterns vary depending on the platform, compiler, and optimization level used to create the binary
- Optimized functions may not have well-known prologues or epilogues
- Some research explores methods based on the structure of code. The approach has been integrated into Binary Ninja, and the research prototype tool can interoperate with IDA Pro

##### Function Detection Using the `.eh_frame` section

- An interesting alternative approach for ELF bins is based on the `.eh_frame` section, which we can use to circumvent the function detection problem entirely
- The `.eh_frame` section contains info related to DWARF-based debugging features such as stack unwinding
  - This includes function boundary info that identifies all functions in the binary
  - The info is present even in stripped binaries, unless the binary was compiled with `gcc`'s `-fno-asynchronous-unwind-tables` flag
  - Present by default not only in C++ bins that use exception handling but in all bins produced by `gcc`, including plain C bins

##### Control-Flow Graphs

- Some funcs are large, analyzing even one func can be a complex task
- To organize the internals of each function, use a *control-flow graph (CFG)*
- CFGs offer a convenient graphical representation of the code structure
- CFGs represent the code inside a func as a set of code blocks, called *basic blocks*, connected by *branch edges*
- A basic block is a sequence of instructions where the first instruction is the only entry point, and the last instruction is the only exit point
- An edge in the CFG from a basic block B to another basic block C means that the last instruction in B may jump to the start of C
- Call edges are not part of a CFG because they target code outside of the function
- Instead, CFG shows only the **"fallthrough" edge** that points to the instruction where control will return after the function call completes
- Indirect edges are often omitted
- Disassemblers also sometimes define a global CFG, it is called an **interprocedural** CFG (ICFG) since it's the union of all per-function CFGs
- ICFGs avoid the need for error-prone func detection but don't offer the compartmentalization benefits that per-func CFGs have

##### Call Graphs

- A *call graph* is designed to represent the edges between call instructions and functions
- *Address-taken functions*: functions that have their address taken anywhere in the code
- It's nice to know which functions are address-taken because this tells us they might be called **indirectly**

##### Object-Oriented Code

- Many binary analysis tools, including fully featured disassemblers like IDA Pro, are targeted at programs written in *procedural languages* like C
- Object-oriented languages like C++ structure code using *classes* that group logically connected functions and data
- Unfortunately, current binary analysis tools lack the ability to recover class hierarchies and exception-handling structures
- C++ programs often contain function pointers because of the way virtual methods are typically implemented
- *Virtual methods* are class methods that are allowed to be overriden in a derived class
- When compiling, the compiler may not know whether a pointer will point to a base class or a derived class at runtime
  - Cannot statically determine which implementation of a virtual method should be used at runtime
  - To solve this, compilers emits tables of function pointers, called *vtables*, that contain pointers to all the virtual functions of a particular class
    - Vtables usually in read-only memory
    - Each polymorphic object has a pointer (*vptr*) to the vtable for the object's type
    - To invoke a virtual method, the compiler emits code that follows the object's vptr at runtime and directly calls the correct entry in its vtable
    - Control flow more difficult to follow
- In case of automated analysis, we can simply pretend classes don't exist

### Structuring Data

- Automatic data structure detection in stripped binaries is a notoriously difficult problem
  - Aside from some research work, disassemblers generally don't even attempt it
- Some exceptions. E.g., if a reference to a data object is passed to a well-known function, such as a lib func, disassemblers like IDA Pro can automatically infer that data type based on the spec. of the lib func

### Decompilation

- *Decompilers* are tools that attempt to "reverse the compilation process"
  - Typically start from disassembled code and translate it into a higher-level language
- Decompilation process is too error-prone to serve as a reliable basis for any automated analysis

### Intermediate Representations

- *IR, Intermediate Representations*, aka. intermediate languages
- Instruction sets like x86 and ARM contain many different instructions with complex semantics
  - For instance, on x86, even seemingly simple instructions like `add` have side effects, such as setting status flags in the `eflags` register
  - The sheer number of instructions and side effects makes it difficult to reason about binary programs in an automated way
    - E.g., dynamic taint analysis and symbolic execution engines must implement explicit handlers that capture the data-flow semantics of all the instructions they analyze
      - Accurately implementing all these handlers is a daunting task
- IRs are designed to remove this burden
- An IR is a simple language that serves as an abstraction from low-level machine languages
- Popular IRs include *REIL (Reverse Engineering Intermediate Language)* and *VEX IR* (the IR used in the *valgrind* instrumentation framework)
- There's even a tool called *McSema* that translates bin into LLVM bitcode (aka. LLVM IR)
- The idea of IR languages is to automatically translate real machine code, such as x86 code, into an IR that captures all of the machcine code's semantics but is much simpler to analyze
  - REIL contains only 17 different instructions
  - Languages like REIL, VEX and LLVM IR explicitly express all operations, with no obscure instruction side effects
- The trade-off of translating a complex instruction set to a simple IR language is that IR languages are **far less concise**
  - Not an issue for automated analyses
  - Make IR hard to read for humans

## Fundamental Analysis methods

- A few "standard" analyses

### Binary Analysis Properties

- Classify different analysis techniques based on some of the different properties that any binary analysis approach can have

#### Interprocedural and Intraprocedural Analysis

- It's not computationally feasible to analyze every possible path through a nontrivial program
- That's why computationally expensive binary analyses are often *intraprocedural*
  - Consider the code only within a single function at a time
  - Analyze the CFG of each func in turn
  - Downside: it's not complete. Some bugs are triggered by a specific combination of functions
- In contrast, *interprocedural* analysis considers an entire program as a whole, typically by linking all the function CFGs together via the call graph

#### Flow-Sensitivity

