#include "EngineClassData.hpp"
#include "Rendering/Rendering.hpp"

#include <functional>
#include <span>

namespace Blokk {

	void EngineCore::InitializeGPUPipeline(BulkObjectCreationData& Data) {
		m_Renderer.CreateAndMapAllBuffers(Data.ObjectCount);
		m_GPUPipelineInitialized = true;
	}

	void EngineCore::BulkCreateObjects(BulkObjectCreationData& Data) {
		m_Renderer.BulkAddDataToGPUBuffers(Data);
	} 

    // Renderer functions --------------------------------------------------------

	// CRUCIAL: Does NOT begin the command buffer, needs to be began from where it was called from
    template<typename T>
    void Renderer::SendDataToGPUBuffer(const std::vector<T>& data, AllocatedBuffer buffer, uint32_t offset) 
    {
        if(data.empty()) return;

        // Copy data into a temporary CPU-side buffer
        VkBuffer stagingBuffer = CreateAndMapBuffer(data.size() * sizeof(T), VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VMA_MEMORY_USAGE_CPU_ONLY).buffer;
        auto copyRegion = CreateBufferCopyRegion(0, offset * sizeof(T), data.size() * sizeof(T));
        std::memcpy(stagingBuffer.mappedData, data.data(), data.size() * sizeof(T));

        vkCmdCopyBuffer(m_CommandBuffer, stagingBuffer, buffer.buffer, 1, &copyRegion);
    }

    void Renderer::BulkAddDataToGPUBuffers(const BulkObjectCreationData& Data) 
    {
        BeginCommandBuffer(); 

        // Copy all data into GPU buffers using the helper function
        SendDataToGPUBuffer<glm::vec2>(Data.Velocities, m_VelocitiesBuffer, m_GPU_ObjectCount);
        SendDataToGPUBuffer<glm::vec2>(Data.Positions, m_PositionsBuffer, m_GPU_ObjectCount);
        SendDataToGPUBuffer<uint32_t>(Data.FrameNumbers, m_FrameNumbersBuffer, m_GPU_ObjectCount);
        SendDataToGPUBuffer<uint32_t>(Data.AnimationNumbers, m_AnimationNumbersBuffer, m_GPU_ObjectCount);

        EndCommandBuffer(); 
        SubmitCommandBuffersAndWait(1); // Submit the command buffer and wait for completion

        m_GPU_ObjectCount = Data.ObjectCount; // Total
    }

    void Renderer::BulkDeleteObjects(std::vector<uint32_t>& Indexes) 
    {
        // Sort the list for easier separation
        std::sort(Indexes.begin(), Indexes.end(), std::greater<uint32_t>());

        uint32_t count = Indexes.size();
        if(count == 0) return;

        uint32_t StartingSwapIndex = m_GPU_ObjectCount - Indexes.size();
        // Represents the first index in the 'Indexes' array that is greater or equal to the starting swap index, so will be swapped with another item to get deleted
        uint32_t StartingSwapIndexInArray = 0; 

        auto splitIndex = std::upper_bound(
            sortedIndexes.begin(), 
            sortedIndexes.end(), 
            StartingSwapIndex, 
            std::greater<uint32_t>()
        );

        std::span<uint32_t> firstGroup(Indexes.begin(), splitIndex);
        std::span<uint32_t> secondGroup(splitIndex, Indexes.end());

        BeginCommandBuffer();

        // TODO: Create and map and add an indexes to delete GPU buffer along with it's size
        // TODO: Make suer it's passing the right params here...
        SendDataToGPUBuffer(firstGroup, m_VisibleIndicesBuffer)
    }
}

