#include <filesystem>

#include "../benchmark/config/configs.hpp"
#include "../benchmark/driver/benchmark_driver.hpp"
#include "../include/mpmc/queue.hpp"
#include "../include/mpsc/queue.hpp"
#include "../include/spmc/queue.hpp"
#include "../include/spsc/queue.hpp"

using namespace lock_free;
using namespace benchmark;

int main(int argc, char* argv[]) {
    namespace fs = std::filesystem;

    const fs::path root = PROJECT_ROOT;

    driver::run_benchmark_suite<lock_free::spsc::Queue>(
            config::spsc_config,
            (root / "results" / "spsc.csv").string()
    );

    driver::run_benchmark_suite<lock_free::spmc::Queue>(
            config::spmc_config,
            (root / "results" / "spmc.csv").string()
    );

    driver::run_benchmark_suite<lock_free::mpsc::Queue>(
            config::mpsc_config,
            (root / "results" / "mpsc.csv").string()
    );

    driver::run_benchmark_suite<lock_free::mpmc::Queue>(
            config::mpmc_config,
            (root / "results" / "mpmc.csv").string()
    );

    return 0;
}