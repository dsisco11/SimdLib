#include <SimdLib/Register.h>

#if !SIMDLIB_REGISTER_INTERFACE_AVAILABLE
#error "The Register positive probe requires the computed interface availability"
#endif

#if !SIMDLIB_REQUIRE_REGISTER_INTERFACE
#error "SimdLib::Register must publish its requirement signal"
#endif

#if defined(__clang__) || defined(__GNUC__)
#if !defined(__cpp_explicit_this_parameter) || __cpp_explicit_this_parameter < 202110L
#error "Clang and GCC Register support must use the standard explicit-object feature macro"
#endif
#endif

#if defined(_MSC_VER) && !defined(__clang__) && _MSVC_LANG <= 202002L
#error "Microsoft C++ Register support requires a post-C++20 language mode"
#endif

/** @brief Exercises the explicit-object declaration forms required by Register. */
struct RegisterExplicitObjectProbe
{
	int value;

	/**
	 * @brief Returns the stored value through a by-value explicit object parameter.
	 * @return Stored probe value.
	 */
	[[nodiscard]] constexpr int VECTORCALL get(this RegisterExplicitObjectProbe self) noexcept
	{
		return self.value;
	}

	/**
	 * @brief Adds two probe values through a by-value explicit object operator.
	 * @param rhs Right operand.
	 * @return Sum of both probe values.
	 */
	[[nodiscard]] constexpr RegisterExplicitObjectProbe VECTORCALL operator+(this RegisterExplicitObjectProbe lhs,
																			 const RegisterExplicitObjectProbe rhs) noexcept
	{
		return {lhs.value + rhs.value};
	}

	/**
	 * @brief Mutates a probe through a reference explicit object parameter.
	 * @param rhs Value added to the probe.
	 * @return Reference to the mutated probe.
	 */
	constexpr RegisterExplicitObjectProbe &VECTORCALL operator+=(this RegisterExplicitObjectProbe &self, const RegisterExplicitObjectProbe rhs) noexcept
	{
		self.value += rhs.value;
		return self;
	}

	/**
	 * @brief Compares two probes through a by-value explicit object operator.
	 * @param rhs Right operand.
	 * @return `true` when both values are equal.
	 */
	[[nodiscard]] constexpr bool VECTORCALL operator==(this RegisterExplicitObjectProbe lhs, const RegisterExplicitObjectProbe rhs) noexcept
	{
		return lhs.value == rhs.value;
	}
};

/**
 * @brief Verifies named, arithmetic, comparison, and mutating explicit-object declarations.
 * @return `true` when every declaration produces its expected value.
 */
consteval bool register_explicit_object_probe_succeeds()
{
	RegisterExplicitObjectProbe value{1};
	value += RegisterExplicitObjectProbe{2};
	return value.get() == 3 && value + RegisterExplicitObjectProbe{4} == RegisterExplicitObjectProbe{7};
}

static_assert(register_explicit_object_probe_succeeds());
