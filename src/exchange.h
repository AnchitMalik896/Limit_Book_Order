#pragma once
#include "gateway_msg.h"
#include "orderbook.h"
#include "spsc_queue.h"
#include "symbol_table.h"
#include "trade.h"
#include <atomic>
#include <cstdint>
#include <format>
#include <iostream>
#include <memory>
#include <string>
#include <string_view>
#include <thread>
#include <array>

// Queue capacities — must be powers of two.
// Kept modest so SymbolSlot lives comfortably on the heap.
static constexpr std::size_t INBOUND_SZ  = 1024;
static constexpr std::size_t OUTBOUND_SZ = 512;

struct SymbolSlot {
    SPSCQueue<GatewayMsg, INBOUND_SZ> inbound;
    SPSCQueue<Trade,      OUTBOUND_SZ> outbound;
    std::jthread                       worker;
    uint32_t                           sym_idx{};

    SymbolSlot() = default;
    SymbolSlot(const SymbolSlot&)            = delete;
    SymbolSlot& operator=(const SymbolSlot&) = delete;
};

class Exchange {
public:
    Exchange();
    ~Exchange();

    // Register a symbol before starting threads.
    void registerSymbol(std::string_view sym);

    // Start one matching thread per registered symbol.
    void start();

    // Submit an order; returns assigned order id.
    uint64_t submit(Order o);

    // Request a cancel. Non-blocking — enqueues the request.
    void cancel(std::string_view sym, uint64_t id);

    // Drain all pending trade notifications into a caller-supplied vector.
    void drainTrades(std::vector<Trade>& out);

    // Print all books. Called from output thread only.
    void printAll(int levels = 5) const;
    void printStats()             const;

    [[nodiscard]] bool running() const noexcept {
        return running_.load(std::memory_order_relaxed);
    }

    void stop();

private:
    std::array<std::unique_ptr<SymbolSlot>, MAX_SYMBOLS> slots_;
    uint32_t                                              slot_count_ = 0;
    std::atomic<uint64_t>               next_id_{1};
    std::atomic<bool>                   running_{false};

    [[nodiscard]] SymbolSlot* slotFor(uint32_t sym_idx) noexcept {
        for (uint32_t i = 0; i < slot_count_; ++i)
            if (slots_[i] && slots_[i]->sym_idx == sym_idx) return slots_[i].get();
        return nullptr;
    }

    [[nodiscard]] const SymbolSlot* slotFor(uint32_t sym_idx) const noexcept {
        for (uint32_t i = 0; i < slot_count_; ++i)
            if (slots_[i] && slots_[i]->sym_idx == sym_idx) return slots_[i].get();
        return nullptr;
    }

    static void matchLoop(SymbolSlot& slot, std::stop_token st);
};
