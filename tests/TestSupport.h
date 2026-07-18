#pragma once

#include <SimdLib/Api.h>

#include <catch2/catch_test_macros.hpp>

#include <algorithm>
#include <array>
#include <bit>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <span>

namespace SimdLib::Tests
{
template <std::size_t Width, class Element>
void require_addition_parity()
{
    using simd = Api<Width, Element>;
    std::array<Element, simd::element_count> lhs{};
    std::array<Element, simd::element_count> rhs{};
    std::array<Element, simd::element_count> expected{};
    for (std::size_t index = 0; index < simd::element_count; ++index)
    {
        lhs[index] = static_cast<Element>(index + 1);
        rhs[index] = static_cast<Element>(2);
        expected[index] = static_cast<Element>(index + 3);
    }
    REQUIRE(simd::to_array(simd::add(simd::load(lhs), simd::load(rhs))) == expected);
}

template <std::size_t Width>
void require_supported_addition_matrix()
{
    require_addition_parity<Width, std::int8_t>();
    require_addition_parity<Width, std::uint8_t>();
    require_addition_parity<Width, std::int16_t>();
    require_addition_parity<Width, std::uint16_t>();
    require_addition_parity<Width, std::int32_t>();
    require_addition_parity<Width, std::uint32_t>();
    require_addition_parity<Width, std::int64_t>();
    require_addition_parity<Width, std::uint64_t>();
    require_addition_parity<Width, float>();
    require_addition_parity<Width, double>();
}

template <std::size_t Width, class Element>
void require_transfer_contracts()
{
    using simd = Api<Width, Element>;
    alignas(Width / 8) std::array<Element, simd::element_count> aligned{};
    for (std::size_t index = 0; index < aligned.size(); ++index)
        aligned[index] = static_cast<Element>(index + 1);

    const auto aligned_register = simd::load_aligned(aligned);
    alignas(Width / 8) std::array<Element, simd::element_count> aligned_output{};
    simd::store_aligned(aligned_register, aligned_output);
    REQUIRE(aligned_output == aligned);

    alignas(64) std::array<Element, simd::element_count + 1> offset_storage{};
    std::copy(aligned.begin(), aligned.end(), offset_storage.begin() + 1);
    const std::span<const Element, simd::element_count> unaligned_input{offset_storage.data() + 1, simd::element_count};
    const auto unaligned_register = simd::load_unaligned(unaligned_input);

    alignas(64) std::array<Element, simd::element_count + 1> offset_output{};
    std::span<Element, simd::element_count> unaligned_output{offset_output.data() + 1, simd::element_count};
    simd::store_unaligned(unaligned_register, unaligned_output);
    REQUIRE(std::equal(aligned.begin(), aligned.end(), unaligned_output.begin()));

    std::array<std::byte, simd::byte_count> bytes{};
    simd::store(unaligned_register, std::span<std::byte>{bytes});
    REQUIRE(bytes.size() == simd::byte_count);

    std::array<std::byte, simd::byte_count + 8> oversized_bytes{};
    simd::store(unaligned_register, std::span<std::byte>{oversized_bytes});
    std::array<Element, simd::element_count> recovered{};
    std::memcpy(recovered.data(), oversized_bytes.data(), simd::byte_count);
    REQUIRE(recovered == aligned);
}

template <std::size_t Width>
void require_supported_transfer_matrix()
{
    require_transfer_contracts<Width, std::int8_t>();
    require_transfer_contracts<Width, std::uint8_t>();
    require_transfer_contracts<Width, std::int16_t>();
    require_transfer_contracts<Width, std::uint16_t>();
    require_transfer_contracts<Width, std::int32_t>();
    require_transfer_contracts<Width, std::uint32_t>();
    require_transfer_contracts<Width, std::int64_t>();
    require_transfer_contracts<Width, std::uint64_t>();
    require_transfer_contracts<Width, float>();
    require_transfer_contracts<Width, double>();
}

template <std::size_t Width, class Element>
constexpr auto movemask_test_bytes()
{
    std::array<std::uint8_t, Width / 8> bytes{};
    for (std::size_t index = 0; index < bytes.size(); ++index)
        bytes[index] = static_cast<std::uint8_t>((index * 19u) | (index % 3u == 1u ? 0u : 0x80u));
    return bytes;
}

template <std::size_t Width, class Element>
constexpr auto movemask_test_values()
{
    using simd = Api<Width, Element>;
    constexpr auto bytes = movemask_test_bytes<Width, Element>();
    static_assert(sizeof(bytes) == sizeof(std::array<Element, simd::element_count>));
    return std::bit_cast<std::array<Element, simd::element_count>>(bytes);
}

template <std::size_t Width, class Element>
constexpr auto expected_byte_movemask()
{
    using simd = Api<Width, Element>;
    constexpr auto bytes = movemask_test_bytes<Width, Element>();
    typename simd::mask_t result = 0;
    for (std::size_t index = 0; index < bytes.size(); ++index)
        result |= static_cast<typename simd::mask_t>((bytes[index] >> 7) & 1u) << index;
    return result;
}

template <std::size_t Width, class Element>
constexpr auto expected_slim_movemask()
{
    using simd = Api<Width, Element>;
    constexpr auto bytes = movemask_test_bytes<Width, Element>();
    typename simd::mask_t result = 0;
    for (std::size_t index = 0; index < simd::element_count; ++index)
    {
        const std::size_t sign_byte = (index + 1) * sizeof(Element) - 1;
        result |= static_cast<typename simd::mask_t>((bytes[sign_byte] >> 7) & 1u) << index;
    }
    return result;
}

template <std::size_t Width, class Element>
void require_movemask_contract()
{
    using simd = Api<Width, Element>;
    const auto value = simd::construct(movemask_test_values<Width, Element>());
    REQUIRE(simd::movemask(value) == expected_byte_movemask<Width, Element>());
    REQUIRE(simd::movemask_slim(value) == expected_slim_movemask<Width, Element>());
}

template <std::size_t Width>
void require_supported_movemask_matrix()
{
    require_movemask_contract<Width, std::int8_t>();
    require_movemask_contract<Width, std::uint8_t>();
    require_movemask_contract<Width, std::int16_t>();
    require_movemask_contract<Width, std::uint16_t>();
    require_movemask_contract<Width, std::int32_t>();
    require_movemask_contract<Width, std::uint32_t>();
    require_movemask_contract<Width, std::int64_t>();
    require_movemask_contract<Width, std::uint64_t>();
    require_movemask_contract<Width, float>();
    require_movemask_contract<Width, double>();
}

template <std::size_t Width, class Element>
consteval bool constexpr_movemask_contract()
{
    using simd = Api<Width, Element>;
    constexpr auto value = simd::construct(movemask_test_values<Width, Element>());
    return simd::movemask(value) == expected_byte_movemask<Width, Element>() &&
           simd::movemask_slim(value) == expected_slim_movemask<Width, Element>();
}
} // namespace SimdLib::Tests
