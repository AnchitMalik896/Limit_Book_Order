#include "generator.h"
#include <cassert>
#include <stdexcept>

OrderGenerator::OrderGenerator(uint64_t seed)
    : rng_(seed), sym_pick_(0, 0) {}

void OrderGenerator::addSymbol(SymbolConfig cfg) {
    cfgs_.push_back(std::move(cfg));
    sym_pick_ = std::uniform_int_distribution<int>(0, (int)cfgs_.size() - 1);
}

Order OrderGenerator::next() {
    if (cfgs_.empty()) throw std::runtime_error("no symbols configured");
    auto& cfg = cfgs_[sym_pick_(rng_)];
    if (unit_(rng_) < cfg.market_order_prob) return makeMarket(cfg);
    return makeLimit(cfg);
}

Order OrderGenerator::makeLimit(const SymbolConfig& cfg) {
    bool is_buy = unit_(rng_) < 0.5;
    int  half   = cfg.spread_ticks / 2;

    std::uniform_int_distribution<int> offset(0, cfg.spread_ticks + 3);
    std::uniform_int_distribution<int> vol(cfg.vol_min, cfg.vol_max);

    int price = is_buy
        ? cfg.mid_price - half - offset(rng_) * cfg.tick
        : cfg.mid_price + half + offset(rng_) * cfg.tick;

    price = std::max(price, cfg.tick);

    return Order{
        .id       = 0,
        .sym      = cfg.sym,
        .side     = is_buy ? Side::BUY : Side::SELL,
        .type     = OrderType::LIMIT,
        .price    = price,
        .qty      = vol(rng_),
        .orig_qty = 0,
        .ts       = 0
    };
}

Order OrderGenerator::makeMarket(const SymbolConfig& cfg) {
    bool is_buy = unit_(rng_) < 0.5;
    std::uniform_int_distribution<int> vol(cfg.vol_min, cfg.vol_max / 2);

    return Order{
        .id       = 0,
        .sym      = cfg.sym,
        .side     = is_buy ? Side::BUY : Side::SELL,
        .type     = OrderType::MARKET,
        .price    = 0,
        .qty      = vol(rng_),
        .orig_qty = 0,
        .ts       = 0
    };
}

void OrderGenerator::drift(double alpha) {
    std::normal_distribution<double> noise(0.0, 1.0);
    for (auto& cfg : cfgs_) {
        double delta = alpha * cfg.mid_price * noise(rng_);
        cfg.mid_price = std::max(cfg.tick, cfg.mid_price + (int)delta);
    }
}
