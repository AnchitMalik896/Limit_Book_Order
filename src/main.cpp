#include "exchange.h"
#include "generator.h"
#include <atomic>
#include <chrono>
#include <csignal>
#include <format>
#include <iostream>
#include <string>
#include <thread>
#include <vector>

static std::atomic<bool> running{true};

// std::signal is the C++ name for signal(); still lowest-overhead portable
// way to catch SIGINT without threading. The handler only writes to an atomic.
static void on_sigint(int) noexcept { running.store(false, std::memory_order_relaxed); }

static void clear_screen() {
    std::cout << "\033[2J\033[H";
}

static void print_header(int tick, int total) {
    std::cout
        << "╔═══════════════════════════════════════════════════════════════╗\n"
        << "║         LIMIT ORDER BOOK EXCHANGE SIMULATOR                   ║\n"
        << std::format("║         Tick: {:<6}  Total Orders: {:<22}║\n", tick, total)
        << "╚═══════════════════════════════════════════════════════════════╝\n";
}

static void print_trades(const std::vector<Trade>& trades) {
    if (trades.empty()) return;
    std::cout << "\n─── EXECUTIONS ─────────────────────────────────────────────────\n";
    for (auto& t : trades) t.print();
}

int main() {
    std::signal(SIGINT, on_sigint);

    Exchange       exch;
    OrderGenerator gen(std::random_device{}());

    gen.addSymbol({"AAPL",  15000, 1, 2, 5, 50, 0.05});
    gen.addSymbol({"TSLA",  25000, 1, 3, 5, 80, 0.07});
    gen.addSymbol({"NVDA",  80000, 5, 4, 2, 30, 0.06});
    gen.addSymbol({"MSFT",  37000, 1, 2, 5, 60, 0.04});

    constexpr int WARMUP        = 200;
    constexpr int PRINT_EVERY   = 20;
    constexpr int DRIFT_EVERY   = 50;
    constexpr int CANCEL_PERIOD = 15;
    using namespace std::chrono_literals;

    int  total_orders = 0;
    int  tick         = 0;

    std::vector<std::pair<std::string, uint64_t>> order_log;
    order_log.reserve(512);

    while (running) {
        Order    o  = gen.next();
        uint64_t id = exch.submit(o);
        ++total_orders;

        order_log.emplace_back(o.sym, id);
        if (order_log.size() > 400)
            order_log.erase(order_log.begin(), order_log.begin() + 100);

        if (tick % CANCEL_PERIOD == 0 && !order_log.empty()) {
            auto& [csym, cid] = order_log.front();
            exch.cancel(csym, cid);
        }

        if (tick % DRIFT_EVERY == 0) gen.drift(0.008);

        if (tick >= WARMUP && tick % PRINT_EVERY == 0) {
            clear_screen();
            print_header(tick, total_orders);
            print_trades(exch.lastTrades());
            exch.printAll(6);
            std::cout << "\n─── MARKET SUMMARY ─────────────────────────────────────────────\n";
            exch.printStats();
            std::cout << "\n[Press Ctrl+C to exit]\n";
            std::this_thread::sleep_for(80ms);
        }

        ++tick;
    }

    std::cout << "\n\nFinal State:\n";
    exch.printAll(10);
    exch.printStats();
    std::cout << std::format("\nTotal orders submitted: {}\nGoodbye.\n", total_orders);
}
