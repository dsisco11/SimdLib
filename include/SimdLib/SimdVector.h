#pragma once
#include <SimdLib/Config.h>
#include <SimdLib/Api.h>
#include <algorithm>
#include <array>
#include <bit>
#include <concepts>
#include <cstdint>
#include <functional>
#include <limits>
#include <span>
#include <tuple>
#include <type_traits>
#include <utility>

namespace SimdLib
{

template <class element_t, int element_count>
	requires std::is_arithmetic_v<element_t>
class SimdVector final
{
  public:
	constexpr static inline auto minimum_bit_count = element_count * sizeof(element_t) * 8;
	constexpr static inline auto simd_width = std::max(std::size_t(128), std::bit_ceil(minimum_bit_count));
	using simd = Api<simd_width, element_t>;
	using vector_t = typename simd::vector_t;
	using mask_t = typename simd::mask_t;
	using area_element_t = std::conditional_t<std::is_integral_v<element_t> && (sizeof(element_t) < sizeof(std::uint32_t)),
											  std::conditional_t<std::is_signed_v<element_t>, std::int32_t, std::uint32_t>, element_t>;

#pragma region Internal
  private:
	simd::vector_t m_data;

	constexpr static inline std::size_t active_mask_bit_count = element_count * sizeof(element_t);
	constexpr static inline mask_t active_cmp_mask = []()
	{
		if constexpr (active_mask_bit_count >= (sizeof(mask_t) * 8))
		{
			return std::numeric_limits<mask_t>::max();
		}
		else
		{
			return static_cast<mask_t>((mask_t{1} << active_mask_bit_count) - 1);
		}
	}();

	constexpr static inline mask_t full_cmp_mask = []()
	{
		if constexpr (simd::byte_count >= (sizeof(mask_t) * 8))
		{
			return std::numeric_limits<mask_t>::max();
		}
		else
		{
			return static_cast<mask_t>((mask_t{1} << simd::byte_count) - 1);
		}
	}();

	constexpr static inline mask_t inactive_cmp_mask = static_cast<mask_t>(full_cmp_mask & ~active_cmp_mask);

	SIMDLIB_FORCE_INLINE constexpr static bool mask_has_any(const mask_t mask) noexcept
	{
		return (mask & active_cmp_mask) != 0;
	}

	SIMDLIB_FORCE_INLINE constexpr static bool mask_has_all(const mask_t mask) noexcept
	{
		return (mask & active_cmp_mask) == active_cmp_mask;
	}

	SIMDLIB_FORCE_INLINE constexpr static bool inactive_mask_has_all(const mask_t mask) noexcept
	{
		return (mask & inactive_cmp_mask) == inactive_cmp_mask;
	}

	/** @brief Verifies debug-only that an operation result preserves zeroed inactive lanes for partial vectors.
	 *  @param value SIMD result being returned from an operation.
	 *  @param operation Name of the operation validating the result.
	 *  @return `value` unchanged.
	 */
	template <class result_t> SIMDLIB_FORCE_INLINE constexpr static result_t CheckResultInactiveLanesZero(const result_t value, const char *operation) noexcept
	{
#if SIMDLIB_ENABLE_CHECKS
		if constexpr (element_count != simd::element_count && std::same_as<std::remove_cvref_t<result_t>, vector_t>)
		{
			if (!std::is_constant_evaluated())
			{
				const mask_t zeroMask = simd::cmp_eq_mask(value, simd::setzero());
				SIMDLIB_PRECONDITION(inactive_mask_has_all(zeroMask), operation);
			}
		}
#else
		(void)operation;
#endif
		return value;
	}

	/** @brief Replaces inactive logical lanes with a caller-provided fill value.
	 *  @param value Source register whose active lanes are preserved.
	 *  @param fillValue Scalar written into every inactive hardware lane.
	 *  @return Register with unchanged active lanes and filled inactive lanes.
	 */
	SIMDLIB_FORCE_INLINE constexpr static vector_t FillInactiveLanes(const vector_t value, const element_t fillValue) noexcept
	{
		if constexpr (element_count == simd::element_count)
		{
			return value;
		}

		const auto lanes = simd::to_array(value);
		return [&]<std::size_t... ActiveIndices, std::size_t... FillIndices>(std::index_sequence<ActiveIndices...>,
																			 std::index_sequence<FillIndices...>) constexpr noexcept -> vector_t
		{
			return simd::setr_partial(static_cast<element_t>(lanes[ActiveIndices])..., ((void)FillIndices, fillValue)...);
		}(std::make_index_sequence<element_count>{}, std::make_index_sequence<simd::element_count - element_count>{});
	}

	template <std::size_t ByteIndex> SIMDLIB_FORCE_INLINE constexpr static std::size_t PairProductPartnerByteIndex() noexcept
	{
		constexpr std::size_t ByteCountPer128Lane = 16;
		constexpr std::size_t ByteIndexWithin128Lane = ByteIndex % ByteCountPer128Lane;
		constexpr std::size_t LaneIndexWithin128Lane = ByteIndexWithin128Lane / sizeof(element_t);
		constexpr std::size_t OddLaneIndexWithin128Lane = (LaneIndexWithin128Lane & ~std::size_t{1}) + 1;
		return (OddLaneIndexWithin128Lane * sizeof(element_t)) + (ByteIndexWithin128Lane % sizeof(element_t));
	}

	/** @brief Builds the right-hand register used to turn multiply-add-adjacent into pairwise products.
	 *  @param value Source register whose active lanes have already been padded with multiplicative identity values.
	 *  @return Register shaped like `[y, 0, w, 0, ...]` so adjacent multiply-add yields pairwise products.
	 */
	SIMDLIB_FORCE_INLINE constexpr static vector_t BuildPairProductPartner(const vector_t value) noexcept
	{
		const vector_t evenLaneMask = []<std::size_t... Indices>(std::index_sequence<Indices...>) constexpr noexcept -> vector_t
		{
			return simd::setr(((Indices % 2) == 0
				? std::bit_cast<element_t>(std::numeric_limits<std::make_unsigned_t<element_t>>::max())
				: element_t{0})...);
		}(std::make_index_sequence<simd::element_count>{});

		return [&]<std::size_t... ByteIndices>(std::index_sequence<ByteIndices...>) constexpr noexcept -> vector_t
		{
			return simd::bitwise_and(simd::template shuffle<PairProductPartnerByteIndex<ByteIndices>()...>(value), evenLaneMask);
		}(std::make_index_sequence<simd::byte_count>{});
	}
#pragma endregion

#pragma region Constructors

  public:
	/** @brief Copies another SIMD vector. */
	SimdVector(const SimdVector &other) = default;
	/** @brief Moves another SIMD vector. */
	SimdVector(SimdVector &&other) = default;
	/** @brief Replaces this SIMD vector with a copy of another SIMD vector.
	 *  @param other Source SIMD vector.
	 *  @return Reference to this SIMD vector.
	 */
	SimdVector &operator=(const SimdVector &other) = default;
	/** @brief Replaces this SIMD vector by moving another SIMD vector.
	 *  @param other Source SIMD vector.
	 *  @return Reference to this SIMD vector.
	 */
	SimdVector &operator=(SimdVector &&other) = default;
	/** @brief Destroys the SIMD vector. */
	~SimdVector() = default;

	/** @brief Constructs a new SIMD vector with all elements set to zero.
	 *  @return Zero-initialized SIMD vector storage.
	 */
	SIMDLIB_FORCE_INLINE constexpr SimdVector() noexcept
	{
		if (std::is_constant_evaluated())
			m_data = {};
		else
			m_data = simd::setzero();
	}

	/** @brief Constructs a new SIMD vector from a SIMD register.
	 *  @param data Source SIMD register.
	 *  @return SIMD vector that wraps `data` unchanged.
	 */
	SIMDLIB_FORCE_INLINE constexpr SimdVector(vector_t data) noexcept
	{
		m_data = data;
	};

