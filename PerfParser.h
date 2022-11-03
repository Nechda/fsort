#ifndef PERFPARSER_H
#define PERFPARSER_H

#include <map>
#include <string>
#include <vector>

struct PerfParser {

    using pair_string = std::pair<std::string, std::string>;
    struct edge_info {
        uint64_t cycles;
        uint64_t calls;
    };
    using FreqTable = std::map<pair_string, edge_info>;

    PerfParser(std::string_view command, std::string_view exec_file, uint64_t n_runs, uint64_t delta)
        : command_{command}, exec_file_{exec_file}, n_runs_{n_runs}, period_delta_{delta} {}

    FreqTable get_control_flow_graph(std::string output_filename) &&;

    static FreqTable read_from_file(std::string filename);

  private:
    std::string_view command_;
    std::string_view exec_file_;
    uint64_t n_runs_;
    uint64_t period_delta_;
};

#endif