#include "FReorder.h"
#include <numeric>
#include <random>

static std::random_device local_rd;
static std::mt19937 local_rd_gen(local_rd());

namespace {

void random_shuffle(std::vector<int> &state) {
    std::iota(state.begin(), state.end(), 0);
    std::random_shuffle(state.begin(), state.end());
}

void gen_candidate_state(std::vector<int> &state) {
    auto size = state.size();

    std::uniform_int_distribution<> distribution(0, size - 1);

    int range_first_ind = distribution(local_rd_gen);
    int range_second_ind = distribution(local_rd_gen);

    auto range_first_it = state.begin();
    std::advance(range_first_it, range_first_ind);
    auto range_second_it = state.begin();
    std::advance(range_second_it, range_second_ind);

    if (range_first_ind > range_second_ind)
        std::reverse(range_second_it, range_first_it);
    else
        std::reverse(range_first_it, range_second_it);
}

bool do_transit(double probability) { return std::generate_canonical<double, 10>(local_rd_gen) <= probability; }

void apply_reorder(std::vector<node *> &functions, const std::vector<int> &state) {
    std::vector<node *> copy_functions = functions;
    for (auto ind : state)
        functions[ind] = copy_functions[ind];
}

} // namespace

void FReorder::run_on_cluster([[maybe_unused]] cluster &c) {
    auto n_functions = c.functions_.size();
    if (n_functions == 0)
        return;

    std::vector<int> state(n_functions);

    if (n_functions <= 8) {
        std::iota(state.begin(), state.end(), 0);

        double metric = c.evaluate_energy(state);
        int idx = 0;
        int answ_prem = 0;
        do {
            auto cur_metric = c.evaluate_energy(state);
            if (cur_metric < metric && idx != 0) {
                metric = cur_metric;
                answ_prem = idx;
            }
            idx++;
        } while (std::next_permutation(state.begin(), state.end()));

        std::iota(state.begin(), state.end(), 0);
        while (answ_prem--) {
            std::next_permutation(state.begin(), state.end());
        }
        apply_reorder(c.functions_, state);
        return;
    }

    std::vector<int> best_state(n_functions);
    random_shuffle(state);

    constexpr int N_ITERATIONS = 10'000;
    constexpr double INIT_TEMPERATURE = 10.0;
    double T = INIT_TEMPERATURE;

    double current_e = c.evaluate_energy(state);
    for (int i = 0; i < N_ITERATIONS; ++i) {
        std::vector<int> perm = state;
        gen_candidate_state(perm);
        double energy = c.evaluate_energy(perm);
        auto dE = energy - current_e;

        if (dE < 0) {
            current_e = energy;
            state = perm;

            best_state = perm;
        } else {
            auto prob = std::exp(-dE / T);
            if (do_transit(prob)) {
                current_e = energy;
                state = perm;
            }
        }

        T = INIT_TEMPERATURE / (i + 1);
    }

    apply_reorder(c.functions_, best_state);
}