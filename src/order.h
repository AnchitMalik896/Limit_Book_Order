#pragma once
#include <cstdint>
#include <chrono>

enum class Side        : uint8_t { BUY, SELL };
enum class OrderType   : uint8_t { LIMIT, MARKET };
enum class TimeInForce : uint8_t {
    GTC,    // Good-Till-Cancel  — rests in book (default)
    IOC,    // Immediate-Or-Cancel — fill what crosses, cancel remainder
    FOK     // Fill-Or-Kill       — fill entire qty or cancel whole order
};

struct alignas(64) Order {
    uint64_t     id;
    int          price;
    int          qty;
    int          orig_qty;
    int64_t      ts;
    uint32_t     sym_idx;
    Side         side;
    OrderType    type;
    TimeInForce  tif;
    uint8_t      _pad[1]{};

    [[nodiscard]] bool is_buy()    const noexcept { return side == Side::BUY; }
    [[nodiscard]] bool is_sell()   const noexcept { return side == Side::SELL; }
    [[nodiscard]] bool is_market() const noexcept { return type == OrderType::MARKET; }
    [[nodiscard]] bool is_ioc()    const noexcept { return tif  == TimeInForce::IOC; }
    [[nodiscard]] bool is_fok()    const noexcept { return tif  == TimeInForce::FOK; }
    [[nodiscard]] bool is_gtc()    const noexcept { return tif  == TimeInForce::GTC; }
    [[nodiscard]] int  filled()    const noexcept { return orig_qty - qty; }
};

[[nodiscard]] inline int64_t now_ns() noexcept {
    return std::chrono::duration_cast<std::chrono::nanoseconds>(
        std::chrono::steady_clock::now().time_since_epoch()
    ).count();
}
