#include <SimdLib/IApi.h>

namespace
{

/** @brief Minimal metadata-only type used to verify the standalone API interface header. */
struct ApiShape
{
	using element_type = int;
	using vector_t = int;
	constexpr static inline std::size_t register_width = 128;
};

static_assert(SimdLib::IApi::Type<ApiShape>);
static_assert(SimdLib::IApi::WidenTarget<ApiShape>);
static_assert(!SimdLib::IApi::Add<ApiShape>);
static_assert(!SimdLib::ApiAvailable<128, bool>);
static_assert(!SimdLib::NativeApiAvailable<bool>);

} // namespace
