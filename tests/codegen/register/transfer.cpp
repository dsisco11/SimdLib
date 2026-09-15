#include <SimdLib/Register.h>
#include <span>

using value_type = SimdLib::Register<float, SIMDLIB_CONTRACT_WIDTH>;

/** @brief Loads two full logical registers, adds once, and stores the result. */
extern "C" void simdlib_contract_transfer_load_operate_store(
    const float* lhs, const float* rhs, float* destination) noexcept
{
    const auto result = value_type::load(std::span<const float, value_type::lane_count>(lhs, value_type::lane_count))
        + value_type::load(std::span<const float, value_type::lane_count>(rhs, value_type::lane_count));
    result.store(std::span<float, value_type::lane_count>(destination, value_type::lane_count));
}
