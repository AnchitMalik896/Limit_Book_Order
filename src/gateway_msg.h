#pragma once
#include "order.h"
#include <cstdint>
#include <variant>

struct CancelRequest {
    uint64_t id;
    uint32_t sym_idx;
};

using GatewayMsg = std::variant<Order, CancelRequest>;
