#include <algorithm>
#include <cstring>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <map>
#include <sstream>
#include <string>

#include "NMParser.h"

#define emit(var)                                                                                                      \
    do {                                                                                                               \
        std::cout << #var " = " << var << std::endl;                                                                   \
    } while (0)

int get_origin(ssize_t adr, const NMParser::SymTable &symbols) {
    auto predicate = [&symbols, &adr](int i) -> bool { return (ssize_t)symbols[i].interal_addr >= adr; };

    int N = symbols.size();
    int L = -1;
    int R = N;

    while (L + 1 < R) {
        int m = L + (R - L) / 2;
        if (predicate(m))
            R = m;
        else
            L = m;
    }

    if (R >= N)
        return -1;

    auto fit = [&](int idx) {
        auto b = symbols[idx].interal_addr;
        return adr - b < symbols[idx].size;
    };

    if (L >= 0 && fit(L))
        return L;
    if (fit(R))
        return R;
    return -1;
}

struct MMapRecord {
    std::string file;
    size_t start;        // virtual memory start
    size_t stop;         // virtual memory end
    size_t inner_offset; // offset in file
};

const char *get_basename(const char *filename) {
    auto p = std::strrchr(filename, '/');
    return p ? p + 1 : (char *)filename;
}

std::vector<MMapRecord> parse_mmaps(const char *mmap_file, const char *exec_file) {
    std::string base = get_basename(exec_file);
    std::ifstream in(mmap_file);
    std::string buf;
    std::vector<MMapRecord> result;
    while (std::getline(in, buf)) {
        std::string dumb_str;
        MMapRecord r;
        std::stringstream ss(buf);
        ss << std::hex;
        ss >> r.start;
        ss.ignore(buf.size(), '-');
        ss >> r.stop >> dumb_str >> r.inner_offset >> dumb_str >> dumb_str >> r.file;
        r.file = get_basename(r.file.c_str());
        if (r.file == base) {
            result.push_back(r);
        }
    }
    return result;
}

ssize_t get_inner_offset(size_t adr, const std::vector<MMapRecord> &mmaps) {
    auto fit = [&](int i) {
        auto b = mmaps[i].start;
        auto e = mmaps[i].stop;
        return b <= adr && adr < e;
    };

    for (int i = 0; i < mmaps.size(); i++) {
        if (!fit(i))
            continue;
        return adr - mmaps[i].start + mmaps[i].inner_offset;
    }

    return -1;
}

// usage ./gen-cg <path-to-executable> <mmap-file> <raw-data>

int main(int argc, char **argv) {
    if (argc != 4) {
        puts("usage:\n./gen-cg <path-to-executable> <mmap-file> <raw-data>");
        return -1;
    }

    auto exec_file = argv[1];
    auto mmap_file = argv[2];
    auto raw_file = argv[3];

    auto mmaps = parse_mmaps(mmap_file, exec_file);

    auto symbols = NMParser(argv[1]).get_symbols();
    std::sort(begin(symbols), end(symbols),
              [](const Symbol &a, const Symbol &b) { return a.interal_addr < b.interal_addr; });

    std::map<std::pair<uint64_t, uint64_t>, uint64_t> graph;
    std::ifstream in(raw_file);
    std::string buf;
    while (std::getline(in, buf)) {
        // caller callee count
        int64_t cr, ce, cn;
        std::stringstream(buf) >> cr >> ce >> cn;

        cr = get_inner_offset(cr, mmaps);
        ce = get_inner_offset(ce, mmaps);
        if (ce < 0 || cr < 0 || ce == cr)
            continue;

        cr = get_origin(cr, symbols);
        ce = get_origin(ce, symbols);

        if (ce < 0 || cr < 0 || ce == cr)
            continue;
        graph[{cr, ce}] += cn;
    }

    std::cout << "digraph G {\n";
    for (auto [edge, weight] : graph) {
        std::cout << symbols[edge.first].name << " -> " << symbols[edge.second].name << " [label = " << weight
                  << " ]\n";
    }
    std::cout << "}\n";

    return 0;
}