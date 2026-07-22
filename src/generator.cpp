#include "generator.h"
#include <stdexcept>

OrderGenerator::OrderGenerator(uint64_t seed) : rng_(seed) {}

void OrderGenerator::addSymbol(const SymbolConfig& cfg) {
    cfgs_.push_back(cfg);
    sym_pick_ = std::uniform_int_distribution<int>(0, (int)cfgs_.size() - 1);
}

TimeInForce OrderGenerator::pickTif(const SymbolConfig& cfg) {
    const double r = unit_(rng_);
    if (r < cfg.fok_prob)                      return TimeInForce::FOK;
    if (r < cfg.fok_prob + cfg.ioc_prob)       return TimeInForce::IOC;
    return TimeInForce::GTC;
}

Order OrderGenerator::next() {
    if (cfgs_.empty()) throw std::runtime_error("no symbols configured");
    auto& cfg = cfgs_[sym_pick_(rng_)];
    if (unit_(rng_) < cfg.market_prob) return makeMarket(cfg);
    return makeLimit(cfg, pickTif(cfg));
}

Order OrderGenerator::makeLimit(const SymbolConfig& cfg, TimeInForce tif) {
    const bool is_buy = unit_(rng_) < 0.5;
    const int  half   = cfg.spread_ticks / 2;

    std::uniform_int_distribution<int> offset(0, cfg.spread_ticks + 3);
    std::uniform_int_distribution<int> vol(cfg.vol_min, cfg.vol_max);

    int price = is_buy
        ? cfg.mid_price - half - offset(rng_) * cfg.tick
        : cfg.mid_price + half + offset(rng_) * cfg.tick;
    price = std::max(price, cfg.tick);

    const uint32_t idx = g_symbols.intern(cfg.sym);
    return Order{
        .id       = 0,
        .price    = price,
        .qty      = vol(rng_),
        .orig_qty = 0,
        .ts       = 0,
        .sym_idx  = idx,
        .side     = is_buy ? Side::BUY : Side::SELL,
        .type     = OrderType::LIMIT,
        .tif      = tif,
    };
}

Order OrderGenerator::makeMarket(const SymbolConfig& cfg) {
    const bool is_buy = unit_(rng_) < 0.5;
    std::uniform_int_distribution<int> vol(cfg.vol_min, cfg.vol_max / 2);

    const uint32_t idx = g_symbols.intern(cfg.sym);
    return Order{
        .id       = 0,
        .price    = 0,
        .qty      = vol(rng_),
        .orig_qty = 0,
        .ts       = 0,
        .sym_idx  = idx,
        .side     = is_buy ? Side::BUY : Side::SELL,
        .type     = OrderType::MARKET,
        .tif      = TimeInForce::IOC,  // market orders are always IOC semantics
    };
}

void OrderGenerator::drift(double alpha) {
    std::normal_distribution<double> noise(0.0, 1.0);
    for (auto& cfg : cfgs_) {
        double delta  = alpha * cfg.mid_price * noise(rng_);
        cfg.mid_price = std::max(cfg.tick, cfg.mid_price + (int)delta);
    }
}
