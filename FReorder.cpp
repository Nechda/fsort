#include "FReorder.h"

#include <cmath>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <map>
#include <unordered_map>

constexpr int CLUSTER_THRESHOLD = 0x1000 * 64;

void FReorder::build_cfg() {
    /* Declare node for each function, readed from final binary */
    for (const auto &func : symtable_) {
        vertices_.push_back({func.name, func.size, 0, nullptr, {}});
    }

    /* Make map for fast searching node by the function name */
    std::unordered_map<std::string, node *> f2n; // func to node
    for (auto &node : vertices_) {
        f2n[node.name] = &node;
    }

    /* Adjust edges in cfg */
    for (const auto &[names, pp] : freq_table_) {
        const auto &caller = names.first;
        const auto &callee = names.second;

        if (f2n.find(caller) == f2n.end() || f2n.find(callee) == f2n.end()) {
            continue;
        }

        auto e = new edge();
        e->caller = f2n[caller];
        e->callee = f2n[callee];
        e->weidth = pp.calls;
        e->cycles = pp.cycles;

        edges_.push_back(e);
    }

    /* After each edges was created, attach it no nodes */
    for (auto e : edges_) {
        e->callee->callers.push_back(e);
    }
}

void FReorder::run(std::string_view ouput_file) && {
    // Step 1. Build actual call-graph from freq-table
    build_cfg();

    // Step 2. Create a cluster for each function, and accociate function to cluster.
    std::vector<cluster *> clusters(vertices_.size());
    for (std::size_t i = 0; i < clusters.size(); i++) {
        clusters[i] = new cluster();
        clusters[i]->add_function_node(&vertices_[i]);
        clusters[i]->ID = i;
        vertices_[i].aux = clusters[i];
    }

    // Step 3. Insert edges between clusters that have a profile.
    std::vector<cluster_edge *> cedges;
    for (auto &cluster : clusters) {
        for (auto function_node : cluster->functions) {
            for (auto &edge : function_node->callers) {
                auto caller_cluster = edge->caller->aux;
                auto callee_cluster = edge->callee->aux;

                caller_cluster->cycles += edge->cycles;

                auto cedge = callee_cluster->get(caller_cluster);
                if (cedge == nullptr) {
                    cedge = new cluster_edge();
                    cedge->caller = caller_cluster;
                    cedge->callee = callee_cluster;

                    cedges.push_back(cedge);
                    cedge->ID = cedges.size();
                    callee_cluster->put(caller_cluster, cedge);
                }
                cedge->weidth += edge->weidth;
            }
        }
    }

    // Step 4. Now insert all created edges into a heap.
    auto &heap = cedges;
    auto extract_max = [&heap]() {
        auto &compare = cluster_edge::comparator;
        auto max_it = std::max_element(heap.begin(), heap.end(), compare);
        std::iter_swap(max_it, std::prev(heap.end()));
        auto cedge = heap.back();
        heap.pop_back();
        return cedge;
    };

    // Step 5. Merge clusters.
    while (!heap.empty()) {
        auto cedge = extract_max();

        auto caller = cedge->caller;
        auto callee = cedge->callee;

        if (caller == callee)
            continue;

        if (caller->size + callee->size > CLUSTER_THRESHOLD)
            continue;

        cluster::merge_to_caller(caller, callee);
        for (auto edge : heap) {
            if (edge->caller == callee) {
                edge->caller = caller;
            }
        }
    }

    // Step 6. Sort the candidate clusters.
    std::sort(clusters.begin(), clusters.end(), cluster::comparator);

    std::ofstream output(std::string(ouput_file), std::ios::out);
    for (auto &c : clusters) {
        c->print(output, true); /* only_funcs = true */
    }

    /* Release memory */
    for (auto &it : clusters)
        delete it;
    for (auto &it : edges_)
        delete it;
}
