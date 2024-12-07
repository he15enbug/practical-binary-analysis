#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <errno.h>

#include <string>
#include <vector>

#include <bfd.h>

#include "./loader.h"


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

static bfd* open_bfd(std::string &fname) {
    static int bfd_inited = 0;
    bfd *bfd_h;

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

static int load_symols_bfd(bfd *bfd_h, Binary *bin) {
    int ret;
    long n, nsyms, i;
    /* In libbfd, symbols are represented by the asymbol structure, which is just a short name for struct bfd_symbol */
    asymbol **bfd_symtab;
    Symbol *sym;

    bfd_symtab = NULL;

    /* Tells us how many bytes to allocate to store all symbols */
    n = bfd_get_symtab_upper_bound(bfd_h);

    if(n < 0) {
        fprint(stderr, "failed to read symtab (%s)\n", bfd_errmsg(bfd_get_error())));
        goto fail;
    }
    else if(n) {
        /* Malloc enough space for all symbols */
        bfd_symtab = (asymbol**) malloc(n);
        if(!bfd_symtab) {
            fprint(stderr, "out of memory\n");
            goto fail;
        }

        /* Populate the symbol table and return the number of symbols that are placed in the table */
        nsyms = bfd_canonicalize_symtab(bfd_h, bfd_symtab);

        if(nsyms < 0) {
            fprint(stderr, "failed to read symtab (%s)\n", bfd_errmsg(bfd_get_error()));
            goto fail;
        }

        for(i = 0; i < nsyms; i++) {
            /* For this binary loader, we are interested only in function symbols */
            if(bfd_symtab[i]->flags & BSF_FUNCTION) {
                bin->symols.push_back(Symbol());
                sym = &bin->symbols.back();
                sym->type = Symbol::SYM_TYPE_FUNC;
                sym->name = std:string(bfd_symtab[i]->name);
                /* Get a function symbol's value, which is the start address */
                sym->addr = bfd_asymbol_value(bfd_symtab[i]);
            }
        }
    }

    ret = 0;
    goto cleanup;

    fail:
    return -1;

    cleanup:
    if(bfd_symtab) {
        free(bfd_symtab);
    }
    return ret;
}

static int load_dynsym_bfd(bfd *bfd_h, Binary *bin) {
    int ret;
    long n, nsyms, i;
    
    /* The same data structure for both static and dynamic symbols */
    asymbol **bfd_dynsym;
    Symbol *sym;

    bfd_dynsym = NULL;

    n = bfd_get_dynamic_symtab_upper_bound(bfd_h);

    if(n < 0) {
        fprint(stderr, "failed to read dynamic symtab (%s)\n", bfd_errmsg(bfd_get_error())));
        goto fail;
    }
    else if(n) {
        bfd_dynsym = (asymbol**) malloc(n);
        if(!bfd_dynsym) {
            fprint(stderr, "out of memory\n");
            goto fail;
        }

        nsyms = bfd_canonicalize_dynamic_symtab(bfd_h, bfd_dynsym);

        if(nsyms < 0) {
            fprint(stderr, "failed to read symtab (%s)\n", bfd_errmsg(bfd_get_error()));
            goto fail;
        }

        for(i = 0; i < nsyms; i++) {
            
            if(bfd_dynsym[i]->flags & BSF_FUNCTION) {
                bin->symols.push_back(Symbol());
                sym = &bin->symbols.back();
                sym->type = Symbol::SYM_TYPE_FUNC;
                sym->name = std:string(bfd_dynsym[i]->name);
                sym->addr = bfd_asymbol_value(bfd_dynsym[i]);
            }
        }
    }

    ret = 0;
    goto cleanup;

    fail:
    return -1;

    cleanup:
    if(bfd_symtab) {
        free(bfd_symtab);
    }
    return ret;
}