	/** @brief Constructs a new SIMD vector with all elements set to the same value.
	 *  @param v Scalar value broadcast into every register lane.
	 *  @return SIMD vector whose lanes are all initialized from `v`.
	 */
	SIMDLIB_FORCE_INLINE constexpr explicit SimdVector(element_t v) noexcept
	{
		if constexpr (element_count == simd::element_count)
		{
			m_data = simd::set1(v);
		}
		else
		{
			m_data = [v]<std::size_t... Indices>(std::index_sequence<Indices...>) constexpr noexcept -> vector_t
			{ return simd::setr_partial(((void)Indices, v)...); }(std::make_index_sequence<element_count>{});
		}
	};

	/** @brief Constructs a new SIMD vector from a writable span of elements.
	 *  @param data Source span containing one full register worth of elements.
	 *  @return SIMD vector loaded from `data`.
	 */
	SIMDLIB_FORCE_INLINE constexpr explicit SimdVector(std::span<element_t, simd::element_count> data) noexcept
	{
		m_data = simd::load(std::span<const element_t, simd::element_count>(data.data(), data.size()));
	};

	/** @brief Constructs a new SIMD vector from a readonly span of elements.
	 *  @param data Source span containing one full register worth of elements.
	 *  @return SIMD vector loaded from `data`.
	 */
	SIMDLIB_FORCE_INLINE constexpr explicit SimdVector(std::span<const element_t, simd::element_count> data) noexcept
	{
		m_data = simd::load(data);
	};

	/** @brief Constructs a new SIMD vector from a writable span containing exactly the logical element count.
	 *  @param data Source span containing exactly the active logical elements.
	 *  @return SIMD vector loaded from `data` without requiring caller-side padding.
	 */
	SIMDLIB_FORCE_INLINE constexpr explicit SimdVector(std::span<element_t, element_count> data) noexcept
		requires(element_count != simd::element_count)
	{
		m_data = simd::template load_partial<element_count>(std::span<const element_t, element_count>(data));
	};

	/** @brief Constructs a new SIMD vector from a readonly span containing exactly the logical element count.
	 *  @param data Source span containing exactly the active logical elements.
	 *  @return SIMD vector loaded from `data` without requiring caller-side padding.
	 */
	SIMDLIB_FORCE_INLINE constexpr explicit SimdVector(std::span<const element_t, element_count> data) noexcept
		requires(element_count != simd::element_count)
	{
		m_data = simd::template load_partial<element_count>(data);
	};

	/** @brief Constructs a new SIMD vector from a fixed array of elements.
	 *  @param data Source array containing one full register worth of elements.
	 *  @return SIMD vector loaded from `data`.
	 */
	SIMDLIB_FORCE_INLINE constexpr explicit SimdVector(const std::array<element_t, simd::element_count> &data) noexcept
	{
		m_data = simd::construct(data);
	};

	/** @brief Constructs a new SIMD vector from a fixed array containing exactly the logical element count.
	 *  @param data Source array containing exactly the active logical elements.
	 *  @return SIMD vector loaded from `data` without requiring caller-side padding.
	 */
	SIMDLIB_FORCE_INLINE constexpr explicit SimdVector(const std::array<element_t, element_count> &data) noexcept
		requires(element_count != simd::element_count)
	{
		m_data = simd::template load_partial<element_count>(std::span<const element_t, element_count>(data));
	};

	/** @brief Constructs a new SIMD vector by widening another SIMD vector with the same logical element count.
	 *  @tparam source_t Source integer element type.
	 *  @param other Source SIMD vector whose active logical lanes are widened into this vector's lane type.
	 *  @return SIMD vector whose active lanes are widened from `other` and whose inactive lanes remain zero-filled.
	 */
	template <class source_t>
		requires(std::is_integral_v<source_t> && std::is_integral_v<element_t> && sizeof(source_t) < sizeof(element_t))
	SIMDLIB_FORCE_INLINE constexpr explicit SimdVector(const SimdVector<source_t, element_count> &other) noexcept
	{
		using source_simd = typename SimdVector<source_t, element_count>::simd;
		m_data = source_simd::template widen<simd>(other.getRegister());
	}

	/** @brief Constructs a new SIMD vector from the declared element values and zero-fills any unused SIMD lanes.
	 *  @param args Active vector elements in low-to-high order.
	 *  @return SIMD vector whose inactive hardware lanes are initialized to zero.
	 */
	template <std::convertible_to<element_t>... Args>
		requires(sizeof...(Args) == element_count)
	SIMDLIB_FORCE_INLINE constexpr explicit SimdVector(Args &&...args) noexcept
	{
		m_data = simd::setr_partial(static_cast<element_t>(std::forward<Args>(args))...);
	}

#pragma endregion

#pragma region Arithmetic Operators

  public:
	/** @brief Adds another register lane-wise to this vector.
	 *  @param rhs Right-hand input register.
	 *  @return Register containing the per-lane sum.
	 */
	SIMDLIB_FORCE_INLINE vector_t VECTORCALL operator+(vector_t rhs) const noexcept
	{
		return CheckResultInactiveLanesZero(simd::add(m_data, rhs), "SimdVector::operator+(vector_t)");
	}

	/** @brief Adds a scalar value to each active lane of this vector.
	 *  @param rhs Scalar value added to every active logical element.
	 *  @return Register containing the per-lane sum.
	 */
	SIMDLIB_FORCE_INLINE vector_t VECTORCALL operator+(element_t rhs) const noexcept
	{
		const SimdVector scalarRhs(rhs);
		return simd::add(m_data, scalarRhs.getRegister());
	}

	/** @brief Subtracts another register lane-wise from this vector.
	 *  @param rhs Right-hand input register.
	 *  @return Register containing the per-lane difference.
	 */
	SIMDLIB_FORCE_INLINE vector_t VECTORCALL operator-(vector_t rhs) const noexcept
	{
		return CheckResultInactiveLanesZero(simd::subtract(m_data, rhs), "SimdVector::operator-(vector_t)");
	}

	/** @brief Subtracts a scalar value from each active lane of this vector.
	 *  @param rhs Scalar value subtracted from every active logical element.
	 *  @return Register containing the per-lane difference.
	 */
	SIMDLIB_FORCE_INLINE vector_t VECTORCALL operator-(element_t rhs) const noexcept
	{
		const SimdVector scalarRhs(rhs);
		return simd::subtract(m_data, scalarRhs.getRegister());
	}

	/** @brief Multiplies this vector by another register lane-wise.
	 *  @param rhs Right-hand input register.
	 *  @return Register containing the per-lane product.
	 */
	SIMDLIB_FORCE_INLINE vector_t VECTORCALL operator*(vector_t rhs) const noexcept
	{
		return CheckResultInactiveLanesZero(simd::multiply(m_data, rhs), "SimdVector::operator*(vector_t)");
	}

	/** @brief Multiplies each active lane of this vector by a scalar value.
	 *  @param rhs Scalar value multiplied into every active logical element.
	 *  @return Register containing the per-lane product.
	 */
	SIMDLIB_FORCE_INLINE vector_t VECTORCALL operator*(element_t rhs) const noexcept
	{
		const SimdVector scalarRhs(rhs);
		return simd::multiply(m_data, scalarRhs.getRegister());
	}

	/** @brief Computes the inclusive per-lane extent between this vector and a minimum bound.
	 *  @tparam target_element_t Integral lane type used for the result.
	 *  @param minInclusive Inclusive minimum bound register.
	 *  @return SIMD vector containing `(this - minInclusive + 1)` per active lane, widened when needed.
	 */
	template <class target_element_t = area_element_t>
	SIMDLIB_FORCE_INLINE auto VECTORCALL size(vector_t minInclusive) const noexcept
		requires(std::is_integral_v<element_t> && std::is_integral_v<target_element_t> && sizeof(target_element_t) >= sizeof(element_t))
	{
		if constexpr (sizeof(target_element_t) > sizeof(element_t))
		{
			using target_vector_t = SimdVector<target_element_t, element_count>;
			const SimdVector minVector{minInclusive};
			const target_vector_t wideMax{*this};
			const target_vector_t wideMin{minVector};
			const target_vector_t difference{wideMax - wideMin.getRegister()};
			return target_vector_t{difference + static_cast<target_element_t>(1)};
		}
		else
		{
			return SimdVector{simd::add(simd::subtract(m_data, minInclusive), SimdVector(static_cast<element_t>(1)).getRegister())};
		}
	}

