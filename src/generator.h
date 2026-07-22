#pragma once
#include "order.h"
#include "symbol_table.h"
#include <random>
#include <string_view>
#include <vector>

struct SymbolConfig {
    std::string_view sym;
    int              mid_price;
    int              tick;
    int              spread_ticks;
    int              vol_min;
    int              vol_max;
    double           market_prob;  // probability of MARKET order
    double           ioc_prob;     // probability of IOC (from remaining limit orders)
    double           fok_prob;     // probability of FOK (from remaining limit orders)
};

class OrderGenerator {
public:
    explicit OrderGenerator(uint64_t seed = 42);

    void  addSymbol(const SymbolConfig& cfg);
    Order next();
    void  drift(double alpha = 0.005);

    const std::vector<SymbolConfig>& symbols() const noexcept { return cfgs_; }

private:
    std::mt19937                           rng_;
    std::vector<SymbolConfig>              cfgs_;
    std::uniform_int_distribution<int>     sym_pick_{0, 0};
    std::uniform_real_distribution<double> unit_{0.0, 1.0};

    Order makeLimit (const SymbolConfig& cfg, TimeInForce tif);
    Order makeMarket(const SymbolConfig& cfg);

    TimeInForce pickTif(const SymbolConfig& cfg);
};
