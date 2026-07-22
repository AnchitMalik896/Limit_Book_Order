#include "orderbook.h"
#include "symbol_table.h"
#include <algorithm>
#include <format>
#include <iostream>
#include <numeric>
#include <ranges>

OrderBook::OrderBook(uint32_t sym_idx) : sym_idx_(sym_idx) {
    scratch_.reserve(32);
    idx_.reserve(2048);
}

// ─── public entry point ───────────────────────────────────────────────────────

const std::vector<Trade>& OrderBook::processOrder(Order& o) {
    scratch_.clear();

    // FOK: pre-check liquidity before touching the book.
    if (o.is_fok()) {
        bool sufficient = o.is_buy()
            ? hasSufficientAskLiquidity(o.price, o.qty)
            : hasSufficientBidLiquidity(o.price, o.qty);
        if (!sufficient) {
            ++fok_cancels_;
            return scratch_;   // zero trades, order silently killed
        }
    }

    if (o.is_buy()) matchBuy(o, scratch_);
    else            matchSell(o, scratch_);

    // IOC: discard any unmatched remainder — never rest.
    // FOK: if we passed the pre-check, remainder should be zero; guard anyway.
    if (o.qty > 0) {
        if (o.is_ioc()) { ++ioc_cancels_; return scratch_; }
        if (o.is_fok()) { ++fok_cancels_; return scratch_; }
        // GTC: rest unmatched portion in the book.
        if (!o.is_market()) addOrder(o);
    }

    return scratch_;
}

// ─── FOK liquidity pre-check (read-only) ─────────────────────────────────────

bool OrderBook::hasSufficientAskLiquidity(int limit_price, int qty) const noexcept {
    int remaining = qty;
    for (auto& [px, q] : asks_) {
        if (px > limit_price) continue;   // price too high — skip
        for (auto& o : q) {
            remaining -= o.qty;
            if (remaining <= 0) return true;
        }
    }
    return false;
}

bool OrderBook::hasSufficientBidLiquidity(int limit_price, int qty) const noexcept {
    int remaining = qty;
    for (auto& [px, q] : bids_) {
        if (px < limit_price) continue;   // price too low — skip
        for (auto& o : q) {
            remaining -= o.qty;
            if (remaining <= 0) return true;
        }
    }
    return false;
}

// ─── matching ─────────────────────────────────────────────────────────────────

void OrderBook::matchBuy(Order& buy, std::vector<Trade>& out) {
    while (buy.qty > 0 && !asks_.empty()) {
        auto it           = asks_.begin();
        const int ask_px  = it->first;
        if (!buy.is_market() && buy.price < ask_px) break;

        auto& q = it->second;
        while (buy.qty > 0 && !q.empty()) {
            auto& rest      = q.front();
            const int exec  = std::min(buy.qty, rest.qty);
            emitTrade(out, buy, rest, ask_px, exec);
            buy.qty  -= exec;
            rest.qty -= exec;
            if (rest.qty == 0) { idx_.erase(rest.id); q.pop_front(); }
        }
        if (q.empty()) asks_.erase(it);
    }
}

void OrderBook::matchSell(Order& sell, std::vector<Trade>& out) {
    while (sell.qty > 0 && !bids_.empty()) {
        auto it           = bids_.begin();
        const int bid_px  = it->first;
        if (!sell.is_market() && sell.price > bid_px) break;

        auto& q = it->second;
        while (sell.qty > 0 && !q.empty()) {
            auto& rest      = q.front();
            const int exec  = std::min(sell.qty, rest.qty);
            emitTrade(out, rest, sell, bid_px, exec);
            sell.qty -= exec;
            rest.qty -= exec;
            if (rest.qty == 0) { idx_.erase(rest.id); q.pop_front(); }
        }
        if (q.empty()) bids_.erase(it);
    }
}

// ─── book management ──────────────────────────────────────────────────────────

void OrderBook::addOrder(Order& o) {
    idx_.emplace(o.id, std::pair{o.side, o.price});
    if (o.is_buy()) bids_[o.price].push_back(o);
    else            asks_[o.price].push_back(o);
}