	/** @brief Computes the multiplicative inclusive extent between this vector and a minimum bound.
	 *  @tparam target_element_t Result lane type used for widened integer accumulation.
	 *  @param minInclusive Inclusive minimum bound register.
	 *  @return Product of `(this - minInclusive + 1)` over the active logical lanes.
	 */
	template <class target_element_t = area_element_t>
	SIMDLIB_FORCE_INLINE auto VECTORCALL area(vector_t minInclusive) const noexcept
		requires(std::is_integral_v<element_t> && std::is_integral_v<target_element_t> && sizeof(target_element_t) >= sizeof(element_t))
	{
		return this->template size<target_element_t>(minInclusive).area();
	}

	/** @brief Divides this vector by another register lane-wise.
	 *  @param rhs Right-hand input register.
	 *  @return Register containing the per-lane quotient.
	 */
	SIMDLIB_FORCE_INLINE vector_t VECTORCALL operator/(vector_t rhs) const noexcept
	{
		return CheckResultInactiveLanesZero(simd::divide(m_data, FillInactiveLanes(rhs, element_t{1})), "SimdVector::operator/(vector_t)");
	}

	/** @brief Divides each active lane of this vector by a scalar value.
	 *  @param rhs Scalar value that divides every active logical element.
	 *  @return Register containing the per-lane quotient.
	 */
	SIMDLIB_FORCE_INLINE vector_t VECTORCALL operator/(element_t rhs) const noexcept
	{
		return simd::divide(m_data, simd::set1(rhs));
	}

	/** @brief Computes the lane-wise remainder with another register.
	 *  @param rhs Right-hand input register.
	 *  @return Register containing the per-lane remainder.
	 */
	SIMDLIB_FORCE_INLINE vector_t VECTORCALL operator%(vector_t rhs) const noexcept
	{
		return CheckResultInactiveLanesZero(simd::modulus(m_data, FillInactiveLanes(rhs, element_t{1})), "SimdVector::operator%(vector_t)");
	}

	/** @brief Computes the lane-wise remainder with a scalar value.
	 *  @param rhs Scalar value used as the modulus for every active logical element.
	 *  @return Register containing the per-lane remainder.
	 */
	SIMDLIB_FORCE_INLINE vector_t VECTORCALL operator%(element_t rhs) const noexcept
	{
		return simd::modulus(m_data, simd::set1(rhs));
	}

	/** @brief Negates each lane of this vector.
	 *  @return Register containing the per-lane negation.
	 */
	SIMDLIB_FORCE_INLINE vector_t VECTORCALL operator-() const noexcept
	{
		return simd::negate(m_data);
	}

	/** @brief Adds another register lane-wise into this vector.
	 *  @param rhs Right-hand input register.
	 *  @return Reference to this SIMD vector after the update.
	 */
	SIMDLIB_FORCE_INLINE SimdVector &VECTORCALL operator+=(vector_t rhs) noexcept
	{
		m_data = CheckResultInactiveLanesZero(simd::add(m_data, rhs), "SimdVector::operator+=(vector_t)");
		return *this;
	}

	/** @brief Adds a scalar value to each active lane of this vector in place.
	 *  @param rhs Scalar value added to every active logical element.
	 *  @return Reference to this SIMD vector after the update.
	 */
	SIMDLIB_FORCE_INLINE SimdVector &VECTORCALL operator+=(element_t rhs) noexcept
	{
		const SimdVector scalarRhs(rhs);
		m_data = simd::add(m_data, scalarRhs.getRegister());
		return *this;
	}

	/** @brief Subtracts another register lane-wise from this vector in place.
	 *  @param rhs Right-hand input register.
	 *  @return Reference to this SIMD vector after the update.
	 */
	SIMDLIB_FORCE_INLINE SimdVector &VECTORCALL operator-=(vector_t rhs) noexcept
	{
		m_data = CheckResultInactiveLanesZero(simd::subtract(m_data, rhs), "SimdVector::operator-=(vector_t)");
		return *this;
	}

	/** @brief Subtracts a scalar value from each active lane of this vector in place.
	 *  @param rhs Scalar value subtracted from every active logical element.
	 *  @return Reference to this SIMD vector after the update.
	 */
	SIMDLIB_FORCE_INLINE SimdVector &VECTORCALL operator-=(element_t rhs) noexcept
	{
		const SimdVector scalarRhs(rhs);
		m_data = simd::subtract(m_data, scalarRhs.getRegister());
		return *this;
	}

	/** @brief Multiplies this vector by another register lane-wise in place.
	 *  @param rhs Right-hand input register.
	 *  @return Reference to this SIMD vector after the update.
	 */
	SIMDLIB_FORCE_INLINE SimdVector &VECTORCALL operator*=(vector_t rhs) noexcept
	{
		m_data = CheckResultInactiveLanesZero(simd::multiply(m_data, rhs), "SimdVector::operator*=(vector_t)");
		return *this;
	}

	/** @brief Multiplies each active lane of this vector by a scalar value in place.
	 *  @param rhs Scalar value multiplied into every active logical element.
	 *  @return Reference to this SIMD vector after the update.
	 */
	SIMDLIB_FORCE_INLINE SimdVector &VECTORCALL operator*=(element_t rhs) noexcept
	{
		const SimdVector scalarRhs(rhs);
		m_data = simd::multiply(m_data, scalarRhs.getRegister());
		return *this;
	}

	/** @brief Divides this vector by another register lane-wise in place.
	 *  @param rhs Right-hand input register.
	 *  @return Reference to this SIMD vector after the update.
	 */
	SIMDLIB_FORCE_INLINE SimdVector &VECTORCALL operator/=(vector_t rhs) noexcept
	{
		m_data = CheckResultInactiveLanesZero(simd::divide(m_data, FillInactiveLanes(rhs, element_t{1})), "SimdVector::operator/=(vector_t)");
		return *this;
	}

	/** @brief Divides each active lane of this vector by a scalar value in place.
	 *  @param rhs Scalar value that divides every active logical element.
	 *  @return Reference to this SIMD vector after the update.
	 */
	SIMDLIB_FORCE_INLINE SimdVector &VECTORCALL operator/=(element_t rhs) noexcept
	{
		m_data = simd::divide(m_data, simd::set1(rhs));
		return *this;
	}

	/** @brief Computes the lane-wise remainder with another register in place.
	 *  @param rhs Right-hand input register.
	 *  @return Reference to this SIMD vector after the update.
	 */
	SIMDLIB_FORCE_INLINE SimdVector &VECTORCALL operator%=(vector_t rhs) noexcept
	{
		m_data = CheckResultInactiveLanesZero(simd::modulus(m_data, FillInactiveLanes(rhs, element_t{1})), "SimdVector::operator%=(vector_t)");
		return *this;
	}

	/** @brief Computes the lane-wise remainder with a scalar value in place.
	 *  @param rhs Scalar value used as the modulus for every active logical element.
	 *  @return Reference to this SIMD vector after the update.
	 */
	SIMDLIB_FORCE_INLINE SimdVector &VECTORCALL operator%=(element_t rhs) noexcept
	{
		m_data = simd::modulus(m_data, simd::set1(rhs));
		return *this;
	}

#pragma endregion

#pragma region Arithmetic Methods (Saturated)

	/** @brief Adds the two vectors together and clamps integer overflow to the underlying type's maximum value.
	 *  @param rhs Right-hand input register.
	 *  @return Saturated sum of `m_data` and `rhs`.
	 */
	SIMDLIB_FORCE_INLINE vector_t VECTORCALL add_saturated(vector_t rhs) const noexcept
		requires requires(vector_t lhsValue, vector_t rhsValue) { simd::add_saturated(lhsValue, rhsValue); }
	{
		return CheckResultInactiveLanesZero(simd::add_saturated(m_data, rhs), "SimdVector::add_saturated(vector_t)");
	}

