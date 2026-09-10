#include <SimdLib/Register.h>

#include <cstdint>

namespace SimdLib::Tests
{
using ByteRegister = SimdLib::Register<std::uint8_t, 128>;

/**
 * @brief Exercises a comparison result as a chained RegisterMask expression.
 * @param lhs Left comparison operand.
 * @param rhs Right comparison operand.
 * @return `true` when every lane in `lhs` is less than or equal to its peer in `rhs`.
 */
[[nodiscard]] bool compare_all_less_equal(const ByteRegister lhs, const ByteRegister rhs) noexcept
{
	return lhs.compare_less_equal(rhs).all();
}
} // namespace SimdLib::Tests
