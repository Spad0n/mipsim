#ifndef CTL_TYPES_HPP
#define CTL_TYPES_HPP

#include "info.hpp"
#include <stdint.h> // uint8_t, uint16_t, uint32_t, uint64_t, int8_t, int16_t, int32_t, int64_t, uintptr_t

struct Nat {};
inline void *operator new(decltype(sizeof 0), void* ptr, Nat) {
    return ptr;
}

namespace ctl {

    using Uint8   = uint8_t;
    using Sint8   = int8_t;
    using Uint16  = uint16_t;
    using Sint16  = int16_t;
    using Uint32  = uint32_t;
    using Sint32  = int32_t;
    using Uint64  = uint64_t;
    using Sint64  = int64_t;
    using Float64 = double;
    using Float32 = float;

    /// @brief Unsigned size type (equivalent to size_t).
    using Ulen = decltype(sizeof 0);
    using Bool = bool;

    using Address = uintptr_t;
    using Hash    = Uint64;
    using Unit    = struct {};

    static_assert(sizeof(Address) == sizeof(void*), "Address must match pointer size");

    constexpr Uint8 operator""_u8(unsigned long long int v) { return static_cast<Uint8>(v); }
    constexpr Uint16 operator""_u16(unsigned long long int v) { return static_cast<Uint16>(v); }
    constexpr Uint32 operator""_u32(unsigned long long int v) { return static_cast<Uint32>(v); }
    constexpr Uint64 operator""_u64(unsigned long long int v) { return static_cast<Uint64>(v); }
    constexpr Ulen operator""_ulen(unsigned long long int v) { return static_cast<Ulen>(v); }

    // Helpers for working with lo and hi parts of integer types.

    /// @brief Returns the lower half of the integer.
    constexpr auto lo(Uint16 v) -> Uint8  { return static_cast<Uint8>(v); }

    /// @brief Returns the upper half of the integer.
    constexpr auto hi(Uint16 v) -> Uint8  { return static_cast<Uint8>(v >> 8); }

    constexpr auto lo(Uint32 v) -> Uint16 { return static_cast<Uint16>(v); }
    constexpr auto hi(Uint32 v) -> Uint16 { return static_cast<Uint16>(v >> 16); }
    constexpr auto lo(Uint64 v) -> Uint32 { return static_cast<Uint32>(v); }
    constexpr auto hi(Uint64 v) -> Uint32 { return static_cast<Uint32>(v >> 32); }

    constexpr auto lo(Sint16 v) -> Uint8  { return lo(Uint16(v)); }
    constexpr auto hi(Sint16 v) -> Uint8  { return hi(Uint16(v)); }
    constexpr auto lo(Sint32 v) -> Uint16 { return lo(Uint32(v)); }
    constexpr auto hi(Sint32 v) -> Uint16 { return hi(Uint32(v)); }
    constexpr auto lo(Sint64 v) -> Uint32 { return lo(Uint64(v)); }
    constexpr auto hi(Sint64 v) -> Uint32 { return hi(Uint64(v)); }

    template<typename T>
    struct Identity { using Type = T; };

} // namespace ctl

#endif // CTL_TYPES_HPP
