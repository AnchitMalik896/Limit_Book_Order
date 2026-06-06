#pragma once
#include "orderbook.h"
#include <unordered_map>
#include <string>
#include <vector>
#include <atomic>
#include <cstdint>

class Exchange {
public:
    Exchange();

    uint64_t           submit(Order o);
    bool               cancel(const std::string& sym, uint64_t id);
    void               printAll(int levels = 5) const;
    void               printStats() const;
    const OrderBook&   book(const std::string& sym) const;
    bool               hasSymbol(const std::string& sym) const;
    std::vector<Trade> lastTrades() const { return last_trades_; }

private:
    std::unordered_map<std::string, OrderBook> books_;
    std::vector<Trade>                          last_trades_;
    std::atomic<uint64_t>                       next_id_{1};

    OrderBook& getOrCreate(const std::string& sym);
};
