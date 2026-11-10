#if (Blokk_Rendering_CullType == 1)
#include "Visibility_AxisCulling-inl.h"
#include <cstdint>
#include <vector>
#include <xsimd/xsimd.hpp>

namespace Blokk
{

template <typename Arch>
void EngineCore::CheckVisibilityFn_Axis(Worker& worker, IndexRange& Range)
{
#ifdef Blokk_CamEnabled
    const float CameraX =
        static_cast<float>(Camera.GetXPosition());

    const float CameraY =
        static_cast<float>(Camera.GetYPosition());
#endif
    IndexRange Range = worker.getRange();

    const std::uint32_t* Widths = &AnimWidths[Range.Start];
    const std::uint32_t* Heights = &AnimHeights[Range.Start];
    const float* XPos = &XPositions[Range.Start];
    const float* YPos = &YPositions[Range.Start];
    const std::uint32_t Size = Range.GetSize();

    worker->IdxResult = CheckVisible_Axis_SIMD<Arch>(
        ScreenWidth,
        ScreenHeight,
        Widths,
        Heights,
        XPos,
        YPos,
        Size,
        Range.Start
#ifdef Blokk_CamEnabled
        , CameraX,
        CameraY
#endif
    );
}

} // namespace Blokk

#endif // Blokk_Rendering_CullType == 1