	/** @brief Adds a scalar value and clamps integer overflow to the underlying type's maximum value.
	 *  @param rhs Scalar value added to every active logical element.
	 *  @return Saturated sum of `m_data` and the broadcast scalar value.
	 */
	SIMDLIB_FORCE_INLINE vector_t VECTORCALL add_saturated(element_t rhs) const noexcept
		requires requires(vector_t lhsValue, vector_t rhsValue) { simd::add_saturated(lhsValue, rhsValue); }
	{
		const SimdVector scalarRhs(rhs);
		return simd::add_saturated(m_data, scalarRhs.getRegister());
	}

	/** @brief Subtracts the two vectors and clamps integer overflow to the underlying type's maximum value.
	 *  @param rhs Right-hand input register.
	 *  @return Saturated difference of `m_data` and `rhs`.
	 */
	SIMDLIB_FORCE_INLINE vector_t VECTORCALL subtract_saturated(vector_t rhs) const noexcept
		requires requires(vector_t lhsValue, vector_t rhsValue) { simd::subtract_saturated(lhsValue, rhsValue); }
	{
		return CheckResultInactiveLanesZero(simd::subtract_saturated(m_data, rhs), "SimdVector::subtract_saturated(vector_t)");
	}

	/** @brief Subtracts a scalar value and clamps integer overflow to the underlying type's valid range.
	 *  @param rhs Scalar value subtracted from every active logical element.
	 *  @return Saturated difference of `m_data` and the scalar value.
	 */
	SIMDLIB_FORCE_INLINE vector_t VECTORCALL subtract_saturated(element_t rhs) const noexcept
		requires requires(vector_t lhsValue, vector_t rhsValue) { simd::subtract_saturated(lhsValue, rhsValue); }
	{
		const SimdVector scalarRhs(rhs);
		return simd::subtract_saturated(m_data, scalarRhs.getRegister());
	}

	/** @brief Multiplies the two vectors and clamps integer overflow to the underlying type's maximum value.
	 *  @param rhs Right-hand input register.
	 *  @return Saturated product of `m_data` and `rhs`.
	 */
	SIMDLIB_FORCE_INLINE vector_t VECTORCALL multiply_saturated(vector_t rhs) const noexcept
		requires requires(vector_t lhsValue, vector_t rhsValue) { simd::multiply_saturated(lhsValue, rhsValue); }
	{
		return CheckResultInactiveLanesZero(simd::multiply_saturated(m_data, rhs), "SimdVector::multiply_saturated(vector_t)");
	}

	/** @brief Multiplies by a scalar value and clamps integer overflow to the underlying type's maximum value.
	 *  @param rhs Scalar value multiplied into every active logical element.
	 *  @return Saturated product of `m_data` and the scalar value.
	 */
	SIMDLIB_FORCE_INLINE vector_t VECTORCALL multiply_saturated(element_t rhs) const noexcept
		requires requires(vector_t lhsValue, vector_t rhsValue) { simd::multiply_saturated(lhsValue, rhsValue); }
	{
		const SimdVector scalarRhs(rhs);
		return simd::multiply_saturated(m_data, scalarRhs.getRegister());
	}

#pragma endregion

#pragma region Bitwise Operators

	/** @brief Inverts every bit in the underlying register.
	 *  @return SIMD vector containing the bitwise inverse.
	 */
	SIMDLIB_FORCE_INLINE SimdVector VECTORCALL operator~() const noexcept
	{
		if constexpr (element_count == simd::element_count)
		{
			return simd::bitwise_not(m_data);
		}
		else
		{
			const auto partialMask = []<std::size_t... Indices>(std::index_sequence<Indices...>) constexpr noexcept -> vector_t
			{ return simd::setr_partial(((void)Indices, static_cast<element_t>(~element_t{0}))...); }(std::make_index_sequence<element_count>{});

			return simd::bitwise_xor(m_data, partialMask);
		}
	}

	/** @brief Computes a lane-wise bitwise AND with another register.
	 *  @param rhs Right-hand input register.
	 *  @return SIMD vector containing the bitwise AND result.
	 */
	SIMDLIB_FORCE_INLINE SimdVector VECTORCALL operator&(vector_t rhs) const noexcept
	{
		return CheckResultInactiveLanesZero(simd::bitwise_and(m_data, rhs), "SimdVector::operator&(vector_t)");
	}

	/** @brief Computes a lane-wise bitwise OR with another register.
	 *  @param rhs Right-hand input register.
	 *  @return SIMD vector containing the bitwise OR result.
	 */
	SIMDLIB_FORCE_INLINE SimdVector VECTORCALL operator|(vector_t rhs) const noexcept
	{
		return CheckResultInactiveLanesZero(simd::bitwise_or(m_data, rhs), "SimdVector::operator|(vector_t)");
	}

	/** @brief Computes a lane-wise bitwise XOR with another register.
	 *  @param rhs Right-hand input register.
	 *  @return SIMD vector containing the bitwise XOR result.
	 */
	SIMDLIB_FORCE_INLINE SimdVector VECTORCALL operator^(vector_t rhs) const noexcept
	{
		return CheckResultInactiveLanesZero(simd::bitwise_xor(m_data, rhs), "SimdVector::operator^(vector_t)");
	}

	/** @brief Applies a lane-wise bitwise AND with another register in place.
	 *  @param rhs Right-hand input register.
	 *  @return Reference to this SIMD vector after the update.
	 */
	SIMDLIB_FORCE_INLINE SimdVector &VECTORCALL operator&=(vector_t rhs) noexcept
	{
		m_data = CheckResultInactiveLanesZero(simd::bitwise_and(m_data, rhs), "SimdVector::operator&=(vector_t)");
		return *this;
	}

	/** @brief Applies a lane-wise bitwise OR with another register in place.
	 *  @param rhs Right-hand input register.
	 *  @return Reference to this SIMD vector after the update.
	 */
	SIMDLIB_FORCE_INLINE SimdVector &VECTORCALL operator|=(vector_t rhs) noexcept
	{
		m_data = CheckResultInactiveLanesZero(simd::bitwise_or(m_data, rhs), "SimdVector::operator|=(vector_t)");
		return *this;
	}

	/** @brief Applies a lane-wise bitwise XOR with another register in place.
	 *  @param rhs Right-hand input register.
	 *  @return Reference to this SIMD vector after the update.
	 */
	SIMDLIB_FORCE_INLINE SimdVector &VECTORCALL operator^=(vector_t rhs) noexcept
	{
		m_data = CheckResultInactiveLanesZero(simd::bitwise_xor(m_data, rhs), "SimdVector::operator^=(vector_t)");
		return *this;
	}

#pragma endregion

#pragma region Shifting Operators

	/** @brief Shifts each integer lane left by the specified amount.
	 *  @param shift Shift count applied to every active lane.
	 *  @return SIMD vector containing the shifted values.
	 */
	SIMDLIB_FORCE_INLINE constexpr SimdVector VECTORCALL operator<<(int shift) const noexcept
	{
		return simd::shift_left(m_data, shift);
	}

	/** @brief Shifts each integer lane right by the specified amount.
	 *  @param shift Shift count applied to every active lane.
	 *  @return SIMD vector containing the shifted values using arithmetic or logical shift semantics for the element type.
	 */
	SIMDLIB_FORCE_INLINE constexpr SimdVector VECTORCALL operator>>(int shift) const noexcept
	{
		if constexpr (std::is_signed_v<element_t>)
			return simd::shift_right_arithmetic(m_data, shift);
		else
			return simd::shift_right(m_data, shift);
	}

	/** @brief Shifts each integer lane left in place.
	 *  @param shift Shift count applied to every active lane.
	 *  @return Reference to this SIMD vector after the update.
	 */
	SIMDLIB_FORCE_INLINE constexpr SimdVector &VECTORCALL operator<<=(int shift) noexcept
	{
		m_data = simd::shift_left(m_data, shift);
		return *this;
	}

	/** @brief Shifts each integer lane right in place.
	 *  @param shift Shift count applied to every active lane.
	 *  @return Reference to this SIMD vector after the update.
	 */
	SIMDLIB_FORCE_INLINE constexpr SimdVector &VECTORCALL operator>>=(int shift) noexcept
	{
		if constexpr (std::is_signed_v<element_t>)
			m_data = simd::shift_right_arithmetic(m_data, shift);
		else
			m_data = simd::shift_right(m_data, shift);
		return *this;
	}

#pragma endregion

#pragma region Comparison Operators

