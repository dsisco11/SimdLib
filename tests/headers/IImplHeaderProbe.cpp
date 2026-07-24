#include <SimdLib/IImpl.h>

namespace
{

/** @brief Minimal metadata-only type used to verify the standalone implementation interface header. */
struct ImplementationShape
{
	using vector_t = int;
};

static_assert(SimdLib::IImpl::Mapping<ImplementationShape>);
static_assert(!SimdLib::IImpl::Add<ImplementationShape>);
static_assert(!SimdLib::IImpl::SetZero<ImplementationShape>);

} // namespace
