#pragma once

#include <cstdint>

class DebouncedButton {
public:
    explicit DebouncedButton(std::uint64_t debounce_us = 30'000)
        : debounce_us_(debounce_us) {}

    bool update(bool pressed, std::uint64_t now_us)
    {
        if (pressed != observed_) {
            observed_ = pressed;
            changed_us_ = now_us;
        }
        if (observed_ == stable_ || now_us - changed_us_ < debounce_us_) return false;
        stable_ = observed_;
        return stable_;
    }

private:
    std::uint64_t debounce_us_;
    std::uint64_t changed_us_{};
    bool observed_{};
    bool stable_{};
};