	/** @brief Returns true if all elements equal the corresponding element in the other vector.
	 *  @param rhs Right-hand input register.
	 *  @return `true` when every active element compares equal.
	 */
	SIMDLIB_FORCE_INLINE constexpr bool VECTORCALL operator==(vector_t rhs) const noexcept
	{
		return mask_has_all(simd::cmp_eq_mask(m_data, rhs));
	}

	/** @brief Returns true if all elements are greater than the corresponding element in the other vector.
	 *  @param rhs Right-hand input register.
	 *  @return `true` when every active element is greater than its counterpart.
	 */
	SIMDLIB_FORCE_INLINE constexpr bool VECTORCALL operator>(vector_t rhs) const noexcept
	{
		return mask_has_all(simd::cmp_gt(m_data, rhs));
	}

	/** @brief Returns true if all elements are greater than or equal to the corresponding element in the other vector.
	 *  @param rhs Right-hand input register.
	 *  @return `true` when every active element is greater than or equal to its counterpart.
	 */
	SIMDLIB_FORCE_INLINE constexpr bool VECTORCALL operator>=(vector_t rhs) const noexcept
	{
		return mask_has_all(simd::cmp_ge(m_data, rhs));
	}

	/** @brief Returns true if all elements are less than the corresponding element in the other vector.
	 *  @param rhs Right-hand input register.
	 *  @return `true` when every active element is less than its counterpart.
	 */
	SIMDLIB_FORCE_INLINE constexpr bool VECTORCALL operator<(vector_t rhs) const noexcept
	{
		return mask_has_all(simd::cmp_lt(m_data, rhs));
	}

	/** @brief Returns true if all elements are less than or equal to the corresponding element in the other vector.
	 *  @param rhs Right-hand input register.
	 *  @return `true` when every active element is less than or equal to its counterpart.
	 */
	SIMDLIB_FORCE_INLINE constexpr bool VECTORCALL operator<=(vector_t rhs) const noexcept
	{
		return mask_has_all(simd::cmp_le(m_data, rhs));
	}

	/** @brief Returns true if any element equals the corresponding element in the other vector.
	 *  @param rhs Right-hand input register.
	 *  @return `true` when at least one active element compares equal.
	 */
	SIMDLIB_FORCE_INLINE constexpr bool VECTORCALL any_equal(vector_t rhs) const noexcept
	{
		return mask_has_any(simd::cmp_eq_mask(m_data, rhs));
	}

	/** @brief Returns true if all elements equal the corresponding element in the other vector.
	 *  @param rhs Right-hand input register.
	 *  @return `true` when every active element compares equal.
	 */
	SIMDLIB_FORCE_INLINE constexpr bool VECTORCALL all_equal(vector_t rhs) const noexcept
	{
		return mask_has_all(simd::cmp_eq_mask(m_data, rhs));
	}

	/** @brief Returns true if any element is greater than the corresponding element in the other vector.
	 *  @param rhs Right-hand input register.
	 *  @return `true` when at least one active element is greater than its counterpart.
	 */
	SIMDLIB_FORCE_INLINE constexpr bool VECTORCALL any_greater(vector_t rhs) const noexcept
	{
		return mask_has_any(simd::cmp_gt(m_data, rhs));
	}

	/** @brief Returns true if all elements are greater than the corresponding element in the other vector.
	 *  @param rhs Right-hand input register.
	 *  @return `true` when every active element is greater than its counterpart.
	 */
	SIMDLIB_FORCE_INLINE constexpr bool VECTORCALL all_greater(vector_t rhs) const noexcept
	{
		return mask_has_all(simd::cmp_gt(m_data, rhs));
	}

	/** @brief Returns true if any element is greater than or equal to the corresponding element in the other vector.
	 *  @param rhs Right-hand input register.
	 *  @return `true` when at least one active element is greater than or equal to its counterpart.
	 */
	SIMDLIB_FORCE_INLINE constexpr bool VECTORCALL any_greater_equal(vector_t rhs) const noexcept
	{
		return mask_has_any(simd::cmp_ge(m_data, rhs));
	}

	/** @brief Returns true if all elements are greater than or equal to the corresponding element in the other vector.
	 *  @param rhs Right-hand input register.
	 *  @return `true` when every active element is greater than or equal to its counterpart.
	 */
	SIMDLIB_FORCE_INLINE constexpr bool VECTORCALL all_greater_equal(vector_t rhs) const noexcept
	{
		return mask_has_all(simd::cmp_ge(m_data, rhs));
	}

	/** @brief Returns true if any element is less than the corresponding element in the other vector.
	 *  @param rhs Right-hand input register.
	 *  @return `true` when at least one active element is less than its counterpart.
	 */
	SIMDLIB_FORCE_INLINE constexpr bool VECTORCALL any_less(vector_t rhs) const noexcept
	{
		return mask_has_any(simd::cmp_lt(m_data, rhs));
	}

	/** @brief Returns true if all elements are less than the corresponding element in the other vector.
	 *  @param rhs Right-hand input register.
	 *  @return `true` when every active element is less than its counterpart.
	 */
	SIMDLIB_FORCE_INLINE constexpr bool VECTORCALL all_less(vector_t rhs) const noexcept
	{
		return mask_has_all(simd::cmp_lt(m_data, rhs));
	}

	/** @brief Returns true if any element is less than or equal to the corresponding element in the other vector.
	 *  @param rhs Right-hand input register.
	 *  @return `true` when at least one active element is less than or equal to its counterpart.
	 */
	SIMDLIB_FORCE_INLINE constexpr bool VECTORCALL any_less_equal(vector_t rhs) const noexcept
	{
		return mask_has_any(simd::cmp_le(m_data, rhs));
	}

	/** @brief Returns true if all elements are less than or equal to the corresponding element in the other vector.
	 *  @param rhs Right-hand input register.
	 *  @return `true` when every active element is less than or equal to its counterpart.
	 */
	SIMDLIB_FORCE_INLINE constexpr bool VECTORCALL all_less_equal(vector_t rhs) const noexcept
	{
		return mask_has_all(simd::cmp_le(m_data, rhs));
	}

#pragma endregion

#pragma region Min/Max

	/** @brief Returns a SIMD register containing the per-element minima.
	 *  @param rhs Right-hand input register.
	 *  @return Register whose lanes are `min(m_data[i], rhs[i])`.
	 */
	SIMDLIB_FORCE_INLINE vector_t VECTORCALL min(vector_t rhs) const noexcept
	{
		return CheckResultInactiveLanesZero(simd::min(m_data, rhs), "SimdVector::min(vector_t)");
	}

	/** @brief Returns a SIMD register containing the per-element maxima.
	 *  @param rhs Right-hand input register.
	 *  @return Register whose lanes are `max(m_data[i], rhs[i])`.
	 */
	SIMDLIB_FORCE_INLINE vector_t VECTORCALL max(vector_t rhs) const noexcept
	{
		return CheckResultInactiveLanesZero(simd::max(m_data, rhs), "SimdVector::max(vector_t)");
	}

#pragma endregion

#pragma region Math Methods

	/** @brief Returns a SIMD register containing the absolute value of each element.
	 *  @return Register containing the per-element absolute values of `m_data`.
	 */
	SIMDLIB_FORCE_INLINE vector_t VECTORCALL abs() const noexcept
		requires requires(vector_t value) { simd::absolute(value); }
	{
		return simd::absolute(m_data);
	}

	/** @brief Computes the square root of each element.
	 *  @return Register containing the per-lane square roots.
	 */
	SIMDLIB_FORCE_INLINE auto VECTORCALL sqrt() const noexcept
		requires requires(vector_t value) { simd::sqrt(value); }
	{
		return simd::sqrt(m_data);
	}

	/** @brief Computes the per-128-bit-lane magnitude when the underlying Simd specialization supports it.
	 *  @return Register containing the lane-local magnitudes broadcast across each lane group.
	 */
	SIMDLIB_FORCE_INLINE auto VECTORCALL magnitude() const noexcept
		requires requires(vector_t value) { simd::magnitude(value); }
	{
		return simd::magnitude(m_data);
	}

