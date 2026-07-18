#pragma once

#include <SimdLib/Api.h>

#include <catch2/catch_test_macros.hpp>

#include <algorithm>
#include <array>
#include <bit>
#include <concepts>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <limits>
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
void require_partial_transfer_contracts()
{
    using simd = Api<Width, Element>;
    alignas(64) std::array<Element, simd::element_count + 1> storage{};
    for (std::size_t index = 0; index < simd::element_count; ++index)
        storage[index + 1] = static_cast<Element>(index + 1);

    const std::span<const Element> unaligned{storage.data() + 1, simd::element_count};
    const auto none = simd::template load_partial<0>(unaligned);
    REQUIRE(simd::to_array(none) == std::array<Element, simd::element_count>{});

    const auto one = simd::template load_partial<1>(unaligned);
    auto expected_one = std::array<Element, simd::element_count>{};
    expected_one[0] = storage[1];
    REQUIRE(simd::to_array(one) == expected_one);

    const auto almost_full = simd::template load_partial<simd::element_count - 1>(unaligned);
    auto expected_almost_full = std::array<Element, simd::element_count>{};
    std::copy_n(storage.begin() + 1, simd::element_count - 1, expected_almost_full.begin());
    REQUIRE(simd::to_array(almost_full) == expected_almost_full);

    const auto full = simd::template load_partial<simd::element_count>(unaligned);
    std::array<Element, simd::element_count> expected_full{};
    std::copy_n(storage.begin() + 1, simd::element_count, expected_full.begin());
    REQUIRE(simd::to_array(full) == expected_full);
}

template <std::size_t Width>
void require_supported_partial_transfer_matrix()
{
    require_partial_transfer_contracts<Width, std::int8_t>();
    require_partial_transfer_contracts<Width, std::uint16_t>();
    require_partial_transfer_contracts<Width, std::int32_t>();
    require_partial_transfer_contracts<Width, std::uint64_t>();
    require_partial_transfer_contracts<Width, float>();
}

template <std::size_t Width, std::integral Element>
void require_comparison_contract()
{
    using simd = Api<Width, Element>;
    std::array<Element, simd::element_count> lhs{};
    std::array<Element, simd::element_count> rhs{};
    for (std::size_t index = 0; index < simd::element_count; ++index)
    {
        switch (index % 4)
        {
        case 0:
            lhs[index] = Element{0};
            rhs[index] = Element{0};
            break;
        case 1:
            lhs[index] = Element{1};
            rhs[index] = Element{2};
            break;
        case 2:
            lhs[index] = Element{3};
            rhs[index] = Element{2};
            break;
        default:
            lhs[index] = std::numeric_limits<Element>::max();
            rhs[index] = std::numeric_limits<Element>::lowest();
            break;
        }
    }

    typename simd::mask_t eq = 0;
    typename simd::mask_t gt = 0;
    typename simd::mask_t ge = 0;
    typename simd::mask_t lt = 0;
    typename simd::mask_t le = 0;
    constexpr typename simd::mask_t lane_mask =
        static_cast<typename simd::mask_t>((typename simd::mask_t{1} << sizeof(Element)) - 1);
    for (std::size_t index = 0; index < simd::element_count; ++index)
    {
        const auto mask = static_cast<typename simd::mask_t>(lane_mask << (index * sizeof(Element)));
        if (lhs[index] == rhs[index])
            eq |= mask;
        if (lhs[index] > rhs[index])
            gt |= mask;
        if (lhs[index] >= rhs[index])
            ge |= mask;
        if (lhs[index] < rhs[index])
            lt |= mask;
        if (lhs[index] <= rhs[index])
            le |= mask;
    }

    const auto left = simd::construct(lhs);
    const auto right = simd::construct(rhs);
    REQUIRE(simd::cmp_eq(left, right) == eq);
    REQUIRE(simd::cmp_eq_mask(left, right) == eq);
    REQUIRE(simd::cmp_gt(left, right) == gt);
    REQUIRE(simd::cmp_ge(left, right) == ge);
    REQUIRE(simd::cmp_lt(left, right) == lt);
    REQUIRE(simd::cmp_le(left, right) == le);
}

