#include "EngineClassData.hpp"
#include "Velocities-inl.h"
#include <cstdint>
#include <vector>
#include <xsimd/xsimd.hpp>

namespace Blokk
{

template <class Arch>
void EngineCore::UpdateRangeOfPositionsFn(Worker& worker, IndexRange& Range)
{
    float* XPos = &m_XPositions[Range.Start];
    float* YPos = &m_YPositions[Range.Start];
    const float* XVel = &m_XVelocities[Range.Start];
    const float* YVel = &m_YVelocities[Range.Start];

    const uint32_t Size = Range.GetSize();
    const float delta = static_cast<float>(m_DeltaTime.secs);

    ProcessVelocities_SIMD<Arch>(XPos, YPos, XVel, YVel, Size, delta);
}

} // namespace Blokk