#include <SimdLib/IRegisterMask.h>
#include <SimdLib/Register.h>

#include <cstdint>
#include <type_traits>
#include <utility>

namespace
{

/** @brief Reports whether a Register exposes a complete identity logical shuffle. */
template <class register_t, std::size_t... indices> [[nodiscard]] consteval bool has_identity_shuffle(std::index_sequence<indices...>) noexcept
{
	return SimdLib::IRegister::Shuffle<register_t, indices...>;
}

/** @brief Reports whether an Api exposes a complete identity logical shuffle. */
template <class api_t, std::size_t... indices> [[nodiscard]] consteval bool has_identity_api_shuffle(std::index_sequence<indices...>) noexcept
{
	return SimdLib::IApi::Shuffle<api_t, indices...>;
}

/** @brief Audits every public Register and RegisterMask declaration for one supported element/width cell. */
template <class element_t, std::size_t bits> [[nodiscard]] consteval bool has_complete_surface() noexcept
{
	using api_t = SimdLib::Api<bits, element_t>;
	using register_t = SimdLib::Register<element_t, bits>;
	using mask_t = typename register_t::mask_type;

	constexpr bool integral = std::is_integral_v<element_t>;
	constexpr bool signed_integral = integral && std::is_signed_v<element_t>;
	constexpr bool byte_and_bit_shifts = integral && bits == 128;

	constexpr bool register_core =
		SimdLib::IRegister::Type<register_t> && SimdLib::IRegister::Zero<register_t> && SimdLib::IRegister::Broadcast<register_t> &&
		SimdLib::IRegister::FromArray<register_t> && SimdLib::IRegister::Load<register_t> && SimdLib::IRegister::LoadAligned<register_t> &&
		SimdLib::IRegister::LoadBytes<register_t> && SimdLib::IRegister::Store<register_t> && SimdLib::IRegister::StoreAligned<register_t> &&
		SimdLib::IRegister::StoreBytes<register_t> && SimdLib::IRegister::ToArray<register_t> && SimdLib::IRegister::Lane<register_t, 0> &&
		SimdLib::IRegister::WithLane<register_t, register_t::lane_count - 1> && !SimdLib::IRegister::Lane<register_t, register_t::lane_count> &&
		!SimdLib::IRegister::WithLane<register_t, register_t::lane_count>;

	constexpr bool arithmetic =
		SimdLib::IRegister::Add<register_t> == SimdLib::IApi::Add<api_t> && SimdLib::IRegister::Subtract<register_t> == SimdLib::IApi::Subtract<api_t> &&
		SimdLib::IRegister::Multiply<register_t> == SimdLib::IApi::Multiply<api_t> && SimdLib::IRegister::Divide<register_t> == SimdLib::IApi::Divide<api_t> &&
		SimdLib::IRegister::Modulus<register_t> == SimdLib::IApi::Modulus<api_t> && SimdLib::IRegister::Negate<register_t> == SimdLib::IApi::Negate<api_t>;

	constexpr bool specialized =
		SimdLib::IRegister::Min<register_t> == SimdLib::IApi::Min<api_t> && SimdLib::IRegister::Max<register_t> == SimdLib::IApi::Max<api_t> &&
		SimdLib::IRegister::Absolute<register_t> == SimdLib::IApi::Absolute<api_t> && SimdLib::IRegister::Sqrt<register_t> == SimdLib::IApi::Sqrt<api_t> &&
		SimdLib::IRegister::Average<register_t> == SimdLib::IApi::Average<api_t> &&
		SimdLib::IRegister::MultiplyAdd<register_t> == SimdLib::IApi::MultiplyAdd<api_t> &&
		SimdLib::IRegister::Magnitude<register_t> == SimdLib::IApi::Magnitude<api_t> &&
		SimdLib::IRegister::MagnitudeChecked<register_t> == SimdLib::IApi::MagnitudeChecked<api_t> &&
		SimdLib::IRegister::Normalize<register_t> == SimdLib::IApi::Normalize<api_t> &&
		SimdLib::IRegister::HorizontalAdd<register_t> == SimdLib::IApi::HorizontalAdd<api_t> &&
		SimdLib::IRegister::HorizontalSubtract<register_t> == SimdLib::IApi::HorizontalSubtract<api_t> &&
		SimdLib::IRegister::MultiplyAddAdjacent<register_t> == SimdLib::IApi::MultiplyAddAdjacent<api_t> &&
		SimdLib::IRegister::MultiplyAddUnsignedSignedBytes<register_t> == SimdLib::IApi::ByteMultiplyAdd<api_t> &&
		SimdLib::IRegister::SumAbsoluteByteDifferences<register_t> == SimdLib::IApi::Sad<api_t> &&
		SimdLib::IRegister::MultiSumAbsoluteByteDifferences<register_t, 0> == SimdLib::IApi::MultiSad<api_t, 0> &&
		SimdLib::IRegister::MinPosition<register_t> == SimdLib::IApi::MinPosition<api_t> &&
		SimdLib::IRegister::MaxPosition<register_t> == SimdLib::IApi::MaxPosition<api_t> &&
		SimdLib::IRegister::AddSaturated<register_t> == SimdLib::IApi::AddSaturated<api_t> &&
		SimdLib::IRegister::SubtractSaturated<register_t> == SimdLib::IApi::SubtractSaturated<api_t> &&
		SimdLib::IRegister::HorizontalAddSaturated<register_t> == SimdLib::IApi::HorizontalAddSaturated<api_t> &&
		SimdLib::IRegister::HorizontalSubtractSaturated<register_t> == SimdLib::IApi::HorizontalSubtractSaturated<api_t> &&
		SimdLib::IRegister::AddSubtract<register_t> == SimdLib::IApi::AddSubtract<api_t> &&
		SimdLib::IRegister::DotProduct<register_t, 0> == SimdLib::IApi::DotProduct<api_t, 0> && !SimdLib::IRegister::DotProduct<register_t, -1> &&
		!SimdLib::IRegister::DotProduct<register_t, 256>;

	constexpr bool bitwise_comparison_and_mask =
		SimdLib::IRegister::BitwiseAnd<register_t> && SimdLib::IRegister::BitwiseOr<register_t> && SimdLib::IRegister::BitwiseXor<register_t> &&
		SimdLib::IRegister::BitwiseNot<register_t> && SimdLib::IRegister::BitwiseAndNot<register_t> && SimdLib::IRegister::Movemask<register_t> &&
		SimdLib::IRegister::LaneSignBits<register_t> && SimdLib::IRegister::CompareEqual<register_t> && SimdLib::IRegister::CompareGreater<register_t> &&
		SimdLib::IRegister::CompareGreaterEqual<register_t> && SimdLib::IRegister::CompareLess<register_t> &&
		SimdLib::IRegister::CompareLessEqual<register_t> && SimdLib::IRegister::Equal<register_t> && SimdLib::IRegister::NotEqual<register_t> &&
		SimdLib::IRegisterMask::Type<mask_t> && SimdLib::IRegisterMask::Any<mask_t> && SimdLib::IRegisterMask::All<mask_t> &&
		SimdLib::IRegisterMask::None<mask_t> && SimdLib::IRegisterMask::Bits<mask_t> && SimdLib::IRegisterMask::Select<mask_t> &&
		SimdLib::IRegisterMask::BitwiseAnd<mask_t> && SimdLib::IRegisterMask::BitwiseOr<mask_t> && SimdLib::IRegisterMask::BitwiseXor<mask_t> &&
		SimdLib::IRegisterMask::BitwiseNot<mask_t>;

	constexpr bool shifts =
		SimdLib::IRegister::ShiftLeft<register_t> == SimdLib::IApi::ShiftLeft<api_t> &&
		SimdLib::IRegister::LogicalShiftRight<register_t> == SimdLib::IApi::ShiftRight<api_t> &&
		SimdLib::IRegister::ShiftRight<register_t> == (signed_integral ? SimdLib::IApi::ArithmeticShiftRight<api_t> : SimdLib::IApi::ShiftRight<api_t>) &&
		SimdLib::IRegister::ByteShiftLeft<register_t> == byte_and_bit_shifts && SimdLib::IRegister::ByteShiftRight<register_t> == byte_and_bit_shifts &&
		SimdLib::IRegister::BitShiftLeft<register_t> == byte_and_bit_shifts && SimdLib::IRegister::BitShiftRight<register_t> == byte_and_bit_shifts &&
		SimdLib::IRegister::IndexedBitShiftLeft<register_t, 0> == byte_and_bit_shifts &&
		SimdLib::IRegister::IndexedBitShiftRight<register_t, 0> == byte_and_bit_shifts && !SimdLib::IRegister::IndexedBitShiftLeft<register_t, -1> &&
		!SimdLib::IRegister::IndexedBitShiftRight<register_t, -1>;

	constexpr bool lower_half = SimdLib::IRegister::LowerHalf<register_t> == (bits == 256 && SimdLib::IApi::LowerHalf<api_t>);
	constexpr bool unpack_low = SimdLib::IRegister::UnpackLow<register_t> == SimdLib::IApi::UnpackLow<api_t>;
	constexpr bool unpack_high = SimdLib::IRegister::UnpackHigh<register_t> == SimdLib::IApi::UnpackHigh<api_t>;
	constexpr bool logical_shuffle = has_identity_shuffle<register_t>(std::make_index_sequence<register_t::lane_count>{}) ==
									 has_identity_api_shuffle<api_t>(std::make_index_sequence<register_t::lane_count>{});
	constexpr bool shuffle_low = SimdLib::IRegister::ShuffleLow<register_t, 0> == SimdLib::IApi::ShuffleLow<api_t, 0>;
	constexpr bool shuffle_high = SimdLib::IRegister::ShuffleHigh<register_t, 0> == SimdLib::IApi::ShuffleHigh<api_t, 0>;
	constexpr bool blend = SimdLib::IRegister::Blend<register_t, 0> == SimdLib::IApi::Blend<api_t, 0>;

	static_assert(register_core);
	static_assert(arithmetic);
	static_assert(specialized);
	static_assert(bitwise_comparison_and_mask);
	static_assert(shifts);
	static_assert(lower_half);
	static_assert(unpack_low);
	static_assert(unpack_high);
	static_assert(logical_shuffle);
	static_assert(shuffle_low);
	static_assert(shuffle_high);
	static_assert(blend);
	return true;
}

/** @brief Audits all ten supported element types for one register width. */
template <std::size_t bits> [[nodiscard]] consteval bool has_complete_surface_for_all_elements() noexcept
{
	return has_complete_surface<std::int8_t, bits>() && has_complete_surface<std::uint8_t, bits>() && has_complete_surface<std::int16_t, bits>() &&
		   has_complete_surface<std::uint16_t, bits>() && has_complete_surface<std::int32_t, bits>() && has_complete_surface<std::uint32_t, bits>() &&
		   has_complete_surface<std::int64_t, bits>() && has_complete_surface<std::uint64_t, bits>() && has_complete_surface<float, bits>() &&
		   has_complete_surface<double, bits>();
}

static_assert(has_complete_surface_for_all_elements<128>());
static_assert(has_complete_surface_for_all_elements<256>());

/** @brief Audits full-width bit casts, numeric conversions, and widening destinations for one source cell. */
template <class source_t, std::size_t bits> [[nodiscard]] consteval bool has_complete_conversion_surface() noexcept
{
	using api_t = SimdLib::Api<bits, source_t>;
	using register_t = SimdLib::Register<source_t, bits>;

	const auto target_matches = []<class target_t>() consteval noexcept
	{
		return SimdLib::IRegister::BitCast<register_t, target_t> &&
			   SimdLib::IRegister::Convert<register_t, target_t> == SimdLib::IApi::Convert<api_t, target_t> &&
			   SimdLib::IRegister::WidenLow<register_t, target_t, 128> == SimdLib::IApi::Widen<api_t, SimdLib::Api<128, target_t>> &&
			   SimdLib::IRegister::WidenLow<register_t, target_t, 256> == SimdLib::IApi::Widen<api_t, SimdLib::Api<256, target_t>>;
	};

	return target_matches.template operator()<std::int8_t>() && target_matches.template operator()<std::uint8_t>() &&
		   target_matches.template operator()<std::int16_t>() && target_matches.template operator()<std::uint16_t>() &&
		   target_matches.template operator()<std::int32_t>() && target_matches.template operator()<std::uint32_t>() &&
		   target_matches.template operator()<std::int64_t>() && target_matches.template operator()<std::uint64_t>() &&
		   target_matches.template operator()<float>() && target_matches.template operator()<double>();
}

/** @brief Audits conversion destinations for all ten source element types at one register width. */
template <std::size_t bits> [[nodiscard]] consteval bool has_complete_conversion_surface_for_all_elements() noexcept
{
	return has_complete_conversion_surface<std::int8_t, bits>() && has_complete_conversion_surface<std::uint8_t, bits>() &&
		   has_complete_conversion_surface<std::int16_t, bits>() && has_complete_conversion_surface<std::uint16_t, bits>() &&
		   has_complete_conversion_surface<std::int32_t, bits>() && has_complete_conversion_surface<std::uint32_t, bits>() &&
		   has_complete_conversion_surface<std::int64_t, bits>() && has_complete_conversion_surface<std::uint64_t, bits>() &&
		   has_complete_conversion_surface<float, bits>() && has_complete_conversion_surface<double, bits>();
}

static_assert(has_complete_conversion_surface_for_all_elements<128>());
static_assert(has_complete_conversion_surface_for_all_elements<256>());

} // namespace
