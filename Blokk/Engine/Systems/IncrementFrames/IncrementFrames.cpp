
#include <cstdint>

#include <xsimd/xsimd.hpp>

#include "EngineClassData.hpp"
#include "IncrementFrames-inl.h"

namespace Blokk
{

template<typename Arch>
void EngineCore::IncrementFramesFn(
    std::vector<std::uint32_t>& FrameNums,
    std::uint32_t Size
) {
    IncrementFramesImpl<Arch>(FrameNums.data(), Size);
}


} // namespace Blokk

