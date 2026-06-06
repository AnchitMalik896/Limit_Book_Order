#pragma once
#include "order.h"
#include "trade.h"
#include <map>
#include <deque>
#include <vector>
#include <unordered_map>
#include <functional>
#include <optional>
#include <string>

struct BookStats {
    int    best_bid  = 0;
    int    best_ask  = 0;
    int    spread    = 0;
    double mid       = 0.0;
    int    bid_depth = 0;
    int    ask_depth = 0;
    long   vol_traded = 0;
    long   trade_count = 0;
};

class OrderBook {
public:
    explicit OrderBook(std::string sym);

    std::vector<Trade> processOrder(Order& o);
    void               cancelOrder(uint64_t id);
    void               printBook(int levels = 5) const;

    BookStats          stats()    const;
    std::optional<int> bestBid()  const;
    std::optional<int> bestAsk()  const;
    std::optional<double> midPrice() const;
    int                spread()   const;
    long               volume()   const { return vol_; }
    long               trades()   const { return trades_; }

private:
    using BidMap = std::map<int, std::deque<Order>, std::greater<int>>;
    using AskMap = std::map<int, std::deque<Order>>;

    std::string  sym_;
    BidMap       bids_;
    AskMap       asks_;
    long         vol_    = 0;
    long         trades_ = 0;

    std::unordered_map<uint64_t, std::pair<Side, int>> idx_;

    void               addOrder(Order& o);
    std::vector<Trade> match(Order& incoming);
    std::vector<Trade> matchBuy(Order& buy);
    std::vector<Trade> matchSell(Order& sell);

    template<typename BookSide>
    void removeFromLevel(BookSide& side, int price, uint64_t id);

    void recordTrade(std::vector<Trade>& out,
                     const Order& buy, const Order& sell,
                     int exec_price, int exec_qty);
};
