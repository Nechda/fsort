#include "CallGraph.h"

void cluster::merge_to_caller(cluster *caller, cluster *callee) {
    caller->size += callee->size;
    caller->cycles += callee->cycles;

    /* Append all cgraph_nodes from callee to caller.  */
    caller->functions.insert(caller->functions.end(), callee->functions.begin(), callee->functions.end());

    callee->functions.clear();
    callee->size = 0;
    callee->cycles = 0;

    /* Iterate all cluster_edges of callee and add them to the caller. */
    for (auto &it : callee->callers) {
        it.second->callee = caller;
        auto ce = caller->get(it.first);

        if (ce != nullptr) {
            ce->weidth += it.second->weidth;
        } else {
            caller->put(it.first, it.second);
        }
    }

    callee->callers.clear();
}