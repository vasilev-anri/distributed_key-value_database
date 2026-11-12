#pragma once

// Bounds-checked big-endian (network byte order) reading and writing.

#include <bit>
#include <concepts>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <optional>
#include <span>
#include <string_view>
#include <utility>
#include <vector>

namespace protocol {

// Converts between host and big-endian order (the same operation both ways).
template <std::unsigned_integral T>
constexpr T to_big_endian(T value) noexcept {
    if constexpr (sizeof(T) > 1 && std::endian::native == std::endian::little) {
        return std::byteswap(value);
    } else {
        return value;
    }
}

class ByteWriter {
public:
    void u8(std::uint8_t v) { put(v); }
    void u16(std::uint16_t v) { put(v); }
    void u32(std::uint32_t v) { put(v); }
    void u64(std::uint64_t v) { put(v); }

    void bytes(std::span<const std::byte> data) { buf_.insert(buf_.end(), data.begin(), data.end()); }
    void bytes(std::string_view text) { bytes(std::as_bytes(std::span(text.data(), text.size()))); }

    [[nodiscard]] std::size_t size() const noexcept { return buf_.size(); }
    [[nodiscard]] std::vector<std::byte> take() noexcept { return std::move(buf_); }

private:
    template <std::unsigned_integral T>
    void put(T value) {
        value = to_big_endian(value);
        const auto* p = reinterpret_cast<const std::byte*>(&value);
        buf_.insert(buf_.end(), p, p + sizeof value);
    }

    std::vector<std::byte> buf_;
};

// Every read returns std::nullopt instead of reading past the end.
class ByteReader {
public:
    explicit ByteReader(std::span<const std::byte> data) noexcept : data_(data) {}

    std::optional<std::uint8_t> u8() noexcept { return get<std::uint8_t>(); }
    std::optional<std::uint16_t> u16() noexcept { return get<std::uint16_t>(); }
    std::optional<std::uint32_t> u32() noexcept { return get<std::uint32_t>(); }
    std::optional<std::uint64_t> u64() noexcept { return get<std::uint64_t>(); }

    std::optional<std::span<const std::byte>> bytes(std::size_t n) noexcept {
        if (remaining() < n) return std::nullopt;
        const auto out = data_.subspan(pos_, n);
        pos_ += n;
        return out;
    }

    [[nodiscard]] std::size_t remaining() const noexcept { return data_.size() - pos_; }
    [[nodiscard]] bool empty() const noexcept { return remaining() == 0; }

private:
    template <std::unsigned_integral T>
    std::optional<T> get() noexcept {
        if (remaining() < sizeof(T)) return std::nullopt;
        T value;
        std::memcpy(&value, data_.data() + pos_, sizeof value);  // safe for unaligned data
        pos_ += sizeof value;
        return to_big_endian(value);
    }

    std::span<const std::byte> data_;
    std::size_t pos_ = 0;
};

}  // namespace protocol
