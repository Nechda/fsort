#ifndef CALLGRAPH_H
#define CALLGRAPH_H

#include <numeric>
#include <string>
#include <unordered_map>
#include <vector>

struct node;
struct edge;
struct cluster;
struct cluster_edge;

struct node {
    std::string name;
    uint64_t size;
    uint64_t cycles;
    cluster *aux;
    std::vector<edge *> callers;
};

struct edge {
    node *caller{nullptr};
    node *callee{nullptr};

    uint64_t weidth = 0;
    uint64_t cycles = 0;
};

struct cluster {
    cluster() {}

    std::vector<node *> functions;
    std::unordered_map<cluster *, cluster_edge *> callers;
    uint64_t size = 0;
    uint64_t cycles = 0;
    uint64_t ID = 0;

    void add_function_node(node *function_node) {
        functions.push_back(function_node);
        size += function_node->size;
    }

    void drop_all_functions() {
        functions.clear();
        functions.shrink_to_fit();
    }

    void put(cluster *caller, cluster_edge *edge) { callers[caller] = edge; }
    cluster_edge *get(cluster *caller) {
        auto search_it = callers.find(caller);
        return search_it == callers.end() ? nullptr : search_it->second;
    }

    void reset_metrics() {
        size = 0;
        cycles = 0;
        callers.clear();
    }

    static void merge_to_caller(cluster *caller, cluster *callee);

    static bool comparator(cluster *lhs, cluster *rhs) {
        auto constexpr MAX_DENSITY = std::numeric_limits<double>::max();
        double da = lhs->size == 0 ? MAX_DENSITY : (double)lhs->cycles / (double)lhs->size;
        double db = rhs->size == 0 ? MAX_DENSITY : (double)rhs->cycles / (double)rhs->size;
        if (std::abs(da - db) < 1e-5)
            return lhs->ID < rhs->ID;
        return da < db;
    }

    template <typename StreamT> void print(StreamT &stream, bool only_funcs) const {
        if (only_funcs) {
            for (auto node : functions)
                stream << node->name << '\n';
            return;
        }

        if (functions.empty())
            return;
        stream << "Cluster{\n  size = " << size << "\n";
        stream << "  samples = " << cycles << "\n";
        stream << "  dencity = " << (double)cycles / size << "\n";
        stream << "  functions = " << functions.size() << "\n";
        for (auto node : functions) {
            stream << "    func.name = " << node->name << "\n";
        }
        stream << "}\n";
    }
};

/* Cluster edge is an oriented edge in between two clusters.  */
struct cluster_edge {
    cluster *caller = nullptr;
    cluster *callee = nullptr;
    uint64_t weidth = 0;
    uint64_t ID = 0;

    double get_cost() const {
        auto constexpr MAX_COST = std::numeric_limits<double>::max();
        auto sz = caller->size + 2 * callee->size;
        double wd = weidth;
        return sz == 0 ? MAX_COST : wd / sz;
    }

    static bool comparator(cluster_edge *lhs, cluster_edge *rhs) {
        auto cl = lhs->get_cost();
        auto cr = rhs->get_cost();
        if (cl != cr)
            return cl < cr;

        auto sl = lhs->callee->size + lhs->caller->size;
        auto sr = rhs->callee->size + rhs->caller->size;
        if (sl != sr)
            return sl > sr; // prefer small blocks

        return lhs->ID < rhs->ID;
    }
};

#endif