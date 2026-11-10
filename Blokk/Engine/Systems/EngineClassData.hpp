#pragma once

#include <SDL3/SDL.h>
#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <glm/glm.hpp>
#include <memory>
#include <string>
#include <vector>

// BFCL
#include <concurrent_unordered_queue.hpp>


// Forward-declare SDL types to avoid pulling the whole SDL header into this
// widely-included header. Include the SDL headers in .cpp files where
// concrete SDL types are required.
struct SDL_Texture;

// Configuration macros are handled in CMake. Check "Cmake/JSONParser.cmake"

#include "types.hpp"
#include "SIMD_Finder.hpp"
#include "GameObject.hpp"


namespace Blokk
{
    // Camera is a basic structure with only position
    // TODO: Add camera rotation (and zoom functionality IF POSSIBLE)
    struct CameraController {
        glm::vec2 position;
    };

    class EngineCore;
    class GameObject;
    class Renderer;
    class Diagnostics;

    using TimeEngineProcessesFnPtr =
        double(EngineCore::*)();

    using nanoseconds = std::chrono::nanoseconds;
    using microseconds = std::chrono::microseconds;
    using milliseconds = std::chrono::milliseconds;
    using seconds = std::chrono::seconds;

    constexpr auto nanoseconds_zero = std::chrono::nanoseconds::zero();
}

#include "Threading.hpp"

namespace Blokk
{
    

    enum Culling
    {
        Axis,
        Basic
    };

    struct ManagerCreation
    {
        vector2<uint32_t> ScreenDimensions;
        uint32_t FPS = 30;
    };

    // Object manager
    class EngineCore
    {
    public:
        friend class GameObject;
        friend class CachedGameObject;
        friend class Renderer;
        friend class Diagnostics;

        EngineCore(ManagerCreation Cr) :

            // Threads
            m_ThreadOpenedPrevFrame(false),
            m_ThreadDestroyedPrevFrame(false),
            m_AdaptiveThreadingEnabled(true),
            m_ThreadCount(std::thread::hardware_concurrency()),
            m_OpenedThreads(m_ThreadCount),
            m_PrevOpenedThreads(0),

            // Frames
            m_FPS(Cr.FPS),
            m_FrameTime {
                std::chrono::duration<double>{1.0 / Cr.FPS},
                std::chrono::duration<double, std::milli>{1.0 / Cr.FPS},
                std::chrono::duration<double, std::micro>{1.0 / Cr.FPS},
                std::chrono::duration<double, std::nano>{1.0 / Cr.FPS}
            },
            m_TargetExecutionTime {
                m_FrameTime.secs * 0.8,
                m_FrameTime.milli * 0.8,
                m_FrameTime.micro * 0.8,
                m_FrameTime.nano * 0.8
            },

            m_FrameExecutionTime(0),
            m_PrevFrameTime(0),

            // Diagnostics timing
            #ifdef Blokk_Diagnostics
            UserUpdateTime(0),
            VelocityTime(0),
            AnimationIncrementTime(0),
            VisibilityCullingTime(0),
            RenderTime(0),
            #endif

            // Camera
            Camera(),

            // SIMD
            m_SIMDRegisterLevel(DetectSIMD()),

            // Screen
            m_ScreenDimensions(static_cast<glm::vec2>(Cr.ScreenDimensions)),

            // Counts
            ObjectCount(0),
            StaticObjectCount(0),
            DynamicObjectCount(0)
        {

            // Throw an error if no threads were found
            if (m_ThreadCount == 0)
            {
                throw std::runtime_error(
                    "Blokk couldn't find the hardware thread count."
                );
            }

            // Set the gameobject's manager
            GameObject::Engine = this;

            // Set the worker's manager
            Worker::Manager = this;

            // Open all threads
            for(uint32_t i = 0; i < m_ThreadCount; i++)
            {
                OpenThread();
                Workers[i]->setID(i);
            }

            // Get the proper functions
            GetFunctions();

            // Get starting delta variable
            m_CurrentExecutionStartTime = SDL_GetPerformanceCounter();
        }

