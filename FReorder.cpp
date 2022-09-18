#include "FReorder.h"

#include <filesystem>
#include <fstream>
#include <iostream>
#include <map>
#include <unordered_map>

constexpr int CLUSTER_THRESHOLD = 0x1000 * 64;

void FReorder::build_cfg() {
    /* Declare node for each function, readed from final binary */
    for (const auto &func : symtable_) {
        vertices_.push_back({func.name, func.size, nullptr});
    }

    /* Make map for fast searching node by the function name */
    std::unordered_map<std::string, node *> f2n; // func to node
    for (auto &node : vertices_) {
        f2n[node.name_] = &node;
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
        e->freq = pp;

        edges_.push_back(e);
    }

    /* After each edges was created, attach it no nodes */
    for (auto e : edges_) {
        e->callee->callers_.push_back(e);
    }
}

void FReorder::run_on_cluster([[maybe_unused]] cluster &c) {
    // TODO: implement ann alogrithm
    return;
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
        vertices_[i].aux_ = clusters[i];
    }

    // Step 3. Insert edges between clusters that have a profile.
    std::vector<cluster_edge *> cedges;
    for (auto &cluster : clusters) {
        for (auto function_node : cluster->functions_) {
            for (auto &edge : function_node->callers_) {
                auto [caller_cluster, callee_cluster, freq, miss] = edge->unpack_data();

                caller_cluster->freq_ += freq;
                caller_cluster->misses_ += miss;

                auto cedge = callee_cluster->get(caller_cluster);
                if (cedge != nullptr) {
                    cedge->m_count += freq;
                    cedge->misses_ += miss;
                } else {
                    auto cedge = new cluster_edge(caller_cluster, callee_cluster, freq, miss);
                    cedges.push_back(cedge);
                    cedge->ID = cedges.size();
                    callee_cluster->put(caller_cluster, cedge);
                }
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

        auto caller = cedge->m_caller;
        auto callee = cedge->m_callee;

        if (caller == callee)
            continue;

        if (caller->m_size + callee->m_size > CLUSTER_THRESHOLD)
            continue;

        cluster::merge_to_caller(caller, callee);
    }

    // Step 6. Sort the candidate clusters.
    std::sort(clusters.begin(), clusters.end(), cluster::comparator);

    // Step 7. Reconnect function to their clusters
    for (auto &cluster : clusters) {
        for (auto function_node : cluster->functions_) {
            function_node->aux_ = cluster;
            function_node->freq_ = 0;
        }
    }

    // Step 8. Recalculate edge frequencies
    std::map<std::pair<node *, node *>, edge *> f2e;
    for (auto e : edges_) {
        if (e->caller->aux_ == e->callee->aux_) {
            e->caller->freq_ += e->freq;
            f2e[{e->caller, e->callee}] = e;
        }
    }

    // Step 9. Do local reordering for each cluster.
    for (auto &c : clusters) {
        run_on_cluster(*c);
    }

    // Step 10. Write final order into file
    std::ofstream output(std::string(ouput_file), std::ios::out);
    for (auto &c : clusters) {
        c->print(std::cerr, false); /* only_funcs = false */
        c->print(output, true);     /* only_funcs = true */
    }

    /* Release memory */
    for (auto &it : clusters)
        delete it;
    for (auto &it : edges_)
        delete it;
}
