#include "FReorder.h"
#include "NMParser.h"
#include "PerfParser.h"

#include <charconv>
#include <cstring>
#include <filesystem>
#include <iostream>

struct Config {
    std::string binary_file;
    std::string output_file;
    std::string command;
    std::string exec_file;
    int repeat_times;
    int delta;

    void set_field(std::string_view key, std::string_view value) {
        if (key == "--binary") {
            binary_file = value;
        }
        if (key == "--output") {
            output_file = value;
        }
        if (key == "--command") {
            command = value;
        }
        if (key == "--exec-file") {
            exec_file = value;
        }
        if (key == "--runs") {
            auto result = std::from_chars(value.data(), value.data() + value.size(), repeat_times);
            if (result.ec == std::errc::invalid_argument) {
                repeat_times = 0;
                std::cerr << "[Warning] repeat_times := 0, due to incorrect input.";
            }
        }
        if (key == "--delta") {
            auto result = std::from_chars(value.data(), value.data() + value.size(), delta);
            if (result.ec == std::errc::invalid_argument) {
                delta = 0;
                std::cerr << "[Warning] delta := 0, due to incorrect input.";
            }
        }
    }

    void print_config() const {
        std::cout << "File with symbols: " << binary_file << std::endl;
        std::cout << "Output file: " << output_file << std::endl;
        std::cout << "Total runs: " << repeat_times << std::endl;
        std::cout << "Exec file filter: " << exec_file << std::endl;
    }
};

Config parse_opts(int argc, char **argv) {
    Config cfg;
    for (int i = 1; i < argc; i++) {
        auto equal_ch = std::strchr(argv[i], '=');
        if (equal_ch == nullptr) {
            continue;
        }
        std::string_view key = argv[i];
        std::string_view value = argv[i];
        key.remove_suffix(key.size() - (equal_ch - argv[i]));
        value.remove_prefix(equal_ch - argv[i] + 1);

        if (!key.starts_with("--")) {
            continue;
        }
        cfg.set_field(key, value);
    }
    return cfg;
}

int main(int argc, char **argv) {
    auto config = parse_opts(argc, argv);
    if (config.binary_file.empty()) {
        std::cerr << "[Error] The file with symbols is not set, use `--binary=...` option.\n";
        return -1;
    }
    if (config.output_file.empty()) {
        std::cerr << "[Warning] Output file is not set. Default value `sorted.out`.\n";
        config.output_file = "sorted.out";
    }
    if (config.command.empty()) {
        std::cerr << "[Error] Empty command. Nothing to run.\n";
        return -1;
    }

    config.print_config();

    auto symbols = NMParser(config.binary_file).get_symbols();

    PerfParser::FreqTable freq_table;
    const std::string SAVED_PROFILE_FILE{"saved.edges"};
    if (std::filesystem::exists(SAVED_PROFILE_FILE)) {
        freq_table = PerfParser::read_from_file(SAVED_PROFILE_FILE);
    } else {
        freq_table = PerfParser(config.command, config.exec_file, config.repeat_times, config.delta)
                         .get_control_flow_graph(SAVED_PROFILE_FILE);
    }

    FReorder(std::move(symbols), std::move(freq_table)).run(config.output_file);

    return 0;
}
