#pragma once
#include <cstdint>
#include <format>
#include <iostream>
#include <string_view>

struct Trade {
    uint64_t buy_id;
    uint64_t sell_id;
    int64_t  ts;
    int      price;
    int      qty;
    uint32_t sym_idx;

    void print(std::string_view sym) const {
        std::cout << std::format(
            "TRADE [{:<4}]  BUY={:<6}  SELL={:<6}  PRICE={:<7}  QTY={}\n",
            sym, buy_id, sell_id, price, qty);
    }
};
