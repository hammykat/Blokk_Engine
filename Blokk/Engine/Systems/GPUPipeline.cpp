#include "EngineClassData.hpp"
#include "Rendering/Rendering.hpp"

#include <cmath>
#include <functional>
#include <span>

namespace Blokk {

	void EngineCore::InitializeGPUPipeline(GPUPipelineCreationData& Data) {
		m_Renderer->CreateAndMapAllBuffers(Data);
		m_GPUPipelineInitialized = true;
	}

	void EngineCore::BulkCreateGPUObjects(BulkObjectCreationData& Data) {
		m_Renderer->BulkAddDataToGPUBuffers(Data);
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

        // Add a barrier to prevent memory bugs
        CmdTransferToComputeBarrier();

        EndCommandBuffer(); 
        SubmitCommandBuffersAndWait(1); // Submit the command buffer and wait for completion

        m_GPU_ObjectCount += Data.ObjectCount; // Total
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
            Indexes.begin(), 
            Indexes.end(), 
            StartingSwapIndex, 
            std::greater<uint32_t>()
        );

        std::span<uint32_t> firstGroup(Indexes.begin(), splitIndex);
        std::span<uint32_t> secondGroup(splitIndex, Indexes.end());

        // First pass -----------------------------------------

        // Send data to staging buffer
        SendDataToCPUBuffer<uint32_t>(m_DeletionStagingBuffer, firstGroup);
        BeginCommandBuffer();

        // Send the data to the GPU buffer to be used
        auto region = CreateBufferCopyRegion(0, 0, firstGroup.size() * sizeof(uint32_t));
        auto countRegion = CreateBufferCopyRegion(0, 0, sizeof(uint32_t));
        vkCmdCopyBuffer(m_CommandBuffer, m_DeletionStagingBuffer, m_IndexesToDeleteBuffer, 1, &region);
        vkCmdCopyBuffer(m_CommandBuffer, m_DeletionStagingBuffer, m_IndexesToDeleteBuffer, 1, &region);
        CmdTransferToComputeBarrier(); // Add a barrier to prevent memory bugs

        vkCmdBindPipeline(m_CommandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, m_ObjectDeletionPipeline);
        vkCmdBindDescriptorSets(m_CommandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, m_PipelineLayout, 0, 1, &m_DescriptorSet, 0, nullptr);

        // Get the required number of groups according to the workgroup size (hardcoded to 256)
        uint32_t NumberOfGroups = std::ceil(static_cast<float>(firstGroup.size()) / 256.0f);
        vkCmdDispatch(m_CommandBuffer, NumberOfGroups, 1, 1);

        // Wait for the first pass to finish
        EndCommandBuffer();
        SubmitCommandBuffersAndWait(1);

        // Second pass -------------------------------------------
        if(secondGroup.empty()) return;

        // Send data to staging buffer
        SendDataToCPUBuffer<uint32_t>(m_DeletionStagingBuffer, secondGroup);
        BeginCommandBuffer();

        // Send the data to the GPU buffer to be used
        region = CreateBufferCopyRegion(0, 0, secondGroup.size() * sizeof(uint32_t));
        vkCmdCopyBuffer(m_CommandBuffer, m_DeletionStagingBuffer, m_IndexesToDeleteBuffer, 1, &region);
        CmdTransferToComputeBarrier(); // Add a barrier to prevent memory bugs

        vkCmdBindPipeline(m_CommandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, m_ObjectDeletionPipeline);
        vkCmdBindDescriptorSets(m_CommandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, m_PipelineLayout, 0, 1, &m_DescriptorSet, 0, nullptr);

        // Get the required number of groups according to the workgroup size (hardcoded to 256)
        uint32_t NumberOfGroups = std::ceil(static_cast<float>(secondGroup.size()) / 256.0f);
        vkCmdDispatch(m_CommandBuffer, NumberOfGroups, 1, 1);

        // Wait for the second pass to finish
        EndCommandBuffer();
        SubmitCommandBuffersAndWait(1);

        //------------------------------------------------

        // Update the object count
        m_GPU_ObjectCount -= Indexes.size();
    }
}

