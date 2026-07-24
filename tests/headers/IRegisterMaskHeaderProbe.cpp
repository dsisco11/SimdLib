#include <SimdLib/IRegisterMask.h>

#include <cstddef>
#include <cstdint>

namespace
{

/** @brief Minimal API metadata used by the standalone RegisterMask interface probe. */
struct ApiShape
{
};

/** @brief Minimal Register result used by the standalone RegisterMask selection probe. */
struct RegisterShape
{
	int native{};
};

/** @brief Minimal aggregate predicate implementation used to verify the standalone RegisterMask interface header. */
struct MaskShape
{
	using element_type = int;
	using api_type = ApiShape;
	using native_type = int;
	using register_type = RegisterShape;
	using bits_type = std::uint32_t;

	constexpr static inline std::size_t register_width = 128;
	constexpr static inline std::size_t byte_count = 16;
	constexpr static inline std::size_t lane_count = 4;

	native_type native{};

	/** @brief Reports whether any predicate lane is active. */
	[[nodiscard]] constexpr bool any() const noexcept
	{
		return native != 0;
	}

	/** @brief Reports whether every predicate lane is active. */
	[[nodiscard]] constexpr bool all() const noexcept
	{
		return native == -1;
	}

	/** @brief Reports whether no predicate lane is active. */
	[[nodiscard]] constexpr bool none() const noexcept
	{
		return native == 0;
	}

	/** @brief Returns one compact bit per logical predicate lane. */
	[[nodiscard]] constexpr bits_type bits() const noexcept
	{
		return static_cast<bits_type>(native);
	}

	/** @brief Selects one Register value according to the predicate. */
	[[nodiscard]] constexpr register_type select(register_type when_true, register_type when_false) const noexcept
	{
		return native != 0 ? when_true : when_false;
	}

	/** @brief Computes predicate intersection. */
	[[maybe_unused, nodiscard]] friend constexpr MaskShape operator&(MaskShape lhs, MaskShape rhs) noexcept
	{
		return {lhs.native & rhs.native};
	}

	/** @brief Computes predicate union. */
	[[maybe_unused, nodiscard]] friend constexpr MaskShape operator|(MaskShape lhs, MaskShape rhs) noexcept
	{
		return {lhs.native | rhs.native};
	}

	/** @brief Computes predicate exclusive union. */
	[[maybe_unused, nodiscard]] friend constexpr MaskShape operator^(MaskShape lhs, MaskShape rhs) noexcept
	{
		return {lhs.native ^ rhs.native};
	}

	/** @brief Computes predicate complement. */
	[[maybe_unused, nodiscard]] friend constexpr MaskShape operator~(MaskShape value) noexcept
	{
		return {~value.native};
	}
};

static_assert(SimdLib::IRegisterMask::Type<MaskShape>);
static_assert(SimdLib::IRegisterMask::Any<MaskShape>);
static_assert(SimdLib::IRegisterMask::All<MaskShape>);
static_assert(SimdLib::IRegisterMask::None<MaskShape>);
static_assert(SimdLib::IRegisterMask::Bits<MaskShape>);
static_assert(SimdLib::IRegisterMask::Select<MaskShape>);
static_assert(SimdLib::IRegisterMask::BitwiseAnd<MaskShape>);
static_assert(SimdLib::IRegisterMask::BitwiseOr<MaskShape>);
static_assert(SimdLib::IRegisterMask::BitwiseXor<MaskShape>);
static_assert(SimdLib::IRegisterMask::BitwiseNot<MaskShape>);

} // namespace
