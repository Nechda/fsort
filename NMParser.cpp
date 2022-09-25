#include "NMParser.h"

#include <bfd.h>

/* This structures from elf-bfd.h */
struct elf_internal_sym {
    bfd_vma st_value;
    bfd_vma st_size;
    unsigned long st_name;
    unsigned char st_info;
    unsigned char st_other;
    unsigned char st_target_internal;
    unsigned int st_shndx;
};

typedef struct elf_internal_sym Elf_Internal_Sym;

typedef struct {

    asymbol symbol;
    elf_internal_sym internal_elf_sym;
    union {
        unsigned int hppa_arg_reloc;
        void *mips_extr;
        void *any;
    } tc_data;

    unsigned short version;

} elf_symbol_type;

#define elf_symbol_from(S)                                                                                             \
    ((((S)->flags & BSF_SYNTHETIC) == 0 && (S)->the_bfd != NULL &&                                                     \
      (S)->the_bfd->xvec->flavour == bfd_target_elf_flavour && (S)->the_bfd->tdata.elf_obj_data != 0)                  \
         ? (elf_symbol_type *)(S)                                                                                      \
         : 0)

enum Errors { CANT_OPEN_FILE = 1, CHECK_FORMAT_ERROR, NO_SYMBOLS, CANT_CREATE_EMPTY_SYMBOL };

template <typename Action_t> static int iterate_over_symbols(const char *filename, Action_t action) {
    bfd_init();
    bfd *abfd;
    char **matching;
    abfd = bfd_openr(filename, NULL);
    if (abfd == nullptr) {
        return CANT_OPEN_FILE;
    }
    if (!bfd_check_format_matches(abfd, bfd_object, &matching)) {
        return CHECK_FORMAT_ERROR;
    }
    if (!(bfd_get_file_flags(abfd) & HAS_SYMS)) {
        return NO_SYMBOLS;
    }

    // Read symbols
    constexpr int NO_DYNAMIC = 0; // only non-synamic symbols
    void *minisyms;
    unsigned size;
    long symcount = bfd_read_minisymbols(abfd, NO_DYNAMIC, &minisyms, &size);

    asymbol *store;
    asymbol *sym;
    symbol_info syminfo;
    bfd_byte *from;
    bfd_byte *fromend;
    store = bfd_make_empty_symbol(abfd);
    if (store == nullptr)
        return CANT_CREATE_EMPTY_SYMBOL;

    from = (bfd_byte *)minisyms;
    fromend = from + symcount * size;

    // And iterate over it
    for (; from < fromend; from += size) {
        sym = bfd_minisymbol_to_symbol(abfd, NO_DYNAMIC, from, store);
        if (sym == nullptr)
            continue;

        if (sym->flags & (BSF_FUNCTION | BSF_GLOBAL)) {
            bfd_get_symbol_info(abfd, sym, &syminfo);
            elf_symbol_type *elfinfo = elf_symbol_from(sym);
            action(syminfo.name, elfinfo ? elfinfo->internal_elf_sym.st_size : 0);
        }
    }

    free(minisyms);
    bfd_close(abfd);

    return 0;
}

NMParser::SymTable NMParser::get_symbols() && {
    SymTable ret;
    iterate_over_symbols(input_file_.c_str(), [&](const char *name, size_t size) {
        Symbol s;
        s.ID = ret.size() + 1;
        s.interal_addr = 0; // unused?
        s.name = name;
        s.size = size;
        ret.push_back(s);
    });
    return ret;
}