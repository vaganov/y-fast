#ifndef _YFAST_UTILS_ALIGNED_H
#define _YFAST_UTILS_ALIGNED_H

#include <cstdint>

namespace yfast::utils {

template <typename T>
struct align_traits;

template <>
struct align_traits<void> {
    static constexpr std::size_t ptr_alignment = 1;
};

template <typename T>
struct align_traits {
    static constexpr auto ptr_alignment = alignof(T);
};

template <unsigned int N, typename T = void>
struct aligned_ptr {
    static_assert(align_traits<T>::ptr_alignment % (1 << N) == 0, "Pointer alignment error");

    static constexpr auto Bits = N;

    static constexpr std::uintptr_t BIT_MASK = (1 << N) - 1;
    static constexpr std::uintptr_t PTR_MASK = ~BIT_MASK;

    std::uintptr_t value;

    aligned_ptr(std::uintptr_t value): value(value) {}

    explicit aligned_ptr(T* ptr = nullptr): value(reinterpret_cast<std::uintptr_t>(ptr)) {}

    template <typename... Args>
    explicit aligned_ptr(T* ptr, Args... args): value(reinterpret_cast<std::uintptr_t>(ptr)) {
        static_assert(sizeof...(Args) == N, "Arguments mismatch");
        const bool bits[] = { static_cast<bool>(args)... };
        for (unsigned int n = 0; n < N; ++n) {
            if (bits[n]) {
                set_bit(n);
            }
        }
    }

    T* get_ptr() const { return reinterpret_cast<T*>(value & PTR_MASK); }
    [[nodiscard]] bool get_bit(unsigned int n) const { return value & (1 << n); }

    void set_ptr(T* ptr) { value = reinterpret_cast<std::uintptr_t>(ptr) | (value & BIT_MASK); }
    void set_bit(unsigned int n, bool bit) {
        const std::uintptr_t mask = 1 << n;
        if (bit) {
            value |= mask;
        }
        else {
            value &= ~mask;
        }
    }
    void set_bit(unsigned int n) {
        const std::uintptr_t mask = 1 << n;
        value |= mask;
    }
    void clear_bit(unsigned int n) {
        const std::uintptr_t mask = 1 << n;
        value &= ~mask;
    }
};

}

#endif
