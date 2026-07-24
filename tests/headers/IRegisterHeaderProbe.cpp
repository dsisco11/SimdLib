#include <SimdLib/IRegister.h>

namespace
{

/** @brief Minimal API metadata used by the standalone Register interface probe. */
struct ApiShape
{
	using mask_t = unsigned int;
};

/** @brief Minimal predicate type used by the standalone Register interface probe. */
struct MaskShape
{
};

/** @brief Minimal aggregate metadata shape used to verify the standalone Register interface header. */
struct RegisterShape
{
	using element_type = int;
	using api_type = ApiShape;
	using native_type = int;
	using mask_type = MaskShape;

	constexpr static inline std::size_t register_width = 128;
	constexpr static inline std::size_t byte_count = 16;
	constexpr static inline std::size_t lane_count = 4;

	native_type native{};
};

static_assert(SimdLib::IRegister::Type<RegisterShape>);
static_assert(SimdLib::IRegister::Shape<RegisterShape, int, 128>);
static_assert(!SimdLib::IRegister::Zero<RegisterShape>);
static_assert(!SimdLib::IRegister::Add<RegisterShape>);
static_assert(!SimdLib::IRegister::LowerHalf<RegisterShape>);
static_assert(!SimdLib::IRegister::UnpackLow<RegisterShape>);
static_assert(!SimdLib::IRegister::UnpackHigh<RegisterShape>);
static_assert(!SimdLib::IRegister::Shuffle<RegisterShape, 0>);
static_assert(!SimdLib::IRegister::ShuffleLow<RegisterShape, 0>);
static_assert(!SimdLib::IRegister::ShuffleHigh<RegisterShape, 0>);
static_assert(!SimdLib::IRegister::Blend<RegisterShape, 0>);
static_assert(!SimdLib::IRegister::BitCast<RegisterShape, float>);
static_assert(!SimdLib::IRegister::Convert<RegisterShape, float>);
static_assert(!SimdLib::IRegister::WidenLow<RegisterShape, long long, 256>);

} // namespace
