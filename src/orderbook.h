#pragma once
#include "order.h"
#include "trade.h"
#include <cstdint>
#include <deque>
#include <functional>
#include <map>
#include <optional>
#include <string_view>
#include <unordered_map>
#include <vector>

struct BookStats {
    int    best_bid   = 0;
    int    best_ask   = 0;
    int    spread     = 0;
    double mid        = 0.0;
    long   bid_depth  = 0;
    long   ask_depth  = 0;
    long   vol_traded = 0;
    long   trade_cnt  = 0;
    long   ioc_cancels = 0;
    long   fok_cancels = 0;
};

class OrderBook {
public:
    explicit OrderBook(uint32_t sym_idx);

    // Returns reference to internal scratch vector — valid until next call.
    const std::vector<Trade>& processOrder(Order& o);
    void                      cancelOrder(uint64_t id);
    void                      printBook(int levels = 5) const;

    [[nodiscard]] BookStats            stats()    const;
    [[nodiscard]] std::optional<int>   bestBid()  const noexcept;
    [[nodiscard]] std::optional<int>   bestAsk()  const noexcept;
    [[nodiscard]] std::optional<double> midPrice() const noexcept;
    [[nodiscard]] int  spread()  const noexcept;
    [[nodiscard]] long volume()  const noexcept { return vol_; }
    [[nodiscard]] long trades()  const noexcept { return trades_; }

private:
    using Level     = std::deque<Order>;
    using BidMap    = std::map<int, Level, std::greater<int>>;
    using AskMap    = std::map<int, Level>;
    using CancelIdx = std::unordered_map<uint64_t, std::pair<Side, int>>;

    uint32_t   sym_idx_;
    BidMap     bids_;
    AskMap     asks_;
    CancelIdx  idx_;
    long       vol_         = 0;
    long       trades_      = 0;
    long       ioc_cancels_ = 0;
    long       fok_cancels_ = 0;

    std::vector<Trade> scratch_;

    void addOrder(Order& o);

    // Core match — fills as much as possible, respects price, modifies o.qty.
    void matchBuy (Order& buy,  std::vector<Trade>& out);
    void matchSell(Order& sell, std::vector<Trade>& out);

    // FOK liquidity check — walks book without modifying anything.
    [[nodiscard]] bool hasSufficientBidLiquidity(int price, int qty) const noexcept;
    [[nodiscard]] bool hasSufficientAskLiquidity(int price, int qty) const noexcept;

    template<typename BookSide>
    void removeFromLevel(BookSide& side, int price, uint64_t id);

    void emitTrade(std::vector<Trade>& out,
                   const Order& buy, const Order& sell,
                   int exec_price, int exec_qty) noexcept;
};
