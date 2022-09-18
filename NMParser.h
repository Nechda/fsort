#ifndef NMPARSER_H
#define NMPARSER_H

#include <string>
#include <string_view>
#include <vector>

struct Symbol {
    std::string name;
    uint64_t size;
    uint64_t interal_addr;
    uint64_t ID;
};

struct NMParser {
    using SymTable = std::vector<Symbol>;

    std::string_view input_file_;

    NMParser(std::string_view input_file) : input_file_{input_file} {}

    SymTable get_symbols() &&;
};

#endif