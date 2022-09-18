#ifndef CALLGRAPH_H
#define CALLGRAPH_H

#include <string>
#include <unordered_map>
#include <vector>

struct node;
struct edge;
struct cluster;
struct cluster_edge;

struct node {
    std::string name_;
    uint64_t size_;
    uint64_t freq_;
    cluster *aux_;
    std::vector<edge *> callers_;

    node(const std::string &name, uint64_t size, cluster *aux) : name_(name), size_(size), aux_(aux) {}
};

struct edge {
    node *caller{nullptr};
    node *callee{nullptr};

    uint64_t freq = 0;
    uint64_t miss = 0;

    std::tuple<cluster *, cluster *, uint64_t, uint64_t> unpack_data() const {
        return {caller->aux_, callee->aux_, freq, miss};
    }
};

struct cluster {
    cluster() {}

    std::vector<node *> functions_;
    std::unordered_map<cluster *, cluster_edge *> callers_;
    uint64_t m_size = 0;
    uint64_t freq_ = 0;
    uint64_t misses_ = 0;
    uint64_t ID = 0;

    void add_function_node(node *function_node) {
        functions_.push_back(function_node);
        m_size += function_node->size_;
    }

    void drop_all_functions() {
        functions_.clear();
        functions_.shrink_to_fit();
    }

    void put(cluster *caller, cluster_edge *edge) { callers_[caller] = edge; }
    cluster_edge *get(cluster *caller) {
        auto search_it = callers_.find(caller);
        return search_it == callers_.end() ? nullptr : search_it->second;
    }

    void reset_metrics() {
        m_size = 0;
        freq_ = 0;
        misses_ = 0;
        callers_.clear();
    }

    double evaluate_energy(const std::vector<int> &perm) const;

    static void merge_to_caller(cluster *caller, cluster *callee);

    static bool comparator(cluster *lhs, cluster *rhs) {
        constexpr double MAX_DENSITY = 1e+8;
        double da = lhs->m_size == 0 ? MAX_DENSITY : (double)lhs->freq_ / (double)lhs->m_size;
        double db = rhs->m_size == 0 ? MAX_DENSITY : (double)rhs->freq_ / (double)rhs->m_size;
        if (std::abs(da - db) < 1e-5)
            return lhs->ID < rhs->ID;
        return da < db;
    }

    template <typename FuncT> void visit_edges(FuncT F) {
        for (auto node : functions_) {
            for (auto e : node->callers_) {
                auto caller = e->caller;
                auto callee = e->callee;
                if (callee->aux_ != this || caller->aux_ != this)
                    continue;
                F(e);
            }
        }
    }

    template <typename StreamT> void print(StreamT &stream, bool only_funcs) const {
        if (only_funcs) {
            for (auto node : functions_)
                stream << node->name_ << '\n';
            return;
        }

        if (functions_.empty())
            return;
        stream << "Cluster{\n  size = " << m_size << "\n";
        stream << "  samples = " << freq_ << "\n";
        stream << "  dencity = " << (double)freq_ / m_size << "\n";
        stream << "  functions = " << functions_.size() << "\n";
        for (auto node : functions_) {
            stream << "    func.name = " << node->name_ << "\n";
        }
        stream << "}\n";
    }
};

/* Cluster edge is an oriented edge in between two clusters.  */
struct cluster_edge {
    cluster *m_caller = nullptr;
    cluster *m_callee = nullptr;
    uint64_t m_count = 0;
    uint64_t misses_ = 0;
    uint64_t ID = 0;

    cluster_edge(cluster *caller, cluster *callee, uint64_t count, uint64_t miss)
        : m_caller(caller), m_callee(callee), m_count(count), misses_(miss) {}

    double get_cost() const { return (double)(m_count) / (1 + m_callee->m_size + m_caller->m_size); }

    static bool comparator(cluster_edge *lhs, cluster_edge *rhs) {
        auto cl = lhs->get_cost();
        auto cr = rhs->get_cost();
        if (cl != cr)
            return cl < cr;

        auto sl = lhs->m_callee->m_size + lhs->m_caller->m_size;
        auto sr = rhs->m_callee->m_size + rhs->m_caller->m_size;
        if (sl != sr)
            return sl > sr; // prefer small blocks

        return lhs->ID < rhs->ID;
    }
};

#endif