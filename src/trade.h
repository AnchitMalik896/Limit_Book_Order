#pragma once
#include <cstdint>
#include <format>
#include <iostream>

struct Trade {
    uint64_t buy_id;
    uint64_t sell_id;
    int      price;
    int      qty;
    int64_t  ts;

    void print() const {
        std::cout << std::format(
            "TRADE: BUY_ID={:<6}  SELL_ID={:<6}  PRICE={:<6}  QTY={}\n",
            buy_id, sell_id, price, qty
        );
    }
};
