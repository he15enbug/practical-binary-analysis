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
