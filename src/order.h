#pragma once
#include <string>
#include <cstdint>
#include <chrono>

enum class Side { BUY, SELL };
enum class OrderType { LIMIT, MARKET };

struct Order {
    uint64_t    id;
    std::string sym;
    Side        side;
    OrderType   type;
    int         price;
    int         qty;
    int         orig_qty;
    int64_t     ts;

    bool is_buy()  const { return side == Side::BUY; }
    bool is_sell() const { return side == Side::SELL; }
    bool is_market() const { return type == OrderType::MARKET; }
    int  filled()  const { return orig_qty - qty; }
};

inline int64_t now_ns() {
    return std::chrono::duration_cast<std::chrono::nanoseconds>(
        std::chrono::steady_clock::now().time_since_epoch()
    ).count();
}