	/** @brief Computes the multiplicative product of the active logical lanes.
	 *  @return Product of the declared logical lanes, widened to 32-bit for sub-32-bit integer vectors.
	 */
	SIMDLIB_FORCE_INLINE area_element_t VECTORCALL area() const noexcept
		requires std::is_integral_v<element_t>
	{
		if constexpr (!std::same_as<area_element_t, element_t>)
		{
			return SimdVector<area_element_t, element_count>{*this}.area();
		}
		if constexpr (element_count == 1)
		{
			if constexpr (simd_width == 128)
			{
				return static_cast<area_element_t>(simd::template extract<0>(m_data));
			}
			else
			{
				using lower_simd = Api<128, element_t>;
				return static_cast<area_element_t>(lower_simd::template extract<0>(simd::lower_half(m_data)));
			}
		}
		else
		{
			const vector_t prepared = FillInactiveLanes(m_data, static_cast<element_t>(1));
			using pair_element_t = std::conditional_t<std::is_signed_v<element_t>, typename simd::template promoted_signed_t<element_t>,
													  typename simd::template promoted_unsigned_t<element_t>>;
			using pair_vector_t = SimdVector<pair_element_t, (element_count + 1) / 2>;
			const SimdVector preparedVector{prepared};
			const pair_vector_t pairProducts{preparedVector.multiply_add_adjacent(BuildPairProductPartner(prepared))};
			return static_cast<area_element_t>(pairProducts.area());
		}
	}

	/** @brief Normalizes floating-point lanes using the Simd API's lane-local length semantics.
	 *  @return Register containing normalized per-lane values.
	 */
	SIMDLIB_FORCE_INLINE auto VECTORCALL normalize() const noexcept
		requires requires(vector_t value) { simd::normalize(value); }
	{
		return simd::normalize(m_data);
	}

	/** @brief Computes the average of corresponding lanes where the specialization supports it.
	 *  @param rhs Right-hand input register.
	 *  @return Register containing the per-lane averages.
	 */
	SIMDLIB_FORCE_INLINE auto VECTORCALL avg(vector_t rhs) const noexcept
		requires requires(vector_t lhsValue, vector_t rhsValue) { simd::avg(lhsValue, rhsValue); }
	{
		return CheckResultInactiveLanesZero(simd::avg(m_data, rhs), "SimdVector::avg(vector_t)");
	}

	/** @brief Computes a fused multiply-add where the specialization supports it.
	 *  @param rhs Right-hand multiplicand register.
	 *  @param addend Register added to the product.
	 *  @return Register containing the multiply-add result.
	 */
	SIMDLIB_FORCE_INLINE auto VECTORCALL multiply_add(vector_t rhs, vector_t addend) const noexcept
		requires requires(vector_t lhsValue, vector_t rhsValue, vector_t addValue) { simd::multiply_add(lhsValue, rhsValue, addValue); }
	{
		return CheckResultInactiveLanesZero(simd::multiply_add(m_data, rhs, addend), "SimdVector::multiply_add(vector_t, vector_t)");
	}

	/** @brief Adds adjacent lane pairs within each 128-bit lane.
	 *  @param rhs Right-hand input register.
	 *  @return Register containing pairwise horizontal sums.
	 */
	SIMDLIB_FORCE_INLINE auto VECTORCALL add_horizontal(vector_t rhs) const noexcept
		requires requires(vector_t lhsValue, vector_t rhsValue) { simd::add_horizontal(lhsValue, rhsValue); }
	{
		return simd::add_horizontal(m_data, rhs);
	}

	/** @brief Subtracts adjacent lane pairs within each 128-bit lane.
	 *  @param rhs Right-hand input register.
	 *  @return Register containing pairwise horizontal differences.
	 */
	SIMDLIB_FORCE_INLINE auto VECTORCALL subtract_horizontal(vector_t rhs) const noexcept
		requires requires(vector_t lhsValue, vector_t rhsValue) { simd::subtract_horizontal(lhsValue, rhsValue); }
	{
		return simd::subtract_horizontal(m_data, rhs);
	}

	/** @brief Adds adjacent lane pairs with saturation where the specialization supports it.
	 *  @param rhs Right-hand input register.
	 *  @return Register containing saturated horizontal sums.
	 */
	SIMDLIB_FORCE_INLINE auto VECTORCALL add_horizontal_saturated(vector_t rhs) const noexcept
		requires requires(vector_t lhsValue, vector_t rhsValue) { simd::hadd_saturated(lhsValue, rhsValue); }
	{
		return simd::hadd_saturated(m_data, rhs);
	}

	/** @brief Subtracts adjacent lane pairs with saturation where the specialization supports it.
	 *  @param rhs Right-hand input register.
	 *  @return Register containing saturated horizontal differences.
	 */
	SIMDLIB_FORCE_INLINE auto VECTORCALL subtract_horizontal_saturated(vector_t rhs) const noexcept
		requires requires(vector_t lhsValue, vector_t rhsValue) { simd::hsubtract_saturated(lhsValue, rhsValue); }
	{
		return simd::hsubtract_saturated(m_data, rhs);
	}

	/** @brief Multiplies adjacent lane pairs and accumulates them into promoted result lanes.
	 *  @param rhs Right-hand input register.
	 *  @return Register whose lane type follows the promoted integer mapping.
	 */
	SIMDLIB_FORCE_INLINE auto VECTORCALL multiply_add_adjacent(vector_t rhs) const noexcept
		requires requires(vector_t lhsValue, vector_t rhsValue) { simd::multiply_add_adjacent(lhsValue, rhsValue); }
	{
		return simd::multiply_add_adjacent(m_data, rhs);
	}

	/** @brief Multiplies raw register bytes as unsigned and signed pairs and accumulates them into signed 16-bit lanes.
	 *  @param rhs Right-hand input register whose bytes are interpreted as signed.
	 *  @return Register containing signed 16-bit accumulation results.
	 */
	SIMDLIB_FORCE_INLINE auto VECTORCALL multiply_add_unsigned_signed_bytes(vector_t rhs) const noexcept
		requires requires(vector_t lhsValue, vector_t rhsValue) { simd::multiply_add_unsigned_signed_bytes(lhsValue, rhsValue); }
	{
		return CheckResultInactiveLanesZero(simd::multiply_add_unsigned_signed_bytes(m_data, rhs), "SimdVector::multiply_add_unsigned_signed_bytes(vector_t)");
	}

	/** @brief Computes byte-wise absolute differences and accumulates them into 64-bit result lanes.
	 *  @param rhs Right-hand input register interpreted byte-wise.
	 *  @return Register containing 64-bit absolute-difference accumulations.
	 */
	SIMDLIB_FORCE_INLINE auto VECTORCALL sum_absolute_byte_differences(vector_t rhs) const noexcept
		requires requires(vector_t lhsValue, vector_t rhsValue) { simd::sum_absolute_byte_differences(lhsValue, rhsValue); }
	{
		return CheckResultInactiveLanesZero(simd::sum_absolute_byte_differences(m_data, rhs), "SimdVector::sum_absolute_byte_differences(vector_t)");
	}

	/** @brief Computes byte-window absolute-difference sums selected by a compile-time immediate mask.
	 *  @tparam imm8 Immediate control mask selecting the source windows.
	 *  @param rhs Right-hand input register interpreted byte-wise.
	 *  @return Register containing byte-window absolute-difference accumulations.
	 */
	template <int imm8>
	SIMDLIB_FORCE_INLINE auto VECTORCALL multi_sum_absolute_byte_differences(vector_t rhs) const noexcept
		requires requires(vector_t lhsValue, vector_t rhsValue) { simd::template multi_sum_absolute_byte_differences<imm8>(lhsValue, rhsValue); }
	{
		return CheckResultInactiveLanesZero(simd::template multi_sum_absolute_byte_differences<imm8>(m_data, rhs),
											"SimdVector::multi_sum_absolute_byte_differences(vector_t)");
	}

	/** @brief Returns the first index of the minimum value in the vector.
	 *  @return Zero-based index of the first minimum element.
	 */
	SIMDLIB_FORCE_INLINE std::size_t VECTORCALL min_position() const noexcept
		requires requires(vector_t value) { simd::min_position(value); }
	{
		return simd::min_position(FillInactiveLanes(m_data, std::numeric_limits<element_t>::max()));
	}