        ~EngineCore()
        {
            // Destroy all objects
            for (uint32_t i = 0; i < ObjectCount; i++) {
                delete m_ObjectInstances[i];
            }

            // Destroy all threads
            for(uint32_t i = 0; i < m_OpenedThreads; i++) {
                DestroyThread();
            }
        }

        void EngineProcess();

        // Camera
        CameraController Camera;

        // Functions the user can call if manual threading enabled
        #ifdef Blokk_Threading_Manual
        void SetThreadCount(uint32_t Count); // Set the number of threads
        void OpenThread(); // Create a thread
        void DestroyThread(); // Destroy a thread
        #endif

        #ifdef Blokk_Threading_AdaptiveTiming

        // Adaptive threading system flags
        void StopAdaptiveThreadingSystem() { // Disable
            AdaptiveThreadingEnabled = false;
        }
        void EnableAdaptiveThreadingSystem() { // Enable
            AdaptiveThreadingEnabled = true;
        }

        // Set the target execution time for engine to do adaptive threading
        void SetTargetExecutionTime(double Time)
        {
            // Make sure time isn't negative
            if (Time <= 0)
            {
                throw std::invalid_argument(
                    "Blokk error: Tried to set frame time to negative value or 0."
                );
            }

            TargetExecutionTime = Time;
        }

        #endif

        // Load a list of objects
        void LoadObjects(const std::vector<ObjectCreationParams>& ObjectData) {
            uint32_t count = ObjectData.size();
            m_ObjectInstances.reserve(count);
            for (uint32_t i = 0; i < count; i++) 
            {
                // Create a new instance. Doesn't need any extra code here because object 
                // manually tells engine to register itself already
                new GameObject(ObjectData[i]); 
            }
        }

    // PRIVATE -------------------------------------------
    private:

        SIMDLevel m_SIMDRegisterLevel;
        glm::vec2 m_ScreenDimensions;

        // Object data --------------------------------------

        std::vector<float> m_XVelocities;
        std::vector<float> m_YVelocities;
        std::vector<float> m_XPositions;
        std::vector<float> m_YPositions;

        // Animations
        std::vector<uint32_t> m_FrameNumbers;
        std::vector<uint32_t> m_AnimationNumbers;
        std::vector<uint8_t> m_IsVisible; // If the object is set to visible (By the user) 

        // Stores the animation names, points to an index in the animations
        std::unordered_map<std::string, uint32_t> m_AnimationNames;

        // Stores a list of animations
		std::vector<std::vector<std::filesystem::path>> m_AnimationPaths; // Contains the paths to the animation frames for each animation

        // Contains the frame dimensions for each animation frame
		std::vector<std::vector<float>> m_FrameWidths; 
        std::vector<std::vector<float>> m_FrameHeights;

        // Contains the number of frames for each animation
        std::vector<uint32_t> m_AnimationSizes; 

        // Contains the dimensions for each animation
        std::vector<float> m_AnimationHeights;
        std::vector<float> m_AnimationWidths;

 
        // Pointers to object instances
        std::vector<GameObject*> m_ObjectInstances;

        // Rendering
        std::vector<uint32_t> RenderObjectIdxs; // Indexes to objects that need to get rendered

        // Frmae timings -----------------------------
        struct Timings
        {
            std::chrono::duration<double> secs{ 0 };
            std::chrono::duration<double, std::milli> milli{ 0 };
            std::chrono::duration<double, std::micro> micro{ 0 };
            std::chrono::duration<double, std::nano> nano = nanoseconds_zero;
        };
        Timings m_FrameTime;
        Timings m_TargetExecutionTime;
        uint32_t m_FPS;
        
        // Delta timing
        Uint64 m_PreviousExecutionStartTime;
        Uint64 m_CurrentExecutionStartTime;
        Timings m_DeltaTime;

        // Work stealing system (for gameObject update functions)