template <std::size_t Width>
void require_supported_comparison_matrix()
{
    require_comparison_contract<Width, std::int8_t>();
    require_comparison_contract<Width, std::uint8_t>();
    require_comparison_contract<Width, std::int16_t>();
    require_comparison_contract<Width, std::uint16_t>();
    require_comparison_contract<Width, std::int32_t>();
    require_comparison_contract<Width, std::uint32_t>();
    require_comparison_contract<Width, std::int64_t>();
    require_comparison_contract<Width, std::uint64_t>();
}

template <std::size_t Width, std::integral Element, std::size_t Count>
void require_transform_pack_mask_contract()
{
    using simd = Api<Width, Element>;
    using write_t = typename simd::template packed_element_t<1>;
    constexpr std::size_t output_count = simd::template packed_element_count<1, Count>;

    std::array<Element, Count> input{};
    for (std::size_t index = 0; index < Count; ++index)
        input[index] = index % 3 == 1 ? Element{0} : static_cast<Element>(index + 1);

    std::array<write_t, output_count + 2> guarded{};
    guarded.fill(static_cast<write_t>(0xA5));
    std::span<write_t, output_count> output{guarded.data() + 1, output_count};
    const auto predicate = simd::set1(Element{0});
    simd::template transform_pack<1>(
        std::span<const Element, Count>{input}, output,
        [&predicate](const typename simd::vector_t value) noexcept
        { return simd::movemask_slim(simd::cmpeq(value, predicate)); });

    std::array<write_t, output_count> expected{};
    for (std::size_t index = 0; index < Count; ++index)
        if (input[index] == 0)
            expected[index / 8] |= static_cast<write_t>(write_t{1} << (index % 8));

    REQUIRE(std::equal(output.begin(), output.end(), expected.begin()));
    REQUIRE(guarded.front() == static_cast<write_t>(0xA5));
    REQUIRE(guarded.back() == static_cast<write_t>(0xA5));
}

template <std::size_t Width, std::unsigned_integral Element, std::size_t Count, std::size_t ResultBitWidth>
void require_transform_pack_width_contract()
{
    static_assert(ResultBitWidth > 0 && ResultBitWidth <= 64);
    using simd = Api<Width, Element>;
    using write_t = typename simd::template packed_element_t<ResultBitWidth>;
    constexpr std::size_t output_count = simd::template packed_element_count<ResultBitWidth, Count>;
    constexpr std::size_t write_element_width = std::numeric_limits<write_t>::digits;
    constexpr std::uint64_t result_mask = ResultBitWidth == 64
        ? std::numeric_limits<std::uint64_t>::max()
        : (std::uint64_t{1} << ResultBitWidth) - 1;

    std::array<Element, Count> input{};
    for (std::size_t index = 0; index < Count; ++index)
        input[index] = static_cast<Element>(index * 5 + 3);

    std::array<write_t, output_count + 2> guarded{};
    guarded.fill(static_cast<write_t>(0xA5));
    std::span<write_t, output_count> output{guarded.data() + 1, output_count};
    simd::template transform_pack<ResultBitWidth>(
        std::span<const Element, Count>{input}, output,
        [](const typename simd::vector_t value) noexcept
        {
            const auto lanes = simd::to_array(value);
            std::uint64_t packed = 0;
            for (std::size_t lane = 0; lane < lanes.size(); ++lane)
                packed |= (static_cast<std::uint64_t>(lanes[lane]) & result_mask) << (lane * ResultBitWidth);
            return packed;
        });

    std::array<write_t, output_count> expected{};
    for (std::size_t index = 0; index < Count; ++index)
    {
        const std::uint64_t result = static_cast<std::uint64_t>(input[index]) & result_mask;
        for (std::size_t bit = 0; bit < ResultBitWidth; ++bit)
        {
            const std::size_t output_bit = index * ResultBitWidth + bit;
            if ((result & (std::uint64_t{1} << bit)) != 0)
                expected[output_bit / write_element_width] |=
                    static_cast<write_t>(write_t{1} << (output_bit % write_element_width));
        }
    }

    REQUIRE(std::equal(output.begin(), output.end(), expected.begin()));
    REQUIRE(guarded.front() == static_cast<write_t>(0xA5));
    REQUIRE(guarded.back() == static_cast<write_t>(0xA5));
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
