#include "EngineClassData.hpp"
#include "Unordered_Queue.hpp"

#include <algorithm>
#include <chrono>
#include <stdexcept>
#include <vector>

#include "Compute_Shaders/Rendering.hpp"

namespace Blokk
{

#ifdef Blokk_Threading_Manual

    void EngineCore::SetThreadCount(uint32_t Count) {
        if (Count < 1 || Count > ThreadCount) {
            throw std::out_of_range(
                "BlokkForge: Invalid count passed in EngineCore::SetThreadCount."
            );
        }

        if (Count < OpenedThreads) {
            while (OpenedThreads != Count) {
                DestroyThread();
            }
        }
        else if (Count > OpenedThreads) {
            while (OpenedThreads != Count) {
                OpenThread();
            }
        }
    }

    void EngineCore::OpenThread()
    {
        if (OpenedThreads == ThreadCount)
        {
            throw std::out_of_range(
                "Blokk error: Cannot create another thread because the maximum thread count has been reached."
            );
        }

        Workers.emplace_back(std::make_unique<Worker>());
        Workers.back()->Awake();

        OpenedThreads++;
    }

    void EngineCore::DestroyThread()
    {
        if (OpenedThreads == 1)
        {
            throw std::out_of_range(
                "Blokk error: Cannot destroy another thread because the minimum thread count has been reached (1)."
            );
        }

        Workers.back()->Stop();
        Workers.pop_back();

        OpenedThreads--;
    }

#endif

double EngineCore::TimeCPUEngineProcesses()
{
    // Get the total start time
    auto TotalStartTime = std::chrono::steady_clock::now();

    // EngineProcesses-----------------------------------------
    
    { // Velocities

        #ifdef Blokk_Diagnostics
            auto StartTime = std::chrono::steady_clock::now();
        #endif

        // Get ranges
        std::vector<IndexRange> VelRanges =
            GetRanges(DynamicObjectCount, m_UsedThreads);

        // Set function
        Worker::CurrentJob = UpdateRangeOfPositions;

        // Loop through
        uint32_t Count = VelRanges.size();
        for (uint32_t i = 0; i < Count; i++)
        {
            Workers[i]->SetRange(VelRanges[i]);
        }

        { // Animation Frame Incrementing 
            // (Done while velocities are being done by other threads)

            #ifdef Blokk_Diagnostics
                auto StartTime = std::chrono::steady_clock::now();
            #endif

            // Let the main thread increment
            (this->*IncrementFrames)(m_FrameNumbers, CPUObjectCount.Total);

            #ifdef Blokk_Diagnostics
                auto EndTime = std::chrono::steady_clock::now();

                AnimationIncrementTime =
                    std::chrono::duration<double, std::milli>(
                        EndTime - StartTime
                    ).count();
            #endif
        }

        // Wait for each to finish
        for (uint32_t i = 0; i < Count; i++)
        {
            Workers[i]->WaitUntilFinished();
        }

        #ifdef Blokk_Diagnostics
            auto EndTime = std::chrono::steady_clock::now();

            VelocityTime =
                std::chrono::duration<double, std::milli>(
                    EndTime - StartTime
                ).count();
        #endif
    }

    { // Animations

        { // Render

            #ifdef Blokk_Diagnostics
                auto StartTime = std::chrono::steady_clock::now();
            #endif

            // Get ranges
            std::vector<IndexRange> VisRanges =
                GetRanges(ObjectCount, m_UsedThreads);

            // Set function
            Worker::CurrentJob = CheckVisibleRange;

            // Loop
            uint32_t Count = VisRanges.size();
            for (uint32_t i = 0; i < Count; i++)
            {
                Workers[i]->SetRange(VisRanges[i]);
            }

            // Wait for each to finish
            for (uint32_t i = 0; i < Count; i++)
            {
                Workers[i]->WaitUntilFinished();
            }

            // Clear idxs
            RenderObjectIdxs.clear();

            // Loop through workers
            for (uint32_t i = 0; i < Count; i++)
            {
                RenderObjectIdxs.insert(
                    RenderObjectIdxs.end(),
                    Workers[i]->IdxResult.begin(),
                    Workers[i]->IdxResult.end()
                );
            }

            #ifdef Blokk_Diagnostics
                auto EndTime = std::chrono::steady_clock::now();

                VisibilityCullingTime =
                    std::chrono::duration<double, std::milli>(
                        EndTime - StartTime
                    ).count();
            #endif
        }
    }

    // Update objects
    UpdateObjects();

    // Get the end time
    auto TotalEndTime = std::chrono::steady_clock::now();

    // Get the total time
    auto TotalTime =
        std::chrono::duration<double, std::milli>(
            TotalEndTime - TotalStartTime
        ).count();

    return TotalTime;
    
}

double EngineCore::TimeGPUEngineProcesses()
{
    if (m_Renderer != nullptr) {
        m_Renderer->RenderObjects();
    }

    // Nothing happened
    return 0.0f;
}

// Executes all the engine processes, returning the time it took to execute all of them
double EngineCore::TimeEngineProcesses()
{
    double totalTime = 0;
    if (CPUObjectCount.Total != 0) {
        totalTime += TimeCPUEngineProcesses();
    }

    if (GPUObjectCount.Total != 0) {
        totalTime += TimeGPUEngineProcesses();
    }

    return totalTime;
}

#ifdef Blokk_Threading_AdaptiveTiming

void EngineCore::DeltaTiming() {
    // Delta timing
    PreviousExecutionStartTime = CurrentExecutionStartTime;
    CurrentExecutionStartTime = SDL_GetPerformanceCounter();

    // Calculate raw hardware ticks passed during the frame
    Uint64 rawTicks = CurrentExecutionStartTime - PreviousExecutionStartTime;

    // Convert ticks to fractional seconds (double) using SDL's frequency
    double seconds = (double)rawTicks / (double)SDL_GetPerformanceFrequency();

    // Store the seconds in struct (std::chrono handles the conversions automatically)
    DeltaTime.secs = std::chrono::duration<double>(seconds);
    DeltaTime.milli = std::chrono::duration_cast<std::chrono::duration<double, std::milli>>(DeltaTime.secs);
    DeltaTime.micro = std::chrono::duration_cast<std::chrono::duration<double, std::micro>>(DeltaTime.secs);
    DeltaTime.nano = std::chrono::duration_cast<std::chrono::duration<double, std::nano>>(DeltaTime.secs);
}

// Main processing function for the user to call
// Handles thread adpating
void EngineCore::EngineProcess()
{
    // Calculate the delta
    DeltaTiming();

    FrameExecutionTime = TimeEngineProcesses();

    // If not enabled, don't try to adapt!
    if (!AdaptiveThreadingEnabled) {
        return;
    }

    // If a thread was opened previous frame
    if (ThreadOpenedPrevFrame)
    {
        // If took longer than previous frame
        if (FrameExecutionTime > PrevFrameTime)
        {
            // Stop using a thread if can
            if(UsedThreads != 1) {
                UsedThreads--;
            }
                
            // Update vars
            ThreadOpenedPrevFrame = false;
        }

        // If took too long
        else if (FrameExecutionTime > TargetExecutionTime)
        {
            // Use another thread if can
            if(UsedThreads != OpenedThreads) 
            {
                UsedThreads++;

                // Update var
                ThreadOpenedPrevFrame = true;
            } 
                
            // Do nothing if else
        }

        // If took a good time
        else 
        {
            ThreadOpenedPrevFrame = false;
        }
    }
    else // If a thread wasn't opened the previous frame
    {
        // If it took too long
        if (FrameExecutionTime > TargetExecutionTime)
        {
            // Use another thread if can
            if(UsedThreads != OpenedThreads) 
            {
                UsedThreads++;

                // Update var
                ThreadOpenedPrevFrame = true;
            }
        }
    }

    PrevFrameTime = FrameExecutionTime;
}


#elif defined(Blokk_Threading_Manual)

// Main function
void EngineCore::EngineProcess()
{
    TimeEngineProcesses();
}

void EngineCore::SetThreadCount(uint32_t Count)
{
    // Safety
    if(Count == 0)
    {
        throw std::invalid_argument(
            "Blokk error: Thread count must be at least 1."
        );
    }
    else if (Count > ThreadCount) // ThreadCount represents max capacity/hardware limits
    {
        throw std::invalid_argument(
            "Blokk error: Requested thread count exceeds hardware thread count."
        );
    }

    // Slide active boundaries smoothly instead of allocating/deallocating OS memory
    UsedThreads = Count;
    OpenedThreads = Count; 
}

#endif


void EngineCore::UpdateObjects()
{
    // Set variables --------------------------

    ThreadRanges.clear();
    FullRanges.clear();

    ProcessingWorkerCount.store(0); // Workers increment variable by themselves, so reset
    RangesCompleteCount.store(0); // No ranges have been completed

    // Reset 
    AllWorkComplete.store(false);
    WorkCompleteBeingChecked.store(false);

    // Get ranges -------------------------------------------------

    // Get the ranges each thread needs to complete
    FullRanges = GetRanges(ObjectCount, UsedThreads);
    
    // Split the ranges into smaller ones
    ThreadRanges.resize(ThreadCount);
    
    // Nothing to do
    if (ObjectCount == 0) return;
    
    // Number of ranges per thread
    uint32_t RangeSize = FullRanges[0].GetSize();
    
    // Split each full range into smaller ranges and assign them
    for (uint32_t i = 0, count = FullRanges.size(); i < count; i++)
    {
        // Get ranges
        auto& Values = GetRanges(FullRanges[i].GetSize(), RangeSize);

        // Reserve
        ThreadRanges[i].reserve(
            static_cast<uint32_t>(Values.size())
        );
    
        // Add to thread's queue
        for (const auto& val : Values) {
            ThreadRanges[i].push(val);
        }
    }

    // ------------------------------------------------------------

    // Set function
    Worker::CurrentJob = UpdateRangeOfObjects;

    // Loop through ranges
    for(uint32_t i = 0, Count = FullRanges.size(); i < Count; i++)
    {
        // Assign a range (just to make the worker start working, as it
        // doesn't use the given range for updating objects)
        Workers[i]->SetRange({0, 0});
    }
}

// Each thread runs this function
void EngineCore::UpdateRangeOfObjects(Worker& worker)
{
    uint32_t idx = worker.getID();

	auto& ranges = ThreadRanges[idx];
    uint32_t remainingInQueue = ranges.size();

    // Loop over the queue until no ranges remain
    while (true)
    {
        IndexRange range;

        // Try to get range
        if (!ranges.try_pop_front(range))
            break; // Stop if fails

        // Process if succeeeds
        ProcessRange(range);
    }

    // Work for itself complete
    RangesCompleteCount.fetch_add(1);

    // Stealing 
    StealUpdateObjects(idx);
}

// Steals objects from other threads to execute
void EngineCore::StealUpdateObjects(uint32_t idx) 
{
    // Keep attempting to steal until complete
    while (!AllWorkComplete.load())
    {
        // Loop through every worker
        for (uint32_t i = 0; i < UsedThreads; i++)
        {
            if (i == idx)
                continue; // Don't steal from yourself

            IndexRange range;

            // Try to get a value
            if (ThreadRanges[i].try_pop_back(range)) {
                ProcessRange(range); // Process if succeeds
            }
        }

        // Make sure to update variables
        UpdateCompletionVariables();
    }
}


// Checks if every worker has finished processing and stores the result, thread-safe
void EngineCore::UpdateCompletionVariables() 
{
    // If the work isn't being checked
    bool expected = false;
    if (WorkCompleteBeingChecked.compare_exchange_strong(expected, true))
    {
        // Store the result
        bool allWorkersFinished = 
            ProcessingWorkerCount == 0 // No workers are processing
            && RangesCompleteCount == UsedThreads; // All ranges are processed

        AllWorkComplete.store(allWorkersFinished);
        WorkCompleteBeingChecked.store(false); // Reset
    }
}

// Processes a specific range of object update functions
void EngineCore::ProcessRange(IndexRange range)
{
    // Started working
    ProcessingWorkerCount.fetch_add(1);

	uint32_t remainingInRange = range.GetSize();
    uint32_t currentObjIdx = range.Start;

    double delta = DeltaTime.secs;

    // Keep processing until no more objects are left in the range
    while (remainingInRange != 0)
    {
        // Process the object
        ObjectInstances[currentObjIdx]->_Update(delta);

        remainingInRange--;
        currentObjIdx++;
    }

    // Stopped working
    ProcessingWorkerCount.fetch_add(-1);
}

void EngineCore::TimeUpdateRangeOfObjects(Worker& worker)
{
    IndexRange Range = worker.getRange();

    double delta = DeltaTime.secs;

	for (uint32_t i = Range.Start, count = Range.End; i < count; i++)
	{
		// Start time
		auto start = std::chrono::steady_clock::now();

		// Run function
		ObjectInstances[i]->_Update(delta);

		// End time
		auto end = std::chrono::steady_clock::now();

		// Store time
        auto totalTime = std::chrono::duration_cast<nanoseconds>(end - start).count();
        TotalUpdateTimes[i] += totalTime;
        TotalUpdateTime += totalTime;
	}
}

// Gets all the average update times of gameObject Update() functions
void EngineCore::GetAverageUpdateTimes() 
{
    uint32_t count = AverageUpdateTimes.size();
    for (uint32_t i = 0; i < count; i++) {
        AverageUpdateTimes[i] = TotalUpdateTimes[i] / NumberOfTrials;
    }
}

// Resets update times
void EngineCore::ResetUpdateTimes() {
	std::fill(TotalUpdateTimes.begin(), TotalUpdateTimes.end(), 0);
    std::fill(AverageUpdateTimes.begin(), AverageUpdateTimes.end(), 0);
}


// TODO: Make loading function smoother, currently can stutter
// Loads the engine. It adapts to the load, allowing other
void EngineCore::LoadEngine(uint32_t Threading_NumberOfTests, Renderer& p_Renderer)
{
    m_NumberOfTrials++;
    nanoseconds totalTime{ 0 };

// Load all the animations ------------------------

    if (m_CurrentLoadingStage == LoadingStage::LoadingAnimations) 
    {
        // Loop through animations
	    for (uint32_t i = m_CurrentProcessingIdx, count = m_AnimationPaths.size(); i < count; i++)
	    {
            // Update processing index
            m_CurrentProcessingIdx = i + 1;

            // Start time
            auto start = std::chrono::steady_clock::now();

            // Load animation
            p_Renderer.LoadTextures(m_AnimationPaths[i], m_AnimationNames[i]);

            // End time
            auto end = std::chrono::steady_clock::now();

            // Total time
            auto time = std::chrono::duration_cast<nanoseconds>(end - start).count();
            totalTime += time;

            // If taken up the time for the frame
            if (totalTime >= m_FrameTime) {
                return; // Leave
            }
	    }

        // Change loading stage
        m_CurrentLoadingStage = LoadingStage::TimingUpdateFunctions;
    }
    
    // Time all the gameObjects update functions ------
    if (m_CurrentLoadingStage == LoadingStage::TimingUpdateFunctions) 
    {
        ResetUpdateTimes(); // Set all to 0
        if(CPUObjectCount.Total == 0) {
            m_CurrentLoadingStage = LoadingStage::AdaptiveThreading;
            return;
        }
    
	    Worker::CurrentJob = TimeUpdateRangeOfObjects;
	    std::vector<IndexRange> Ranges = GetRanges(ObjectCount, m_UsedThreads);

        // Set each worker's assigned ranges
        uint32_t count = Ranges.size();
	    for (uint32_t i = 0; i < count; i++) {
		    Workers[i]->SetRange(Ranges[i]);
	    }

        // Wait for every worker to complete their work
        for (uint32_t i = 0; i < count; i++) {
            Workers[i]->WaitUntilFinished();
        }

        // Calculate the average times
        GetAverageUpdateTimes();

        // Change stage
        m_CurrentLoadingStage = LoadingStage::FilterUpdateFunctions;

        // Return if time exceeded
        if (totalTime > m_FrameTime.nano) {
            return;
        }
    }

    if (m_CurrentLoadingStage == LoadingStage::FilterUpdateFunctions)
    {
        // If no objects need to be updated
        if (CPUObjectCount.Total == 0) {
            break; // TODO: I don't think break can go here, add proper thing
        }

        if (m_TotalUpdateTime > m_FrameTime.nano) 
        {
            // Get the amount of time needed to cut off
            auto timeExceeded = m_FrameTime.nano - m_TotalUpdateTime;
            auto timeExceededRemaining = timeExceeded;


            // Remove the unneeded functions
            bool complete = false;
            while (!complete)
            {

                // Lowest and highest priority function levels
                uint8_t LowestPriorityLevel = UINT8_MAX;
                uint32_t HighestPriorityLevel = 0;
                for (auto& level : m_UpdatePriorities) {
                    if (level < LowestPriorityLevel)
                        LowestPriorityLevel = level; // lowest priority
                    if (level > HighestPriorityLevel)
                        HighestPriorityLevel = level; // highest priority 
                }

                // Do not discard functions of the highest priority level
                if (LowestPriorityLevel == HighestPriorityLevel) {
                    m_CurrentLoadingStage = LoadingStage::AdaptiveThreading;
                    complete = true;
                    break;
                }

                // Get the functions that are the lowest priority level
                uint32_t count = m_UpdatePriorities.size();
                std::vector<uint32_t> LowestPriorityFunctions;
                for (uint32_t i = 0; i < count; i++) {
                    if (m_UpdatePriorities[i] == LowestPriorityLevel)
                        LowestPriorityFunctions.push_back(i);
                }

                for (auto& idx : LowestPriorityFunctions) 
                {
                    // Update the time remainging to be exceeded
                    timeExceededRemaining -= m_AverageUpdateTimes[idx];
                    Remove_Function(idx); // Remove the function

                    // If the time exceeded has been fixed by removing the functions
                    if (timeExceededRemaining.count() <= 0) 
                    {
                        // Leave the loop
                        m_CurrentLoadingStage = LoadingStage::AdaptiveThreading;
                        complete = true; 
                        break;
                    }
                }
            }
        }
    }
    
    // Allow user to update their loading screen
    return;

    // Adapt threading --------------------------------

// If adaptive threading is enabled, adapt the number of threads used based on the time it took to load the engine
#ifdef Blokk_Threading_AdaptiveTiming
    if (CurrentLoadingStage == LoadingStage::AdaptiveThreading) 
    {
        for (uint32_t i = 0; i < Threading_NumberOfTests; i++) {
            EngineProcess(); // Allow the engine to process to adapt
        }

        // Completed!
        CurrentLoadingStage = LoadingStage::Complete;
        return;
    }
#else 
    // If adaptive threading is not enabled, just finish the loading process
    if(m_CurrentLoadingStage == LoadingStage::AdaptiveThreading) {
        m_CurrentLoadingStage = LoadingStage::Complete;
        return;
    }

#endif // Blokk_Threading_AdaptiveTiming

}


inline void EngineCore::Remove_Function(uint32_t idx) {
    UpdateFunctionsSet[idx] = false;
    UpdatePriorities[idx] = UINT8_MAX;
    AverageUpdateTimes[idx] = nanoseconds_zero;
}


// #endif // ifdef Blokk_GameObjects_UpdateFunctions

} // namespace Blokk