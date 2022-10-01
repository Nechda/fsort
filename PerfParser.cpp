#include "PerfParser.h"

#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#include <array>
#include <iostream>
#include <map>
#include <sstream>
#include <string>
#include <vector>

namespace {

struct Jump {
    std::string from;
    std::string to;
    int cycles{1};
    int miss{0};
    [[maybe_unused]] friend std::ostream &operator<<(std::ostream &out, const Jump &j) {
        out << "{from = " << std::hex << j.from << "; to = " << std::hex << j.to << '}';
        return out;
    }
};

struct Trace {
  private:
    std::array<Jump, 32U> stack_;
    int size_ = 0;

  public:
    int size() const { return size_; }

    bool empty() const { return size_ == 0; }

    void push_back(Jump j) { stack_[size_++] = j; }
    void pop_back() { size_ -= !empty(); }

    Jump &back() { return stack_[size_ - 1]; }

    const Jump &operator[](int i) const { return stack_[i]; }
    Jump &operator[](int i) { return stack_[i]; }

    friend std::istream &operator>>(std::istream &in, Trace &trace);
    [[maybe_unused]] friend std::ostream &operator<<(std::ostream &out, const Trace &trace);
};

using FreqTable = PerfParser::FreqTable;

std::tuple<std::string_view, std::string_view, int> extract_br(const std::string &str) {
    auto pos = str.find('/');
    std::string_view fi(str.data(), pos);

    auto pos2 = str.find('/', pos + 1);
    std::string_view se(str.data() + pos + 1, pos2 - pos - 1);

    // drop +0x...

    fi = fi.substr(0, fi.find("+0x"));
    se = se.substr(0, se.find("+0x"));

    std::string_view cy(str.data() + str.find_last_of('/') + 1);
    auto cycles = std::atoi(cy.data());

    return {fi, se, cycles};
}

std::istream &operator>>(std::istream &in, Trace &trace) {
    std::string comm, event;
    in >> comm >> event;
    std::string jump_event;
    for (; in >> jump_event;) {
        if (comm != "cc1plus")
            continue;

        auto [a, b, cycles] = extract_br(jump_event);

        Jump j;
        j.from = a;
        j.to = b;
        j.cycles = cycles;
        j.miss = !event.starts_with("cycles") && trace.empty();
        trace.push_back(j);
    }
    return in;
}

std::ostream &operator<<(std::ostream &out, const Trace &trace) {
    for (int i = 0; i < trace.size(); i++) {
        out << trace[i] << " ";
    }
    return out;
}

std::vector<Trace> read_traces() {
    std::vector<Trace> traces;
    std::string buf;
    while (std::getline(std::cin, buf)) {
        Trace t;
        std::stringstream(buf) >> t;
        traces.push_back(t);
    }
    return traces;
}

std::vector<Trace> execute() {
    constexpr int WRITE_END = 1;
    constexpr int READ_END = 0;
    pid_t pid;
    int fd[2];
    if (pipe(fd))
        throw std::runtime_error("Bad pipe open");
    pid = fork();
    std::vector<Trace> traces;
    if (pid == 0) {
        dup2(fd[WRITE_END], STDOUT_FILENO);
        close(fd[READ_END]);
        close(fd[WRITE_END]);
        execlp("perf", "perf", "script", "-F", "comm,event,brstacksym", "--no-demangle", "-v", NULL);
        std::cerr << "Failed perf\n";
        exit(1);
    } else {
        dup2(fd[READ_END], STDIN_FILENO);
        close(fd[READ_END]);
        close(fd[WRITE_END]);
        traces = read_traces();
        int status;
        waitpid(pid, &status, 0);
    }
    return traces;
}

void update_table(FreqTable &table, const std::vector<Trace> &traces) {
    for (const auto &tr : traces) {
        for (int i = 0; i < tr.size(); i++) {
            auto &j = tr[i];
            if (j.from != "[unknown]" && j.to != "[unknown]") {
                table[{j.from, j.to}] += j.cycles;
            }
        }
    }
}

const std::string &get_formated_command(std::string_view command, uint64_t period) {
    if (command.empty())
        throw std::runtime_error("Empty command. Nothing to run.");

    static std::string buf;

    std::stringstream ss;
    ss << "perf record -e cycles/period=" << period << "/u -j call " << command;
    buf = ss.str();
    return buf;
}

} // namespace

PerfParser::FreqTable PerfParser::get_control_flow_graph() && {
    FreqTable table;
    for (uint64_t i = 0; i < n_runs_; i++) {
        constexpr uint64_t LOW_PERIOD = 250'000;
        auto period = LOW_PERIOD + i * period_delta_;
        std::cout << "[" << (i + 1) << "/" << n_runs_ << "] run, period = " << period << std::endl;
        auto ret_code = system(get_formated_command(command_, period).c_str());
        if (ret_code != 0) {
            throw "Perf record failed";
        }
        auto traces = execute();
        update_table(table, traces);
    }
    return table;
}