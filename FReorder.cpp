#include "FReorder.h"

#include <filesystem>
#include <fstream>
#include <iostream>
#include <map>
#include <set>
#include <queue>
#include <unordered_map>

constexpr uint64_t PAGE_SIZE = 0x1000;
constexpr uint64_t ITLB_ITEMS = 64;
constexpr uint64_t TREE_SIZE_THRESHOLD = PAGE_SIZE * ITLB_ITEMS;

CallGraph FReorder::build_cfg() {

    CallGraph cg;

    /* Declare node for each function, readed from final binary */
    std::unordered_map<std::string, uint64_t> func_to_id;
    for (const auto &func : symtable_) {
        cg.nodes_.push_back({func.name, func.size, 0, 0});
        func_to_id[func.name] = func_to_id.size();
    }

    /* Adjust edges in cfg */
    for (const auto &[names, edge_info] : freq_table_) {
        const auto &caller = names.first;
        const auto &callee = names.second;

        if (func_to_id.find(caller) == func_to_id.end() || func_to_id.find(callee) == func_to_id.end()) {
            continue;
        }

        auto caller_id = func_to_id[caller];
        auto callee_id = func_to_id[callee];
        cg.edges_.push_back({caller_id, callee_id, edge_info.calls, 0.0});

        // Update cycles count
        cg.nodes_[caller_id].cycles += edge_info.cycles;
    }
    cg.sort_edges();
    // TODO: calc jmp_prob
    cg.normalize_prob();

    return cg;
}

std::vector<uint64_t> FReorder::create_order(const CallGraph &cg) {
    std::vector<uint64_t> final_order;

    struct cluster_t {
        std::vector<uint64_t> func_ids;
        double metric;
    };
    std::vector<cluster_t> clusters;

    auto func_cmp = [&](uint64_t lhs, uint64_t rhs) -> bool {
        auto a_prob = cg.nodes_[lhs].exec_prob;
        auto b_prob = cg.nodes_[rhs].exec_prob;
        return std::abs(a_prob - b_prob) < 1e-6 ? (lhs < rhs) : (a_prob < b_prob);
    };
    std::priority_queue<uint64_t, std::vector<uint64_t>, decltype(func_cmp)> function_queue(func_cmp);
    for(uint64_t i = 0; i < cg.nodes_.size(); i++) function_queue.push(i);
    
    std::set<uint64_t> visited_functions;
    while(!function_queue.empty()) {
        // Find new root of tree
        auto root = function_queue.top();
        function_queue.pop();
        while(visited_functions.contains(root) && !function_queue.empty()) {
            root = function_queue.top();
            function_queue.pop();
        }
        if(function_queue.empty()) break;
        visited_functions.insert(root);

        // Start build tree
        uint64_t tree_binary_size = 0;
        std::vector<uint64_t> selected_nodes;
        std::vector<uint64_t> worklist;
        worklist.push_back(root);

        do {
            // Find most hotest function among sucsessors of visited vertices
            auto max_it = std::max_element(worklist.begin(), worklist.end(), func_cmp);
            std::iter_swap(max_it, std::prev(worklist.end()));
            root = worklist.back();
            worklist.pop_back();

            // Save them
            selected_nodes.push_back(root);
            visited_functions.insert(root);
            tree_binary_size += cg.get_func_size(root);

            // Append non-visited successors into worklist
            for (auto edge_id : cg.successors(root)) {
                auto succ_id = cg.edges_[edge_id].callee_id;
                worklist.push_back(succ_id);
            }

            // Remove visited vertices from worklist
            std::cout << "worklist size = " << worklist.size() << '\n';
            worklist.erase(std::remove_if(worklist.begin(), worklist.end(), [&](uint64_t v){return visited_functions.contains(v);}), worklist.end());
        } while (!worklist.empty() && tree_binary_size < TREE_SIZE_THRESHOLD);

        // Shedule selected_nodes on pages
        std::array<std::vector<uint64_t>, ITLB_ITEMS> page_storage;
        for (uint64_t i = 0; i < selected_nodes.size(); i++) {
            page_storage[i % ITLB_ITEMS].push_back(selected_nodes[i]);
        }

        std::cout << "cluster {\n";
        for (uint64_t i = 0; i < ITLB_ITEMS; i++) {
            for (auto id : page_storage[i]) {
                std::cout << "  " << cg.nodes_[id].name << '\n';
            }
        }
        std::cout << "}\n";

        // Concatenate all pages into one order sequence
        for (uint64_t page_idx = 0; page_idx < ITLB_ITEMS; page_idx++ )
            final_order.insert(final_order.end(), page_storage[page_idx].begin(), page_storage[page_idx].end());
        
    }
    return final_order;
}

void FReorder::run(std::string_view ouput_file) && {
    // Step 1. Build actual call-graph from freq-table
    const auto cg = build_cfg();

    // Step 2. Evaluate actual probability of function execution
    //TODO: implement propper jmp_prob evaluation
    #if 0
    std::vector<double> prob(cg.nodes_.size());
    for (uint64_t i = 0; i < prob.size(); i++) {
        double answ = cg.nodes_[i].exec_prob;
        for (auto edge_id : cg.predecessors(i)) {
            double jmp_prob = 0; //cg.edges_[edge_id].jmp_prob;
            double exec_prob = cg.nodes_[cg.edges_[edge_id].caller_id].exec_prob;
            answ = std::max(answ, jmp_prob * exec_prob);
        }
        prob[i] = answ;
    }
    #endif

    // Step 3. Create order
    auto sorted_funcs = create_order(cg);

    // Step 4. Dump order to the file
    std::ofstream output(std::string(ouput_file), std::ios::out);
    for (auto func_id : sorted_funcs) {
        output << cg.nodes_[func_id].name << '\n';
    }
}
