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


