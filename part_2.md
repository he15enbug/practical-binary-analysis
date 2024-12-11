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
