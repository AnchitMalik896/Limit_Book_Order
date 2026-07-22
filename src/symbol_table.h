#pragma once
#include <array>
#include <cstdint>
#include <stdexcept>
#include <string>
#include <string_view>

static constexpr uint32_t MAX_SYMBOLS = 16;

class SymbolTable {
    std::array<std::string, MAX_SYMBOLS> names_;
    uint32_t count_ = 0;
    bool frozen_ = false;

public:
    void freeze() noexcept {
        frozen_ = true;
    }

    [[nodiscard]] bool is_frozen() const noexcept {
        return frozen_;
    }

    [[nodiscard]] uint32_t intern(std::string_view sym) {
        for (uint32_t i = 0; i < count_; ++i)
            if (names_[i] == sym) return i;
        if (frozen_)
            throw std::runtime_error("cannot intern new symbols after symbol table is frozen");
        if (count_ == MAX_SYMBOLS)
            throw std::runtime_error("symbol table full");
        names_[count_] = sym;
        return count_++;
    }

    [[nodiscard]] std::string_view name(uint32_t idx) const noexcept {
        return names_[idx];
    }

    [[nodiscard]] uint32_t count() const noexcept { return count_; }
};

inline SymbolTable g_symbols;
