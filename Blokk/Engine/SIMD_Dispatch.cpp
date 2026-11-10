#include "EngineClassData.hpp"
namespace Blokk 
{
    // Get the right function implementations according to the user's SIMD
    void EngineCore::GetFunctions()
    {
        #if (Blokk_Visibility_CullType == 0)
            CheckVisibleRangeFn = CheckVisibilityFn_Basic;
        #elif (Blokk_Visibility_CullType == 1)
            CheckVisibleRangeFn = CheckVisibilityFn_Axis;
        #endif

        switch (m_SIMDRegisterLevel)
        {
            case SIMDLevel::Scalar:
                #if (Blokk_Visibility_CullType == 0)
                    CheckVisibleRangeFn = &EngineCore::CheckVisibilityFn_Basic<xsimd::default_arch>;
                #elif (Blokk_Visibility_CullType == 1)
                    CheckVisibleRangeFn = &EngineCore::CheckVisibilityFn_Axis<xsimd::default_arch>;  
                #endif
                IncrementFrames = &EngineCore::IncrementFramesFn<xsimd::default_arch>;
                UpdatePositions = &EngineCore::UpdateRangeOfPositions<xsimd::default_arch>;
                return;

            // x86
            case SIMDLevel::SSE2:
                #if (Blokk_Visibility_CullType == 0)
                    CheckVisibleRangeFn = &EngineCore::CheckVisibilityFn_Basic<xsimd::sse2>;
                #elif (Blokk_Visibility_CullType == 1)
                    CheckVisibleRangeFn = &EngineCore::CheckVisibilityFn_Axis<xsimd::sse2>;  
                #endif
                IncrementFrames = &EngineCore::IncrementFramesFn<xsimd::sse2>;
                UpdatePositions = &EngineCore::UpdateRangeOfPositions<xsimd::sse2>;
                return;
            case SIMDLevel::SSE3:
                #if (Blokk_Visibility_CullType == 0)
                    CheckVisibleRangeFn = &EngineCore::CheckVisibilityFn_Basic<xsimd::sse3>;
                #elif (Blokk_Visibility_CullType == 1)
                    CheckVisibleRangeFn = &EngineCore::CheckVisibilityFn_Axis<xsimd::sse3>;  
                #endif
                IncrementFrames = &EngineCore::IncrementFramesFn<xsimd::sse3>;
                UpdatePositions = &EngineCore::UpdateRangeOfPositions<xsimd::sse3>;
                return;
            case SIMDLevel::SSSE3:
                #if (Blokk_Visibility_CullType == 0)
                    CheckVisibleRangeFn = &EngineCore::CheckVisibilityFn_Basic<xsimd::ssse3>;
                #elif (Blokk_Visibility_CullType == 1)
                    CheckVisibleRangeFn = &EngineCore::CheckVisibilityFn_Axis<xsimd::ssse3>;  
                #endif
                IncrementFrames = &EngineCore::IncrementFramesFn<xsimd::ssse3>;
                UpdatePositions = &EngineCore::UpdateRangeOfPositions<xsimd::ssse3>;
                return;
            case SIMDLevel::SSE4_1:
                #if (Blokk_Visibility_CullType == 0)
                    CheckVisibleRangeFn = &EngineCore::CheckVisibilityFn_Basic<xsimd::sse4_1>;
                #elif (Blokk_Visibility_CullType == 1)
                    CheckVisibleRangeFn = &EngineCore::CheckVisibilityFn_Axis<xsimd::sse4_1>;  
                #endif
                IncrementFrames = &EngineCore::IncrementFramesFn<xsimd::sse4_1>;
                UpdatePositions = &EngineCore::UpdateRangeOfPositions<xsimd::sse4_1>;
                return;
            case SIMDLevel::SSE4_2:
                #if (Blokk_Visibility_CullType == 0)
                    CheckVisibleRangeFn = &EngineCore::CheckVisibilityFn_Basic<xsimd::sse4_2>;
                #elif (Blokk_Visibility_CullType == 1)
                    CheckVisibleRangeFn = &EngineCore::CheckVisibilityFn_Axis<xsimd::sse4_2>;  
                #endif
                IncrementFrames = &EngineCore::IncrementFramesFn<xsimd::sse4_2>;
                UpdatePositions = &EngineCore::UpdateRangeOfPositions<xsimd::sse4_2>;
                return;

            case SIMDLevel::AVX:
                #if (Blokk_Visibility_CullType == 0)
                    CheckVisibleRangeFn = &EngineCore::CheckVisibilityFn_Basic<xsimd::avx>;
                #elif (Blokk_Visibility_CullType == 1)
                    CheckVisibleRangeFn = &EngineCore::CheckVisibilityFn_Axis<xsimd::avx>;  
                #endif
                IncrementFrames = &EngineCore::IncrementFramesFn<xsimd::avx>;
                UpdatePositions = &EngineCore::UpdateRangeOfPositions<xsimd::avx>;
                return;
            case SIMDLevel::AVX2:
                #if (Blokk_Visibility_CullType == 0)
                    CheckVisibleRangeFn = &EngineCore::CheckVisibilityFn_Basic<xsimd::avx2>;
                #elif (Blokk_Visibility_CullType == 1)
                    CheckVisibleRangeFn = &EngineCore::CheckVisibilityFn_Axis<xsimd::avx2>;  
                #endif
                IncrementFrames = &EngineCore::IncrementFramesFn<xsimd::avx2>;
                UpdatePositions = &EngineCore::UpdateRangeOfPositions<xsimd::avx2>;
                return;

            case SIMDLevel::AVX512_128:
                #if (Blokk_Visibility_CullType == 0)
                    CheckVisibleRangeFn = &EngineCore::CheckVisibilityFn_Basic<xsimd::avx512vl_128>;
                #elif (Blokk_Visibility_CullType == 1)
                    CheckVisibleRangeFn = &EngineCore::CheckVisibilityFn_Axis<xsimd::avx512vl_128>;  
                #endif
                IncrementFrames = &EngineCore::IncrementFramesFn<xsimd::avx512vl_128>;
                UpdatePositions = &EngineCore::UpdateRangeOfPositions<xsimd::avx512vl_128>;
                return;
            case SIMDLevel::AVX512_256:
                #if (Blokk_Visibility_CullType == 0)
                    CheckVisibleRangeFn = &EngineCore::CheckVisibilityFn_Basic<xsimd::avx512vl_256>;
                #elif (Blokk_Visibility_CullType == 1)
                    CheckVisibleRangeFn = &EngineCore::CheckVisibilityFn_Axis<xsimd::avx512vl_256>;  
                #endif
                IncrementFrames = &EngineCore::IncrementFramesFn<xsimd::avx512vl_256>;
                UpdatePositions = &EngineCore::UpdateRangeOfPositions<xsimd::avx512vl_256>;
                return;

            case SIMDLevel::AVX512F:
                #if (Blokk_Visibility_CullType == 0)
                    CheckVisibleRangeFn = &EngineCore::CheckVisibilityFn_Basic<xsimd::avx512f>;
                #elif (Blokk_Visibility_CullType == 1)
                    CheckVisibleRangeFn = &EngineCore::CheckVisibilityFn_Axis<xsimd::avx512f>;  
                #endif
                IncrementFrames = &EngineCore::IncrementFramesFn<xsimd::avx512f>;
                UpdatePositions = &EngineCore::UpdateRangeOfPositions<xsimd::avx512f>;
                return;
            case SIMDLevel::AVX512CD:
                #if (Blokk_Visibility_CullType == 0)
                    CheckVisibleRangeFn = &EngineCore::CheckVisibilityFn_Basic<xsimd::avx512cd>;
                #elif (Blokk_Visibility_CullType == 1)
                    CheckVisibleRangeFn = &EngineCore::CheckVisibilityFn_Axis<xsimd::avx512cd>;  
                #endif
                IncrementFrames = &EngineCore::IncrementFramesFn<xsimd::avx512cd>;
                UpdatePositions = &EngineCore::UpdateRangeOfPositions<xsimd::avx512cd>;
                return;
            case SIMDLevel::AVX512DQ:
                #if (Blokk_Visibility_CullType == 0)
                    CheckVisibleRangeFn = &EngineCore::CheckVisibilityFn_Basic<xsimd::avx512dq>;
                #elif (Blokk_Visibility_CullType == 1)
                    CheckVisibleRangeFn = &EngineCore::CheckVisibilityFn_Axis<xsimd::avx512dq>;  
                #endif
                IncrementFrames = &EngineCore::IncrementFramesFn<xsimd::avx512dq>;
                UpdatePositions = &EngineCore::UpdateRangeOfPositions<xsimd::avx512dq>;
                return;
            case SIMDLevel::AVX512BW:
                #if (Blokk_Visibility_CullType == 0)
                    CheckVisibleRangeFn = &EngineCore::CheckVisibilityFn_Basic<xsimd::avx512bw>;
                #elif (Blokk_Visibility_CullType == 1)
                    CheckVisibleRangeFn = &EngineCore::CheckVisibilityFn_Axis<xsimd::avx512bw>;  
                #endif
                IncrementFrames = &EngineCore::IncrementFramesFn<xsimd::avx512bw>;
                UpdatePositions = &EngineCore::UpdateRangeOfPositions<xsimd::avx512bw>;
                return;
            case SIMDLevel::AVX512VL:
                #if (Blokk_Visibility_CullType == 0)
                    CheckVisibleRangeFn = &EngineCore::CheckVisibilityFn_Basic<xsimd::avx512vl>;
                #elif (Blokk_Visibility_CullType == 1)
                    CheckVisibleRangeFn = &EngineCore::CheckVisibilityFn_Axis<xsimd::avx512vl>;  
                #endif
                IncrementFrames = &EngineCore::IncrementFramesFn<xsimd::avx512vl>;
                UpdatePositions = &EngineCore::UpdateRangeOfPositions<xsimd::avx512vl>;
                return;

            case SIMDLevel::AVX512IFMA:
                #if (Blokk_Visibility_CullType == 0)
                    CheckVisibleRangeFn = &EngineCore::CheckVisibilityFn_Basic<xsimd::avx512ifma>;
                #elif (Blokk_Visibility_CullType == 1)
                    CheckVisibleRangeFn = &EngineCore::CheckVisibilityFn_Axis<xsimd::avx512ifma>;  
                #endif
                IncrementFrames = &EngineCore::IncrementFramesFn<xsimd::avx512ifma>;
                UpdatePositions = &EngineCore::UpdateRangeOfPositions<xsimd::avx512ifma>;
                return;
            case SIMDLevel::AVX512VBMI:
                #if (Blokk_Visibility_CullType == 0)
                    CheckVisibleRangeFn = &EngineCore::CheckVisibilityFn_Basic<xsimd::avx512vbmi>;
                #elif (Blokk_Visibility_CullType == 1)
                    CheckVisibleRangeFn = &EngineCore::CheckVisibilityFn_Axis<xsimd::avx512vbmi>;  
                #endif
                IncrementFrames = &EngineCore::IncrementFramesFn<xsimd::avx512vbmi>;
                UpdatePositions = &EngineCore::UpdateRangeOfPositions<xsimd::avx512vbmi>;
                return;
            case SIMDLevel::AVX512VBMI2:
                #if (Blokk_Visibility_CullType == 0)
                    CheckVisibleRangeFn = &EngineCore::CheckVisibilityFn_Basic<xsimd::avx512vbmi2>;
                #elif (Blokk_Visibility_CullType == 1)
                    CheckVisibleRangeFn = &EngineCore::CheckVisibilityFn_Axis<xsimd::avx512vbmi2>;  
                #endif
                IncrementFrames = &EngineCore::IncrementFramesFn<xsimd::avx512vbmi2>;
                UpdatePositions = &EngineCore::UpdateRangeOfPositions<xsimd::avx512vbmi2>;
                return;
            case SIMDLevel::AVX512VNNI:
                #if (Blokk_Visibility_CullType == 0)
                    CheckVisibleRangeFn = &EngineCore::CheckVisibilityFn_Basic<xsimd::avx512vnni<xsimd::avx512vbmi2>>;
                #elif (Blokk_Visibility_CullType == 1)
                    CheckVisibleRangeFn = &EngineCore::CheckVisibilityFn_Axis<xsimd::avx512vnni<xsimd::avx512vbmi2>>;  
                #endif
                IncrementFrames = &EngineCore::IncrementFramesFn<xsimd::avx512vnni<xsimd::avx512vbmi2>>;
                UpdatePositions = &EngineCore::UpdateRangeOfPositions<xsimd::avx512vnni<xsimd::avx512vbmi2>>;
                return;

            case SIMDLevel::AVX_VNNI:
                #if (Blokk_Visibility_CullType == 0)
                    CheckVisibleRangeFn = &EngineCore::CheckVisibilityFn_Basic<xsimd::avxvnni>;
                #elif (Blokk_Visibility_CullType == 1)
                    CheckVisibleRangeFn = &EngineCore::CheckVisibilityFn_Axis<xsimd::avxvnni>;  
                #endif
                IncrementFrames = &EngineCore::IncrementFramesFn<xsimd::avxvnni>;
                UpdatePositions = &EngineCore::UpdateRangeOfPositions<xsimd::avxvnni>;
                return;

            // ARM
            case SIMDLevel::NEON:
                #if (Blokk_Visibility_CullType == 0)
                    CheckVisibleRangeFn = &EngineCore::CheckVisibilityFn_Basic<xsimd::neon>;
                #elif (Blokk_Visibility_CullType == 1)
                    CheckVisibleRangeFn = &EngineCore::CheckVisibilityFn_Axis<xsimd::neon>;  
                #endif
                IncrementFrames = &EngineCore::IncrementFramesFn<xsimd::neon>;
                UpdatePositions = &EngineCore::UpdateRangeOfPositions<xsimd::neon>;
                return;
            case SIMDLevel::NEON64:
                #if (Blokk_Visibility_CullType == 0)
                    CheckVisibleRangeFn = &EngineCore::CheckVisibilityFn_Basic<xsimd::neon64>;
                #elif (Blokk_Visibility_CullType == 1)
                    CheckVisibleRangeFn = &EngineCore::CheckVisibilityFn_Axis<xsimd::neon64>;  
                #endif
                IncrementFrames = &EngineCore::IncrementFramesFn<xsimd::neon64>;
                UpdatePositions = &EngineCore::UpdateRangeOfPositions<xsimd::neon64>;
                return;
            case SIMDLevel::I8MM:
                #if (Blokk_Visibility_CullType == 0)
                    CheckVisibleRangeFn = &EngineCore::CheckVisibilityFn_Basic<xsimd::i8mm<xsimd::neon64>>;
                #elif (Blokk_Visibility_CullType == 1)
                    CheckVisibleRangeFn = &EngineCore::CheckVisibilityFn_Axis<xsimd::i8mm<xsimd::neon64>>;  
                #endif
                IncrementFrames = &EngineCore::IncrementFramesFn<xsimd::i8mm<xsimd::neon64>>;
                UpdatePositions = &EngineCore::UpdateRangeOfPositions<xsimd::i8mm<xsimd::neon64>>;
                return;

            // ARM SVE
            case SIMDLevel::SVE128:
                #if (Blokk_Visibility_CullType == 0)
                    CheckVisibleRangeFn = &EngineCore::CheckVisibilityFn_Basic<xsimd::detail::sve<128>>;
                #elif (Blokk_Visibility_CullType == 1)
                    CheckVisibleRangeFn = &EngineCore::CheckVisibilityFn_Axis<xsimd::detail::sve<128>>;  
                #endif
                IncrementFrames = &EngineCore::IncrementFramesFn<xsimd::detail::sve<128>>;
                UpdatePositions = &EngineCore::UpdateRangeOfPositions<xsimd::detail::sve<128>>;
                return;
            case SIMDLevel::SVE256:
                #if (Blokk_Visibility_CullType == 0)
                    CheckVisibleRangeFn = &EngineCore::CheckVisibilityFn_Basic<xsimd::detail::sve<256>>;
                #elif (Blokk_Visibility_CullType == 1)
                    CheckVisibleRangeFn = &EngineCore::CheckVisibilityFn_Axis<xsimd::detail::sve<256>>;  
                #endif
                IncrementFrames = &EngineCore::IncrementFramesFn<xsimd::detail::sve<256>>;
                UpdatePositions = &EngineCore::UpdateRangeOfPositions<xsimd::detail::sve<256>>;
                return;
            case SIMDLevel::SVE512:
                #if (Blokk_Visibility_CullType == 0)
                    CheckVisibleRangeFn = &EngineCore::CheckVisibilityFn_Basic<xsimd::detail::sve<512>>;
                #elif (Blokk_Visibility_CullType == 1)
                    CheckVisibleRangeFn = &EngineCore::CheckVisibilityFn_Axis<xsimd::detail::sve<512>>;  
                #endif
                IncrementFrames = &EngineCore::IncrementFramesFn<xsimd::detail::sve<512>>;
                UpdatePositions = &EngineCore::UpdateRangeOfPositions<xsimd::detail::sve<512>>;
                return;

            // RISC-V
            case SIMDLevel::RVV128:
                #if (Blokk_Visibility_CullType == 0)
                    CheckVisibleRangeFn = &EngineCore::CheckVisibilityFn_Basic<xsimd::detail::rvv<128>>;
                #elif (Blokk_Visibility_CullType == 1)
                    CheckVisibleRangeFn = &EngineCore::CheckVisibilityFn_Axis<xsimd::detail::rvv<128>>;  
                #endif
                IncrementFrames = &EngineCore::IncrementFramesFn<xsimd::detail::rvv<128>>;
                UpdatePositions = &EngineCore::UpdateRangeOfPositions<xsimd::detail::rvv<128>>;
                return;
            case SIMDLevel::RVV256:
                #if (Blokk_Visibility_CullType == 0)
                    CheckVisibleRangeFn = &EngineCore::CheckVisibilityFn_Basic<xsimd::detail::rvv<256>>;
                #elif (Blokk_Visibility_CullType == 1)
                    CheckVisibleRangeFn = &EngineCore::CheckVisibilityFn_Axis<xsimd::detail::rvv<256>>;  
                #endif
                IncrementFrames = &EngineCore::IncrementFramesFn<xsimd::detail::rvv<256>>;
                UpdatePositions = &EngineCore::UpdateRangeOfPositions<xsimd::detail::rvv<256>>;
                return;
            case SIMDLevel::RVV512:
                #if (Blokk_Visibility_CullType == 0)
                    CheckVisibleRangeFn = &EngineCore::CheckVisibilityFn_Basic<xsimd::detail::rvv<512>>;
                #elif (Blokk_Visibility_CullType == 1)
                    CheckVisibleRangeFn = &EngineCore::CheckVisibilityFn_Axis<xsimd::detail::rvv<512>>;  
                #endif
                IncrementFrames = &EngineCore::IncrementFramesFn<xsimd::detail::rvv<512>>;
                UpdatePositions = &EngineCore::UpdateRangeOfPositions<xsimd::detail::rvv<512>>;
                return;

            // POWER
            case SIMDLevel::VSX:
                #if (Blokk_Visibility_CullType == 0)
                    CheckVisibleRangeFn = &EngineCore::CheckVisibilityFn_Basic<xsimd::vsx>;
                #elif (Blokk_Visibility_CullType == 1)
                    CheckVisibleRangeFn = &EngineCore::CheckVisibilityFn_Axis<xsimd::vsx>;  
                #endif
                IncrementFrames = &EngineCore::IncrementFramesFn<xsimd::vsx>;
                UpdatePositions = &EngineCore::UpdateRangeOfPositions<xsimd::vsx>;
                return;

            // IBM Z
            case SIMDLevel::VXE:
                #if (Blokk_Visibility_CullType == 0)
                    CheckVisibleRangeFn = &EngineCore::CheckVisibilityFn_Basic<xsimd::vxe>;
                #elif (Blokk_Visibility_CullType == 1)
                    CheckVisibleRangeFn = &EngineCore::CheckVisibilityFn_Axis<xsimd::vxe>;  
                #endif
                IncrementFrames = &EngineCore::IncrementFramesFn<xsimd::vxe>;
                UpdatePositions = &EngineCore::UpdateRangeOfPositions<xsimd::vxe>;
                return;

            // WebAssembly
            case SIMDLevel::WASM:
                #if (Blokk_Visibility_CullType == 0)
                    CheckVisibleRangeFn = &EngineCore::CheckVisibilityFn_Basic<xsimd::wasm>;
                #elif (Blokk_Visibility_CullType == 1)
                    CheckVisibleRangeFn = &EngineCore::CheckVisibilityFn_Axis<xsimd::wasm>;  
                #endif
                IncrementFrames = &EngineCore::IncrementFramesFn<xsimd::wasm>;
                UpdatePositions = &EngineCore::UpdateRangeOfPositions<xsimd::wasm>;
                return;
                
            default:
                return;
        }
    }
}