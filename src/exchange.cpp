#include "exchange.h"
#include <format>
#include <iostream>
#include <stdexcept>
#include <string>

Exchange::Exchange() = default;

OrderBook& Exchange::getOrCreate(const std::string& sym) {
    return books_.try_emplace(sym, sym).first->second;
}

uint64_t Exchange::submit(Order o) {
    o.id       = next_id_.fetch_add(1, std::memory_order_relaxed);
    o.ts       = now_ns();
    o.orig_qty = o.qty;

    last_trades_ = getOrCreate(o.sym).processOrder(o);
    return o.id;
}

bool Exchange::cancel(const std::string& sym, uint64_t id) {
    auto it = books_.find(sym);
    if (it == books_.end()) return false;
    it->second.cancelOrder(id);
    return true;
}

const OrderBook& Exchange::book(const std::string& sym) const {
    auto it = books_.find(sym);
    if (it == books_.end()) throw std::runtime_error("unknown symbol: " + sym);
    return it->second;
}

bool Exchange::hasSymbol(const std::string& sym) const {
    return books_.contains(sym);
}

void Exchange::printAll(int levels) const {
    for (auto& [sym, b] : books_) b.printBook(levels);
}

void Exchange::printStats() const {
    constexpr std::string_view hdr =
        "{:<8} {:>8} {:>8} {:>8} {:>8} {:>10} {:>8}\n";
    constexpr std::string_view row =
        "{:<8} {:>8} {:>8} {:>8} {:>8.1f} {:>10} {:>8}\n";

    std::cout << '\n'
              << std::format(hdr, "SYMBOL","BID","ASK","SPREAD","MID","VOLUME","TRADES")
              << std::string(66, '-') << '\n';

    for (auto& [sym, b] : books_) {
        auto s = b.stats();
        std::cout << std::format(row,
            sym, s.best_bid, s.best_ask, s.spread, s.mid, s.vol_traded, s.trade_count);
    }
}
