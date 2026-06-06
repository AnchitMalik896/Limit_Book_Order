#pragma once
#include "order.h"
#include <vector>
#include <string>
#include <random>
#include <unordered_map>

struct SymbolConfig {
    std::string sym;
    int         mid_price;
    int         tick;
    int         spread_ticks;
    int         vol_min;
    int         vol_max;
    double      market_order_prob;
};

class OrderGenerator {
public:
    explicit OrderGenerator(uint64_t seed = 42);

    void addSymbol(SymbolConfig cfg);
    Order next();
    void  drift(double alpha = 0.005);
    const std::vector<SymbolConfig>& symbols() const { return cfgs_; }

private:
    std::mt19937                          rng_;
    std::vector<SymbolConfig>             cfgs_;
    std::uniform_int_distribution<int>    sym_pick_;
    std::uniform_real_distribution<double> unit_{0.0, 1.0};

    Order makeLimit(const SymbolConfig& cfg);
    Order makeMarket(const SymbolConfig& cfg);
};