        uint32_t m_NumberOfTrials = 0;
        uint32_t m_CurrentProcessingIdx = 0;
        std::vector<nanoseconds> m_TotalUpdateTimes; // Represents the total update time for each function
        std::vector<nanoseconds> m_AverageUpdateTimes; // Represents the average amount of time each functoin took to execute
        bool m_ImprovedUpdateTime; // Represents if the update time from this frame is less than the previous
        std::vector<uint8_t> m_UpdatePriorities; // Represents the game object update function priority levels
        std::vector<uint8_t> m_UpdateFunctionsSet; // Represents if the update function has been set
        nanoseconds m_TotalUpdateTime; // Total amount of time for all functions to update
        nanoseconds m_AverageUpdateTime; // Average amount of time for all the functions to update
        nanoseconds m_PrevAvgUpdateTime;
        bool m_ImprovedUpdateTimes; // If the update times from this frame were better that previous frames (shorter)

        uint8_t m_LowestUpdatePriorityLevel;
        uint8_t m_HighestUpdatePriorityLevel;
        std::vector<uint32_t> m_LowestUpdatePriorityFunctions;

        std::vector<concurrent_unordered_queue<IndexRange>> ThreadRanges; // Represents every thread's queue of ranges to complete
        std::vector<IndexRange> m_FullRanges; // Represent the full ranges assigned to each thread at the start
        std::atomic<uint32_t> m_ProcessingWorkerCount = 0; // Represents the number of workers currently processing
        std::atomic<uint32_t> m_RangesCompleteCount; // Represents the number of workers that have their ranges complete OR the last range is being currently processed
        std::atomic<bool> m_AllWorkComplete; // If all the ranges have been processed
        std::atomic<bool> m_WorkCompleteBeingChecked; // If a thread is checking whether all the work is complete (To avoid multiple threads checking at once)


		enum class LoadingStage
		{
			LoadingAnimations, // Currently loading all the animations
			TimingUpdateFunctions, // Currently timing all the gameObject update functions
            FilterUpdateFunctions, // Currently getting rid of unneeded update functions if taking too long
			AdaptiveThreading, // Currently adapting the threading system to the current load
			Complete // Finished loading the engine
		};

        // Allow the user to create their own custom loading screen for their game! --------------------------------------
		LoadingStage m_CurrentLoadingStage; // Current loading stage of the engine
		float m_CurrentPercentage; // Current percentage of the current loading stage
		uint32_t m_CurrentLoadingObjectIdx; // Current object index being processed for the current loading stage
        uint32_t m_NumberOfTrials; 

        // Workers
		uint32_t m_ThreadCount; // Total number of threads available on the system
		uint32_t m_OpenedThreads; // Total number of threads currently opened by the engine
		uint32_t m_UsedThreads; // Total number of threads currently being used by the engine
        uint32_t m_PrevOpenedThreads;

        bool m_ThreadOpenedPrevFrame;
        bool m_ThreadDestroyedPrevFrame;
        bool m_AdaptiveThreadingEnabled;

        double m_FrameExecutionTime;
        double m_PrevFrameTime;
        double m_TargetExecutionTime;

        std::vector<std::unique_ptr<Worker>> Workers;
        
        #ifdef Blokk_Diagnostics

        double UserUpdateTime;
        double VelocityTime;
        double AnimationIncrementTime;
        double VisibilityCullingTime;
        double RenderTime;

        #endif
        
        // For updating GPU object data after the start
        Renderer* m_Renderer;

        // GPU object data (only taken at start)
        std::vector<glm::vec2> m_StartingVelocities; // The velocities of each object
        std::vector<glm::vec2> m_StartingPositions; // The positions of each object
        std::vector<uint32_t> m_StartingFrameNumbers; // The frame numbers of each object
        std::vector<uint32_t> StartingAnimationNumbers; // The animation numbers of each object (Stored as indexes to animation array)

        // CPU object data 

        struct Counts {
            uint32_t Total, Static, Dynamic;
        };

        // Counts
        Counts GPUObjectCount; // Objects that are updated by GPU
        Counts CPUObjectCount; // Objects that are updated by the CPU

