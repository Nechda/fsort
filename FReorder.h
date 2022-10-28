#include "NMParser.h"
#include "PerfParser.h"

#include "CallGraph.h"

struct FReorder {

    FReorder(NMParser::SymTable &&symtable, PerfParser::FreqTable &&freq_table)
        : symtable_{symtable}, freq_table_{freq_table} {}

    void run(std::string_view ouput_file) &&;

  private:
    NMParser::SymTable symtable_;
    PerfParser::FreqTable freq_table_;

    std::vector<node> vertices_;
    std::vector<edge *> edges_;

    void build_cfg();
};