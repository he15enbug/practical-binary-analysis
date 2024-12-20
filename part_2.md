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


