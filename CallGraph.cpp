#include "CallGraph.h"

double cluster::evaluate_energy(const std::vector<int> &perm) const { //! TODO check it
    std::vector<size_t> dists(functions_.size());
    std::unordered_map<node *, int> table;
    for (std::size_t i = 0; i < functions_.size(); i++) {
        table[functions_[perm[i]]] = perm[i];
    }
    auto dist_for_func = [this, &dists, &table](node *f) -> int { return dists[table[f]]; };

    auto get_dist_for_call = [&](node *caller, node *callee) -> int {
        return std::abs(dist_for_func(caller) + (int)caller->size_ / 2 - dist_for_func(callee));
    };

    dists[perm[0]] = 0;
    for (std::size_t i = 1; i < functions_.size(); i++) {
        dists[perm[i]] = dists[perm[i - 1]] + functions_[perm[i]]->size_;
    }

    double cur_metric = 0;
    for (auto node : functions_) {
        for (auto e : node->callers_) {
            auto caller = e->caller;
            auto callee = e->callee;
            if (callee->aux_ != this || caller->aux_ != this)
                continue;
            cur_metric += (double)e->freq / (1.0 + get_dist_for_call(caller, callee));
        }
    }
    return cur_metric;
}

void cluster::merge_to_caller(cluster *caller, cluster *callee) {
    caller->m_size += callee->m_size;
    caller->freq_ += callee->freq_;
    caller->misses_ += callee->misses_;

    /* Append all cgraph_nodes from callee to caller.  */
    for (unsigned i = 0; i < callee->functions_.size(); i++) {
        caller->functions_.push_back(callee->functions_[i]);
    }

    callee->functions_.clear();
    callee->m_size = 0;
    callee->freq_ = 0;
    callee->misses_ = 0;

    /* Iterate all cluster_edges of callee and add them to the caller. */
    for (auto &it : callee->callers_) {
        it.second->m_callee = caller;
        auto ce = caller->get(it.first);

        if (ce != nullptr) {
            ce->m_count += it.second->m_count;
            ce->misses_ += it.second->misses_;
        } else {
            caller->put(it.first, it.second);
        }
    }

    callee->callers_.clear();
}