	/** @brief Returns the first index of the maximum value in the vector.
	 *  @return Zero-based index of the first maximum element.
	 */
	SIMDLIB_FORCE_INLINE std::size_t VECTORCALL max_position() const noexcept
		requires requires(vector_t value) { simd::max_position(value); }
	{
		return simd::max_position(FillInactiveLanes(m_data, std::numeric_limits<element_t>::lowest()));
	}

	/** @brief Alternates subtraction and addition across lanes for floating-point SIMD families.
	 *  @param rhs Right-hand input register.
	 *  @return Register containing alternating subtract/add results.
	 */
	SIMDLIB_FORCE_INLINE auto VECTORCALL add_subtract(vector_t rhs) const noexcept
		requires requires(vector_t lhsValue, vector_t rhsValue) { simd::add_subtract(lhsValue, rhsValue); }
	{
		return simd::add_subtract(m_data, rhs);
	}

	/** @brief Computes the scalar dot product over the vector's declared dimension count.
	 *  @param rhs Right-hand input register.
	 *  @return Scalar dot-product result for the active vector dimensions.
	 */
	SIMDLIB_FORCE_INLINE element_t VECTORCALL dot_product(vector_t rhs) const noexcept
		requires(std::is_floating_point_v<element_t> &&
				 requires(vector_t lhsValue, vector_t rhsValue) { simd::template dot_product<0x11>(lhsValue, rhsValue); })
	{
		if constexpr (std::same_as<element_t, float>)
		{
			constexpr int laneElementCount = 4;
			constexpr int lowActiveCount = element_count < laneElementCount ? element_count : laneElementCount;
			constexpr int lowMask = (((1 << lowActiveCount) - 1) << 4) | 0x1;
			const auto partial = simd::template dot_product<lowMask>(m_data, rhs);
			element_t result = simd::get_element(partial, 0);

			if constexpr (simd_width == 256 && element_count > laneElementCount)
			{
				result = static_cast<element_t>(result + simd::get_element(partial, laneElementCount));
			}

			return result;
		}
		else
		{
			constexpr int laneElementCount = 2;
			constexpr int lowActiveCount = element_count < laneElementCount ? element_count : laneElementCount;
			constexpr int lowMask = (((1 << lowActiveCount) - 1) << 4) | 0x1;
			const auto partial = simd::template dot_product<lowMask>(m_data, rhs);
			element_t result = simd::get_element(partial, 0);

			if constexpr (simd_width == 256 && element_count > laneElementCount)
			{
				result = static_cast<element_t>(result + simd::get_element(partial, laneElementCount));
			}

			return result;
		}
	}

	/** @brief Clamps each element between the corresponding minimum and maximum elements.
	 *  @param minValue Register containing the per-element lower bounds.
	 *  @param maxValue Register containing the per-element upper bounds.
	 *  @return Register containing `m_data` clamped to `[minValue, maxValue]` per lane.
	 */
	SIMDLIB_FORCE_INLINE vector_t VECTORCALL clamp(vector_t minValue, vector_t maxValue) const noexcept
		requires requires(vector_t value) {
			simd::min(value, value);
			simd::max(value, value);
		}
	{
		minValue = FillInactiveLanes(minValue, element_t{});
		maxValue = FillInactiveLanes(maxValue, element_t{});
		const vector_t result = simd::max(minValue, simd::min(m_data, maxValue));
		return CheckResultInactiveLanesZero(FillInactiveLanes(result, element_t{}), "SimdVector::clamp(vector_t, vector_t)");
	}

	/** @brief Clamps each element between the provided scalar minimum and maximum bounds.
	 *  @param minValue Scalar lower bound broadcast to every lane.
	 *  @param maxValue Scalar upper bound broadcast to every lane.
	 *  @return Register containing `m_data` clamped to `[minValue, maxValue]` per lane.
	 */
	SIMDLIB_FORCE_INLINE vector_t VECTORCALL clamp(element_t minValue, element_t maxValue) const noexcept
		requires requires(vector_t value) {
			simd::min(value, value);
			simd::max(value, value);
		}
	{
		return this->clamp(SimdVector{minValue}.getRegister(), SimdVector{maxValue}.getRegister());
	}

	/** @brief Returns the sign of each element as -1, 0, or 1, or 0 and 1 for unsigned types.
	 *  @return Register containing the per-element sign classification of `m_data`.
	 */
	SIMDLIB_FORCE_INLINE vector_t VECTORCALL sign() const noexcept
		requires requires(vector_t value) {
			simd::cmpgt(value, value);
			simd::bitwise_and(value, value);
			simd::bitwise_or(value, value);
		}
	{
		const vector_t zero = simd::setzero();
		const vector_t positiveMask = simd::cmpgt(m_data, zero);
		const vector_t positiveOne = simd::set1(static_cast<element_t>(1));
		const vector_t positive = simd::bitwise_and(positiveMask, positiveOne);

		if constexpr (std::is_signed_v<element_t>)
		{
			const vector_t negativeMask = simd::cmpgt(zero, m_data);
			const vector_t negativeOne = simd::set1(static_cast<element_t>(-1));
			const vector_t negative = simd::bitwise_and(negativeMask, negativeOne);
			return simd::bitwise_or(positive, negative);
		}
		else
		{
			return positive;
		}
	}

#pragma endregion

#pragma region Conversion Operators

	/** @brief Implicitly converts this wrapper to the underlying SIMD register.
	 *  @return Copy of the wrapped SIMD register.
	 */
	SIMDLIB_FORCE_INLINE VECTORCALL operator vector_t() const noexcept
	{
		return m_data;
	}

	/** @brief Returns a mutable span view over the underlying register storage.
	 *  @return Mutable span covering every hardware lane in the register.
	 */
	SIMDLIB_FORCE_INLINE operator std::span<element_t, simd::element_count>() noexcept
	{
		return std::span<element_t, simd::element_count>(Detail::register_data<element_t>(m_data), simd::element_count);
	}

	/** @brief Returns a readonly span view over the underlying register storage.
	 *  @return Readonly span covering every hardware lane in the register.
	 */
	SIMDLIB_FORCE_INLINE operator std::span<const element_t, simd::element_count>() const noexcept
	{
		return std::span<const element_t, simd::element_count>(Detail::register_data<element_t>(m_data), simd::element_count);
	}

	/** @brief Converts the wrapped SIMD register to a fixed array.
	 *  @return Array containing the full underlying register contents in lane order.
	 */
	SIMDLIB_FORCE_INLINE constexpr explicit operator std::array<element_t, simd::element_count>() const noexcept
	{
		return simd::to_array(m_data);
	}

	/** @brief Converts the SIMD vector to an array of elements.
	 *  @return Array containing the full underlying register contents in lane order.
	 */
	SIMDLIB_FORCE_INLINE constexpr std::array<element_t, simd::element_count> toArray() const noexcept
	{
		return static_cast<std::array<element_t, simd::element_count>>(*this);
	}

	/** @brief Returns a span over the SIMD vector's elements.
	 *  @return Mutable span view of the full underlying register storage.
	 */
	SIMDLIB_FORCE_INLINE std::span<element_t, simd::element_count> getSpan() noexcept
	{
		return static_cast<std::span<element_t, simd::element_count>>(*this);
	}

	/** @brief Returns a readonly span over the SIMD vector's elements.
	 *  @return Readonly span view of the full underlying register storage.
	 */
	SIMDLIB_FORCE_INLINE std::span<const element_t, simd::element_count> getSpan() const noexcept
	{
		return static_cast<std::span<const element_t, simd::element_count>>(*this);
	}

	/** @brief Returns the underlying SIMD register.
	 *  @return Mutable reference to the wrapped SIMD register.
	 */
	SIMDLIB_FORCE_INLINE vector_t &VECTORCALL getRegister() noexcept
	{
		return m_data;
	}

	/** @brief Returns the underlying SIMD register.
	 *  @return Copy of the wrapped SIMD register.
	 */
	SIMDLIB_FORCE_INLINE vector_t VECTORCALL getRegister() const noexcept
	{
		return m_data;
	}

