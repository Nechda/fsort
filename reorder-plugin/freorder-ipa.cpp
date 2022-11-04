/*
 * This plugin is a truncated version of Martin Liska's c3-pass
 * The original sources you may see on
 * https://gcc.gnu.org/legacy-ml/gcc-patches/2019-09/msg01142.html
 * */

// This is the first gcc header to be included
#include "gcc-plugin.h"
#include "plugin-version.h"
#include "tree-pass.h"
#include "context.h"
#include "tree.h"
#include "cgraph.h"

#include <fstream>
#include <iostream>
#include <unordered_map>
#include <vector>

int plugin_is_GPL_compatible; ///< To prove our dedication to free software

static struct plugin_info freorder_plugin_info = {"1.0", "c3_reorder"};

namespace {

struct dbg {
    dbg() {
        const auto home_dir = getenv("HOME");
        if (home_dir) {
            file.open(std::string(home_dir) + "/.pass_dump.out");
        } else {
            file.open("/dev/null");
        }
    }
    template <typename T> dbg &operator<<(const T &val) {
        file << val;
        return *this;
    }

  private:
    std::ofstream file;
};

static std::unordered_map<std::string, int> get_order()
{
    const auto filename = getenv("ORDER");
    if (!filename) return {};

    std::ifstream file(filename);
    if (!file.is_open()) return {};

    std::unordered_map<std::string, int> table;
    int idx = 0;
    std::string str;
    while (std::getline(file, str)) {
        table[str] = idx++;
    }
    return table;
}

static unsigned int reorder_functions() {
    dbg outs;

    static int last_idx = 10'000'000; // yea this a magic number


    outs << "Start reordering pass\n";
    auto table = get_order();

    cgraph_node *node;
    FOR_EACH_DEFINED_FUNCTION(node) {
        if (node == nullptr && !node->alias && !node->global.inlined_to)
            continue;

        int id = 0;
        auto search = table.find(node->asm_name());
        if (search == table.end()) {
            id = last_idx--;
        } else {
            id = search->second;
        }

        outs << "Function:\n"
             << "    Name:" << node->name() << "\n"
             << "    AsmName:" << node->asm_name() << "\n"
             << "    Index:" << id << "\n";
        node->text_sorted_order = id;
    }

    return 0;
}

/// passage info
const pass_data pass_data_ipa_reorder = {
    SIMPLE_IPA_PASS, /* type */
    "c3_reorder",    /* name */
    OPTGROUP_NONE,   /* optinfo_flags */
    TV_NONE,         /* tv_id */
    0,               /* properties_required */
    0,               /* properties_provided */
    0,               /* properties_destroyed */
    0,               /* todo_flags_start */
    0,               /* todo_flags_finish */
};

struct reorder_pass : public simple_ipa_opt_pass {
    reorder_pass(gcc::context *ctxt) : simple_ipa_opt_pass(pass_data_ipa_reorder, ctxt) {}
    unsigned int execute(function *) override { return reorder_functions(); }
    bool gate(function *) override { return flag_reorder_functions; }
};

} // namespace

int plugin_init(plugin_name_args *plugin_info, plugin_gcc_version *version) {
    if (!plugin_default_version_check(version, &gcc_version)) {
        std::cerr << "Bad gcc version to compile the plugin" << std::endl;
        return 1;
    }

    register_callback(plugin_info->base_name, PLUGIN_INFO, NULL, &freorder_plugin_info);
    register_pass_info pass_info;
    pass_info.pass = new reorder_pass(g);
    pass_info.reference_pass_name = "simdclone";
    pass_info.ref_pass_instance_number = 1;
    pass_info.pos_op = PASS_POS_INSERT_AFTER;
    register_callback(plugin_info->base_name, PLUGIN_PASS_MANAGER_SETUP, NULL, &pass_info);

    return 0;
}
