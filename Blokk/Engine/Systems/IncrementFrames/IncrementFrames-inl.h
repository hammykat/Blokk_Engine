#pragma once

#include <cstdint>

#include <xsimd/xsimd.hpp>

namespace Blokk
{

template <class Arch>
void IncrementFramesImpl(
    uint32_t* FrameNumbers,
    uint32_t Size
)
{
    using Batch = xsimd::batch<std::uint32_t, Arch>;

    constexpr std::size_t Lanes = Batch::size;

    const Batch One(1);

    std::uint32_t i = 0;

    for (; i + Lanes <= Size;
         i += static_cast<std::uint32_t>(Lanes))
    {
        Batch FrameData =
            xsimd::load_unaligned(FrameNumbers + i);

        FrameData += One;

        FrameData.store_unaligned(FrameNumbers + i);
    }

    // Remaining elements
    for (; i < Size; ++i)
    {
        ++FrameNumbers[i];
    }
}

}
