#include "orderbook.h"
#include <algorithm>
#include <format>
#include <iostream>
#include <numeric>
#include <ranges>
#include <string>

OrderBook::OrderBook(std::string sym) : sym_(std::move(sym)) {}

std::vector<Trade> OrderBook::processOrder(Order& o) {
    auto trades = match(o);
    if (o.qty > 0 && !o.is_market()) addOrder(o);
    return trades;
}

void OrderBook::addOrder(Order& o) {
    idx_[o.id] = {o.side, o.price};
    if (o.is_buy()) bids_[o.price].push_back(o);
    else             asks_[o.price].push_back(o);
}

std::vector<Trade> OrderBook::match(Order& incoming) {
    return incoming.is_buy() ? matchBuy(incoming) : matchSell(incoming);
}

std::vector<Trade> OrderBook::matchBuy(Order& buy) {
    std::vector<Trade> out;
    while (buy.qty > 0 && !asks_.empty()) {
        auto it     = asks_.begin();
        int  ask_px = it->first;

        if (!buy.is_market() && buy.price < ask_px) break;

        auto& q = it->second;
        while (buy.qty > 0 && !q.empty()) {
            auto& resting  = q.front();
            int   exec_qty = std::min(buy.qty, resting.qty);
            recordTrade(out, buy, resting, ask_px, exec_qty);
            buy.qty     -= exec_qty;
            resting.qty -= exec_qty;
            if (resting.qty == 0) {
                idx_.erase(resting.id);
                q.pop_front();
            }
        }
        if (q.empty()) asks_.erase(it);
    }
    return out;
}

std::vector<Trade> OrderBook::matchSell(Order& sell) {
    std::vector<Trade> out;
    while (sell.qty > 0 && !bids_.empty()) {
        auto it     = bids_.begin();
        int  bid_px = it->first;

        if (!sell.is_market() && sell.price > bid_px) break;

        auto& q = it->second;
        while (sell.qty > 0 && !q.empty()) {
            auto& resting  = q.front();
            int   exec_qty = std::min(sell.qty, resting.qty);
            recordTrade(out, resting, sell, bid_px, exec_qty);
            sell.qty    -= exec_qty;
            resting.qty -= exec_qty;
            if (resting.qty == 0) {
                idx_.erase(resting.id);
                q.pop_front();
            }
        }
        if (q.empty()) bids_.erase(it);
    }
    return out;
}

void OrderBook::recordTrade(std::vector<Trade>& out,
                            const Order& buy, const Order& sell,
                            int exec_price, int exec_qty) {
    out.push_back({buy.id, sell.id, exec_price, exec_qty, now_ns()});
    vol_    += exec_qty;
    trades_ += 1;
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

std::optional<int> OrderBook::bestBid() const {
    if (bids_.empty()) return std::nullopt;
    return bids_.begin()->first;
}

std::optional<int> OrderBook::bestAsk() const {
    if (asks_.empty()) return std::nullopt;
    return asks_.begin()->first;
}

std::optional<double> OrderBook::midPrice() const {
    auto bid = bestBid();
    auto ask = bestAsk();
    if (!bid || !ask) return std::nullopt;
    return (*bid + *ask) / 2.0;
}

int OrderBook::spread() const {
    auto bid = bestBid();
    auto ask = bestAsk();
    if (!bid || !ask) return -1;
    return *ask - *bid;
}

static int levelTotal(const std::deque<Order>& q) {
    return std::accumulate(q.begin(), q.end(), 0,
        [](int acc, const Order& o){ return acc + o.qty; });
}

BookStats OrderBook::stats() const {
    BookStats s;
    if (auto b = bestBid()) s.best_bid = *b;
    if (auto a = bestAsk()) s.best_ask = *a;
    s.spread = spread();
    if (auto m = midPrice()) s.mid = *m;
    for (auto& [px, q] : bids_) s.bid_depth += levelTotal(q);
    for (auto& [px, q] : asks_) s.ask_depth += levelTotal(q);
    s.vol_traded  = vol_;
    s.trade_count = trades_;
    return s;
}

void OrderBook::printBook(int levels) const {
    constexpr std::string_view border = "╔══════════════════════════════╗";
    constexpr std::string_view div    = "╠══════════════════════════════╣";
    constexpr std::string_view end    = "╚══════════════════════════════╝";

    auto row = [](std::string_view content) {
        std::cout << std::format("║  {:<28}║\n", content);
    };
    auto pxrow = [](int px, int qty) {
        std::cout << std::format("║  {:>6}  :  {:<16}║\n", px, qty);
    };

    std::cout << '\n' << border << '\n';
    std::cout << std::format("║  BOOK: {:<22}║\n", sym_);
    std::cout << div << '\n';

    row("ASKS");
    int shown = 0;
    for (auto it = asks_.rbegin(); it != asks_.rend() && shown < levels; ++it, ++shown)
        pxrow(it->first, levelTotal(it->second));
    if (asks_.empty()) row("(empty)");

    std::cout << div << '\n';
    if (auto m = midPrice())
        std::cout << std::format("║  mid={:<7.1f}  spd={:<10}║\n", *m, spread());
    else
        row("no mid");
    std::cout << div << '\n';

    row("BIDS");
    shown = 0;
    for (auto it = bids_.begin(); it != bids_.end() && shown < levels; ++it, ++shown)
        pxrow(it->first, levelTotal(it->second));
    if (bids_.empty()) row("(empty)");

    std::cout << div << '\n';
    std::cout << std::format("║  vol={:<6}  trades={:<8}║\n", vol_, trades_);
    std::cout << end << '\n';
}