void OrderBook::cancelOrder(uint64_t id) {
    auto it = idx_.find(id);
    if (it == idx_.end()) return;
    auto [side, price] = it->second;
    idx_.erase(it);
    if (side == Side::BUY) removeFromLevel(bids_, price, id);
    else                    removeFromLevel(asks_, price, id);
}

template<typename BookSide>
void OrderBook::removeFromLevel(BookSide& side, int price, uint64_t id) {
    auto it = side.find(price);
    if (it == side.end()) return;
    auto& q   = it->second;
    auto  pos = std::ranges::find_if(q, [id](const Order& o){ return o.id == id; });
    if (pos != q.end()) q.erase(pos);
    if (q.empty()) side.erase(it);
}

void OrderBook::emitTrade(std::vector<Trade>& out,
                           const Order& buy, const Order& sell,
                           int exec_price, int exec_qty) noexcept {
    out.push_back({buy.id, sell.id, now_ns(), exec_price, exec_qty, sym_idx_});
    vol_    += exec_qty;
    trades_ += 1;
}

// ─── queries ──────────────────────────────────────────────────────────────────

std::optional<int> OrderBook::bestBid() const noexcept {
    if (bids_.empty()) return std::nullopt;
    return bids_.begin()->first;
}

std::optional<int> OrderBook::bestAsk() const noexcept {
    if (asks_.empty()) return std::nullopt;
    return asks_.begin()->first;
}

std::optional<double> OrderBook::midPrice() const noexcept {
    auto bid = bestBid(), ask = bestAsk();
    if (!bid || !ask) return std::nullopt;
    return (*bid + *ask) / 2.0;
}

int OrderBook::spread() const noexcept {
    auto bid = bestBid(), ask = bestAsk();
    if (!bid || !ask) return -1;
    return *ask - *bid;
}

static long levelTotal(const std::deque<Order>& q) noexcept {
    return std::accumulate(q.begin(), q.end(), 0L,
        [](long a, const Order& o){ return a + o.qty; });
}

BookStats OrderBook::stats() const {
    BookStats s;
    if (auto b = bestBid()) s.best_bid = *b;
    if (auto a = bestAsk()) s.best_ask = *a;
    s.spread      = spread();
    if (auto m = midPrice()) s.mid = *m;
    for (auto& [px, q] : bids_) s.bid_depth += levelTotal(q);
    for (auto& [px, q] : asks_) s.ask_depth += levelTotal(q);
    s.vol_traded  = vol_;
    s.trade_cnt   = trades_;
    s.ioc_cancels = ioc_cancels_;
    s.fok_cancels = fok_cancels_;
    return s;
}

// ─── display ──────────────────────────────────────────────────────────────────

void OrderBook::printBook(int levels) const {
    const auto sym = g_symbols.name(sym_idx_);

    auto row   = [](std::string_view c) {
        std::cout << std::format("║  {:<28}║\n", c);
    };
    auto pxrow = [](int px, long qty) {
        std::cout << std::format("║  {:>7}  :  {:<16}║\n", px, qty);
    };

    std::cout << std::format("\n╔══════════════════════════════╗\n"
                             "║  BOOK: {:<22}║\n"
                             "╠══════════════════════════════╣\n", sym);
    row("ASKS");
    int shown = 0;
    for (auto it = asks_.rbegin(); it != asks_.rend() && shown < levels; ++it, ++shown)
        pxrow(it->first, levelTotal(it->second));
    if (asks_.empty()) row("(empty)");

    std::cout << "╠══════════════════════════════╣\n";
    if (auto m = midPrice())
        std::cout << std::format("║  mid={:<7.1f}  spd={:<10}║\n", *m, spread());
    else
        row("no mid");
    std::cout << "╠══════════════════════════════╣\n";

    row("BIDS");
    shown = 0;
    for (auto it = bids_.begin(); it != bids_.end() && shown < levels; ++it, ++shown)
        pxrow(it->first, levelTotal(it->second));
    if (bids_.empty()) row("(empty)");

    std::cout << std::format("╠══════════════════════════════╣\n"
                             "║  vol={:<6}  trades={:<8}║\n"
                             "║  ioc_cancel={:<4} fok_cancel={:<3}║\n"
                             "╚══════════════════════════════╝\n",
                             vol_, trades_, ioc_cancels_, fok_cancels_);
}
