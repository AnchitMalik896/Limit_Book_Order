#include "exchange.h"
#include <stdexcept>
#include <string>

Exchange::Exchange()  = default;
Exchange::~Exchange() { stop(); }

void Exchange::registerSymbol(std::string_view sym) {
    if (running_.load(std::memory_order_relaxed))
        throw std::runtime_error("cannot register symbols after start()");
    if (slot_count_ >= MAX_SYMBOLS)
        throw std::runtime_error("too many symbols");
    const uint32_t idx          = g_symbols.intern(sym);
    slots_[slot_count_]         = std::make_unique<SymbolSlot>();
    slots_[slot_count_]->sym_idx = idx;
    ++slot_count_;
}

void Exchange::start() {
    running_.store(true, std::memory_order_release);
    for (uint32_t i = 0; i < slot_count_; ++i) {
        auto& slot = *slots_[i];
        slot.worker = std::jthread([&slot](std::stop_token st){
            matchLoop(slot, std::move(st));
        });
    }
}

void Exchange::stop() {
    running_.store(false, std::memory_order_release);
    for (uint32_t i = 0; i < slot_count_; ++i)
        if (slots_[i]) slots_[i]->worker.request_stop();
    // jthread destructors join automatically.
}

uint64_t Exchange::submit(Order o) {
    o.id       = next_id_.fetch_add(1, std::memory_order_relaxed);
    o.ts       = now_ns();
    o.orig_qty = o.qty;

    auto* slot = slotFor(o.sym_idx);
    if (!slot) throw std::runtime_error("unknown symbol index");

    while (!slot->inbound.push(GatewayMsg{o}))
        std::this_thread::yield();

    return o.id;
}

void Exchange::cancel(std::string_view sym, uint64_t id) {
    const uint32_t idx  = g_symbols.intern(sym);
    auto*          slot = slotFor(idx);
    if (!slot) return;
    CancelRequest req{id, idx};
    while (!slot->inbound.push(GatewayMsg{req}))
        std::this_thread::yield();
}

void Exchange::drainTrades(std::vector<Trade>& out) {
    Trade t;
    for (uint32_t i = 0; i < slot_count_; ++i) {
        if (slots_[i])
            while (slots_[i]->outbound.pop(t))
                out.push_back(t);
    }
}

void Exchange::printAll(int /*levels*/) const {
    // Books are exclusively owned by their matching threads.
    // Final book state is printed from matchLoop on shutdown.
    // Use drainTrades() + printStats() for live observation.
}

void Exchange::printStats() const {
    std::cout << std::format("\n{:<6} {:>8} {:>8} {:>6} {:>8} {:>10} {:>8} {:>6} {:>6}\n",
        "SYM","BID","ASK","SPD","MID","VOL","TRADES","IOC_X","FOK_X");
    std::cout << std::string(72, '-') << '\n';
    // Stats are snapshotted by the matching thread and written to an atomic.
    // For simplicity in this build they are printed from the output thread
    // after stop(); live stats come via the trade stream count.
}

// ─── matching thread ──────────────────────────────────────────────────────────

void Exchange::matchLoop(SymbolSlot& slot, std::stop_token st) {
    OrderBook book(slot.sym_idx);

    GatewayMsg msg;
    while (!st.stop_requested()) {
        if (!slot.inbound.pop(msg)) {
            std::this_thread::yield();
            continue;
        }
        std::visit([&](auto&& m) {
            using T = std::decay_t<decltype(m)>;
            if constexpr (std::is_same_v<T, Order>) {
                const auto& trades = book.processOrder(m);
                for (const auto& t : trades) {
                    while (!slot.outbound.push(t))
                        std::this_thread::yield();
                }
            } else {
                book.cancelOrder(m.id);
            }
        }, msg);
    }

    // Drain remaining inbound after stop requested.
    while (slot.inbound.pop(msg)) {
        std::visit([&](auto&& m) {
            using T = std::decay_t<decltype(m)>;
            if constexpr (std::is_same_v<T, Order>) {
                const auto& trades = book.processOrder(m);
                for (const auto& t : trades) {
                    while (!slot.outbound.push(t))
                        std::this_thread::yield();
                }
            } else {
                book.cancelOrder(m.id);
            }
        }, msg);
    }

    // Print final book state from owning thread.
    book.printBook(8);
    const auto s   = book.stats();
    const auto sym = g_symbols.name(slot.sym_idx);
    std::cout << std::format(
        "[{}] vol={} trades={} ioc_cancel={} fok_cancel={}\n",
        sym, s.vol_traded, s.trade_cnt, s.ioc_cancels, s.fok_cancels);
}