        // Functions -------------------------------------------------

        void SetRenderer(Renderer& p_Renderer) {
            m_Renderer = &p_Renderer;
        }


        // Split a number into x ranges
        std::vector<IndexRange> GetRanges(uint32_t Length, uint32_t Count)
        {
            if (Count == 0 || Length == 0)
                return {};

            uint32_t NewCount = std::min(Length, Count);

            uint32_t BaseSize = Length / NewCount;
            uint32_t Remainder = Length % NewCount; // Find out how many extra items are leftover

            std::vector<IndexRange> Result;
            Result.reserve(NewCount);

            uint32_t CurrentStart = 0;

            for (uint32_t i = 0; i < NewCount; i++)
            {
                // Give 1 extra item from the remainder pool to the first few threads
                uint32_t CurrentSize = BaseSize + (i < Remainder ? 1 : 0);
                uint32_t CurrentEnd = CurrentStart + CurrentSize;

                Result.push_back(IndexRange{CurrentStart, CurrentEnd});
                
                CurrentStart = CurrentEnd; // Roll forward to the next index slot
            }

            return Result;
        }

        void GetRanges(uint32_t Length, uint32_t Count, std::vector<IndexRange>* Result) {
            *Result = GetRanges(Length, Count);
        }


        

        // Time engine processes -------------------------------------

        // Calculates delta
        void DeltaTiming();

        // Execute engine processes and time it
        double TimeEngineProcesses();
        double TimeGPUEngineProcesses();
        double TimeCPUEngineProcesses();

        // Swap 2 objects
        void SwapStaticObjects(uint32_t Obj1, uint32_t Obj2);
        void SwapDynamicObjects(uint32_t Obj1, uint32_t Obj2);

        // Destroy an object
        void DestroyDynamicObject(uint32_t ObjIdx);
        void DestroyStaticObject(uint32_t ObjIdx);

        // POSITIONS W/ VELS -----------------------------------------

        template<typename Arch>
        void UpdateRangeOfPositionsFn(Worker& worker, IndexRange& Range);

        using UpdateRangeOfPositionsFnPtr = void(EngineCore::*)(Worker& worker, IndexRange& Range);
        UpdateRangeOfPositionsFnPtr UpdatePositions;

        // Visibility checks ------------------------------------------

        #if (Blokk_Visibility_CullType == 0) // Basic
            template <typename Arch>
            void CheckVisibilityFn_Basic(Worker& worker, IndexRange& Range);
        #elif (Blokk_Visibility_CullType == 1) // Axis
            template <typename Arch>
            void CheckVisibilityFn_Axis(Worker& worker, IndexRange& Range);
        #endif

        using CheckVisibleRangeFnPtr = void(EngineCore::*)(Worker& worker, IndexRange& Range);
        CheckVisibleRangeFnPtr CheckVisibleRange;

        // Adding objects ----------------------------------------------------

        void ProcessAddCommand(EngineObjectCreationParams Fields);

        void ProcessAddCommand(bool Visible, vector2<float> Velocity, vector2<float> Position, GameObject* Object);

        // Frame increment --------------------------------------------

        template<typename Arch>
        void IncrementFramesFn(std::vector<uint32_t>& FrameNums, uint32_t Size);
        using IncrementFramesFnPtr = void(EngineCore::*)(std::vector<uint32_t>& FrameNums, uint32_t Size);
        IncrementFramesFnPtr IncrementFrames;

        // GameObject update functions
        #ifdef Blokk_GameObject_UpdateFunctions

        void UpdateObjects();
        void TimeUpdateRangeOfObjects(Worker& worker);
        void ResetUpdateTimes();
        void UpdateTimeVariables();
        void ProcessRange(IndexRange range);
        void UpdateRangeOfObjects(Worker& worker);
        void UpdateCompletionVariables();
        void StealUpdateObjects(uint32_t idx);
        void GetAverageUpdateTimes();
        void Remove_Function(uint32_t idx);
        
        #endif

        void LoadEngine(uint32_t Threading_NumberOfTests);
    };
}