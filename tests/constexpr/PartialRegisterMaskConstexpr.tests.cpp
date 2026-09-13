#include <SimdLib/PartialRegister.h>

#include <array>
#include <cstdint>

namespace
{

/** @brief Creates a canonical partial predicate for constant-evaluation tests. */
consteval auto constexpr_true_mask()
{
	using mask_t = SimdLib::PartialRegisterMask<std::uint32_t, 128, 3>;
	using api_t = typename mask_t::api_type;
	constexpr std::array<std::uint32_t, 4> all_true{0xffffffffU, 0xffffffffU, 0xffffffffU, 0xffffffffU};
	return mask_t::from_native(api_t::construct(all_true));
}

/** @brief Verifies active-only predicate reductions during constant evaluation. */
consteval bool has_constexpr_partial_mask_reductions()
{
	const auto true_mask = constexpr_true_mask();
	return true_mask.bits() == 0x7U && true_mask.any() && true_mask.all() && !true_mask.none();
}

/** @brief Verifies immutable predicate composition during constant evaluation. */
consteval bool has_constexpr_partial_mask_composition()
{
	const auto true_mask = constexpr_true_mask();
	return (true_mask ^ true_mask).none();
}

/** @brief Verifies mixed partial-predicate selection during constant evaluation. */
consteval bool has_constexpr_partial_mask_selection()
{
	using mask_t = SimdLib::PartialRegisterMask<std::uint64_t, 128, 1>;
	using value_t = typename mask_t::register_type;
	using api_t = typename mask_t::api_type;
	constexpr std::array<std::uint64_t, 2> mixed{0xffffffffffffffffULL, 0U};
	constexpr std::array<std::uint64_t, 2> when_true{1U, 2U};
	constexpr std::array<std::uint64_t, 2> when_false{5U, 6U};
	const auto mixed_mask = mask_t::from_native(api_t::construct(mixed));
	const auto selected = mixed_mask.select(value_t::from_native(api_t::construct(when_true)), value_t::from_native(api_t::construct(when_false)));
	const auto lanes = api_t::to_array(selected.to_native());
	return lanes[0] == 1U && lanes[1] == 0U;
}

static_assert(has_constexpr_partial_mask_reductions());

// MSVC 19.44 ICEs while inspecting a PartialRegisterMask result from composition or select() in constant evaluation.
#if !defined(_MSC_VER) || defined(__clang__)
static_assert(has_constexpr_partial_mask_composition());
static_assert(has_constexpr_partial_mask_selection());
#endif

} // namespace
