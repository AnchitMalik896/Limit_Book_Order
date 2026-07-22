#include "exchange.h"
#include "generator.h"
#include "symbol_table.h"
#include <atomic>
#include <chrono>
#include <csignal>
#include <format>
#include <iostream>
#include <string>
#include <thread>
#include <vector>

static std::atomic<bool> g_running{true};
static void on_sigint(int) noexcept {
    g_running.store(false, std::memory_order_relaxed);
}

// ─── output thread ────────────────────────────────────────────────────────────

static void outputLoop(Exchange& exch, std::stop_token st) {
    using namespace std::chrono_literals;
    std::vector<Trade> batch;
    batch.reserve(256);

    long  total_trades = 0;
    long  print_tick   = 0;

    while (!st.stop_requested()) {
        exch.drainTrades(batch);

        for (const auto& t : batch) {
            t.print(g_symbols.name(t.sym_idx));
            ++total_trades;
        }
        batch.clear();

        if (++print_tick % 400 == 0) {
            std::cout << std::format(
                "\n─── output thread: {} trades seen so far ───\n", total_trades);
        }

        std::this_thread::sleep_for(1ms);
    }

    // Final drain
    exch.drainTrades(batch);
    for (const auto& t : batch) {
        t.print(g_symbols.name(t.sym_idx));
        ++total_trades;
    }
    std::cout << std::format("\n[output] total trades received: {}\n", total_trades);
}

// ─── gateway thread ───────────────────────────────────────────────────────────

static void gatewayLoop(Exchange& exch, std::stop_token st) {
    using namespace std::chrono_literals;

    OrderGenerator gen(std::random_device{}());
    gen.addSymbol({"AAPL", 15000, 1, 2,  5, 50, 0.05, 0.08, 0.04});
    gen.addSymbol({"TSLA", 25000, 1, 3,  5, 80, 0.07, 0.10, 0.05});
    gen.addSymbol({"NVDA", 80000, 5, 4,  2, 30, 0.06, 0.09, 0.04});
    gen.addSymbol({"MSFT", 37000, 1, 2,  5, 60, 0.04, 0.07, 0.03});

    std::vector<std::pair<std::string_view, uint64_t>> log;
    log.reserve(512);

    int tick = 0;
    while (!st.stop_requested() && g_running.load(std::memory_order_relaxed)) {
        Order    o  = gen.next();
        uint64_t id = exch.submit(o);

        log.emplace_back(g_symbols.name(o.sym_idx), id);
        if (log.size() > 400)
            log.erase(log.begin(), log.begin() + 100);

        // Periodic cancel of oldest tracked order
        if (tick % 15 == 0 && !log.empty()) {
            auto [sym, cid] = log.front();
            exch.cancel(sym, cid);
        }

        if (tick % 50 == 0) gen.drift(0.008);

        ++tick;
    }
    std::cout << std::format("\n[gateway] submitted {} orders\n", tick);
}

// ─── main ─────────────────────────────────────────────────────────────────────

int main() {
    std::signal(SIGINT, on_sigint);

    // ── 1. Register symbols (must happen before start()) ──
    Exchange exch;
    exch.registerSymbol("AAPL");
    exch.registerSymbol("TSLA");
    exch.registerSymbol("NVDA");
    exch.registerSymbol("MSFT");

    // ── 2. Start one matching thread per symbol ──
    exch.start();

    // ── 3. Start output thread ──
    std::jthread output_thread([&exch](std::stop_token st){
        outputLoop(exch, std::move(st));
    });

    // ── 4. Run gateway on this thread ──
    {
        std::stop_source gw_stop;
        std::jthread gateway_thread([&exch](std::stop_token st){
            gatewayLoop(exch, std::move(st));
        });

        // Block until Ctrl+C
        using namespace std::chrono_literals;
        while (g_running.load(std::memory_order_relaxed))
            std::this_thread::sleep_for(50ms);

        // Signal threads to stop
        gateway_thread.request_stop();
    } // gateway_thread joins here

    output_thread.request_stop();
    // output_thread joins on destruction

    // ── 5. Stop matching threads — each prints its own final book state ──
    exch.stop();

    std::cout << "\n[main] shutdown complete.\n";
    return 0;
}