	/** @brief Returns a tuple containing the span view used by tuple-like integrations.
	 *  @return Tuple containing the readonly span view of this SIMD vector.
	 */
	SIMDLIB_FORCE_INLINE constexpr auto getTuple() const noexcept
	{
		return std::tuple{this->getSpan()};
	}

#pragma endregion

#pragma region Accessors

	/** @brief Returns the element at the requested lane index.
	 *  @param index Zero-based lane index within the full register view.
	 *  @return Copy of the element stored at `index`.
	 */
	constexpr inline element_t operator[](const std::size_t index) const noexcept
	{
		return this->getSpan()[index];
	}

	/** @brief Returns a mutable reference to the element at the requested lane index.
	 *  @param index Zero-based lane index within the full register view.
	 *  @return Mutable reference to the element stored at `index`.
	 */
	constexpr inline element_t &operator[](const std::size_t index) noexcept
	{
		return this->getSpan()[index];
	}

#pragma region Named Element Accessors

	/** @brief Returns a mutable reference to the first element.
	 *  @return Mutable reference to element `0`.
	 */
	constexpr inline element_t &x() noexcept
		requires(element_count > 0)
	{
		return operator[](0);
	}

	/** @brief Returns a mutable reference to the second element.
	 *  @return Mutable reference to element `1`.
	 */
	constexpr inline element_t &y() noexcept
		requires(element_count > 1)
	{
		return operator[](1);
	}

	/** @brief Returns a mutable reference to the third element.
	 *  @return Mutable reference to element `2`.
	 */
	constexpr inline element_t &z() noexcept
		requires(element_count > 2)
	{
		return operator[](2);
	}

	/** @brief Returns a mutable reference to the fourth element.
	 *  @return Mutable reference to element `3`.
	 */
	constexpr inline element_t &w() noexcept
		requires(element_count > 3)
	{
		return operator[](3);
	}

	/** @brief Returns the first element.
	 *  @return Copy of element `0`.
	 */
	constexpr inline element_t x() const noexcept
		requires(element_count > 0)
	{
		return operator[](0);
	}

	/** @brief Returns the second element.
	 *  @return Copy of element `1`.
	 */
	constexpr inline element_t y() const noexcept
		requires(element_count > 1)
	{
		return operator[](1);
	}

	/** @brief Returns the third element.
	 *  @return Copy of element `2`.
	 */
	constexpr inline element_t z() const noexcept
		requires(element_count > 2)
	{
		return operator[](2);
	}

	/** @brief Returns the fourth element.
	 *  @return Copy of element `3`.
	 */
	constexpr inline element_t w() const noexcept
		requires(element_count > 3)
	{
		return operator[](3);
	}

#pragma endregion

#pragma endregion

#pragma region Internal
#pragma endregion
};

#pragma region Hashing

} // namespace SimdLib

namespace std
{
template <class element_t, int element_count> struct hash<SimdLib::SimdVector<element_t, element_count>>
{
	[[nodiscard]] constexpr size_t operator()(const SimdLib::SimdVector<element_t, element_count> &value) const noexcept
	{
		const auto lanes = value.toArray();
		const auto fmix32 = [](uint32_t mixed) constexpr noexcept -> uint32_t
		{
			mixed ^= mixed >> 16;
			mixed *= 0x85EBCA6Bu;
			mixed ^= mixed >> 13;
			mixed *= 0xC2B2AE35u;
			mixed ^= mixed >> 16;
			return mixed;
		};

		const auto fmix64 = [](uint64_t mixed) constexpr noexcept -> uint64_t
		{
			mixed ^= mixed >> 33;
			mixed *= 0xFF51AFD7ED558CCDull;
			mixed ^= mixed >> 33;
			mixed *= 0xC4CEB9FE1A85EC53ull;
			mixed ^= mixed >> 33;
			return mixed;
		};

		if constexpr (sizeof(element_t) <= sizeof(uint32_t))
		{
			constexpr std::array<uint32_t, 4> salts{
				0u,
				0x9E3779B9u,
				0x7F4A7C15u,
				0x94D049BBu,
			};

			uint32_t combined = fmix32(static_cast<uint32_t>(element_count) ^ 0xD6E8FEB9u);
			for (int index = 0; index < element_count; ++index)
			{
				uint32_t part;
				if constexpr (std::is_floating_point_v<element_t>)
				{
					part = lanes[index] == element_t{0} ? 0u : std::bit_cast<uint32_t>(lanes[index]);
				}
				else if constexpr (std::is_signed_v<element_t>)
				{
					using unsigned_t = std::make_unsigned_t<element_t>;
					part = static_cast<uint32_t>(static_cast<unsigned_t>(lanes[index]));
				}
				else
				{
					part = static_cast<uint32_t>(lanes[index]);
				}

				combined ^= fmix32(part ^ salts[static_cast<std::size_t>(index) % salts.size()] ^ (0x27D4EB2Du * static_cast<uint32_t>(index)));
			}

			return fmix32(combined);
		}
		else
		{
			constexpr std::array<uint64_t, 4> salts{
				0ull,
				0x9E3779B97F4A7C15ull,
				0xC2B2AE3D27D4EB4Full,
				0x165667B19E3779F9ull,
			};

			uint64_t combined = fmix64(static_cast<uint64_t>(element_count) ^ 0xD6E8FEB86659FD93ull);
			for (int index = 0; index < element_count; ++index)
			{
				uint64_t part;
				if constexpr (std::is_floating_point_v<element_t>)
				{
					part = lanes[index] == element_t{0} ? 0ull : std::bit_cast<uint64_t>(lanes[index]);
				}
				else if constexpr (std::is_signed_v<element_t>)
				{
					using unsigned_t = std::make_unsigned_t<element_t>;
					part = static_cast<uint64_t>(static_cast<unsigned_t>(lanes[index]));
				}
				else
				{
					part = static_cast<uint64_t>(lanes[index]);
				}

				combined ^= fmix64(part ^ salts[static_cast<std::size_t>(index) % salts.size()] ^ (0x9E3779B97F4A7C15ull * static_cast<uint64_t>(index)));
			}

			const uint64_t finalHash = fmix64(combined);
			if constexpr (sizeof(size_t) >= sizeof(uint64_t))
			{
				return static_cast<size_t>(finalHash);
			}
			else
			{
				return static_cast<size_t>(finalHash ^ (finalHash >> 32));
			}
		}
	}
};

} // namespace std

namespace SimdLib
{

#pragma endregion

#pragma region Vector Types

using VectorInt8 = SimdVector<int8_t, 4>;
using VectorUInt8 = SimdVector<uint8_t, 4>;

using VectorInt16 = SimdVector<int16_t, 4>;
using VectorUInt16 = SimdVector<uint16_t, 4>;

using VectorInt32 = SimdVector<int32_t, 4>;
using VectorUInt32 = SimdVector<uint32_t, 4>;

using VectorInt64 = SimdVector<int64_t, 4>;
using VectorUInt64 = SimdVector<uint64_t, 4>;

#pragma endregion

#pragma region Type Aliases (Unsigned)

using uint8x16 = SimdVector<uint8_t, 16>;
using uint8x32 = SimdVector<uint8_t, 32>;

using uint16x8 = SimdVector<uint16_t, 8>;
using uint16x16 = SimdVector<uint16_t, 16>;

using uint32x4 = SimdVector<uint32_t, 4>;
using uint32x8 = SimdVector<uint32_t, 8>;

using uint64x2 = SimdVector<uint64_t, 2>;
using uint64x4 = SimdVector<uint64_t, 4>;

#pragma endregion

#pragma region Type Aliases (Signed)

using int8x16 = SimdVector<int8_t, 16>;
using int8x32 = SimdVector<int8_t, 32>;

using int16x8 = SimdVector<int16_t, 8>;
using int16x16 = SimdVector<int16_t, 16>;

using int32x4 = SimdVector<int32_t, 4>;
using int32x8 = SimdVector<int32_t, 8>;

using int64x2 = SimdVector<int64_t, 2>;
using int64x4 = SimdVector<int64_t, 4>;

#pragma endregion

} // namespace SimdLib
