#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#define VMA_IMPLEMENTATION
#include <vk_mem_alloc.h>

#include <GLFW/glfw3.h>
#include <VkBootstrap.h>

// GLM 
#include <glm/glm.hpp>

#include <cassert>
#include <filesystem>
#include <iostream>
#include <span>
#include <string>
#include <vector>

#include "Rendering.hpp"
#include "types.hpp"

#include "../EngineClassData.hpp"

namespace Blokk 
{

    void Renderer::DrawFrame()
    {
        // 1. Wait for the previous frame using this slot to finish executing on the GPU
        vkWaitForFences(m_Device, 1, &m_InFlightFences[m_CurrentFrame], VK_TRUE, UINT64_MAX);

        // 2. Acquire the next available image from the swapchain
        uint32_t imageIndex;
        VkResult result = vkAcquireNextImageKHR(
            m_Device,
            m_Swapchain,
            UINT64_MAX,
            m_ImageAvailableSemaphores[m_CurrentFrame], // Signals when the image is ready for rendering
            VK_NULL_HANDLE,
            &imageIndex
        );

        // Handle swapchain recreation if the window was resized or minimized
        if (result == VK_ERROR_OUT_OF_DATE_KHR) 
        {
            // Handle minimization (width or height is 0)
            int width = 0, height = 0;
            while (width == 0 || height == 0) {
                width = m_Window.GetWidth();
                height = m_Window.GetHeight();
                glfwWaitEvents(); // Pause execution until window comes back or unminimizes
            }

            // Wait for GPU to finish any pending operations before destroying the old swapchain
            vkDeviceWaitIdle(m_Device);

            // Clean up existing swapchain image views
            for (auto imageView : m_SwapchainImageViews) {
                vkDestroyImageView(m_Device, imageView, nullptr);
            }
            m_SwapchainImageViews.clear();

            // Replace the old swapchain with a new one
            CreateSwapchain();
            
            return; // Skip the rest of the frame since we need to re-acquire the image
        }

        // Now that we know we are submitting work, reset the fence
        vkResetFences(m_Device, 1, &m_InFlightFences[m_CurrentFrame]);

        // 3. Record command buffers (this calls your compute culling and render pass drawing logic)
        vkResetCommandBuffer(m_CommandBuffer, 0);
    
        // Begin command buffer recording (your helper wrapper)
        BeginCommandBuffer(); 
    
        // Execute your compute culling, barriers, dynamic rendering, and indirect draw
        RunComputeShaders(); 

        if (vkEndCommandBuffer(m_CommandBuffer) != VK_SUCCESS) {
            throw std::runtime_error("Failed to record command buffer!");
        }

        // 4. Submit the command buffer to the graphics queue
        VkSubmitInfo submitInfo{};
        submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

        // Wait until the swapchain image is actually available before writing color output
        VkSemaphore waitSemaphores[] = { m_ImageAvailableSemaphores[m_CurrentFrame] };
        VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
        submitInfo.waitSemaphoreCount = 1;
        submitInfo.pWaitSemaphores = waitSemaphores;
        submitInfo.pWaitDstStageMask = waitStages;

        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = &m_CommandBuffer;

        // Signal this semaphore once rendering is completely done
        VkSemaphore signalSemaphores[] = { m_RenderFinishedSemaphores[m_CurrentFrame] };
        submitInfo.signalSemaphoreCount = 1;
        submitInfo.pSignalSemaphores = signalSemaphores;

        // Submit with the current frame's fence so the CPU knows when this frame finishes
        if (vkQueueSubmit(m_GraphicsQueue, 1, &submitInfo, m_InFlightFences[m_CurrentFrame]) != VK_SUCCESS) {
            throw std::runtime_error("Failed to submit draw command buffer!");
        }

        // 5. Present the rendered image back to the swapchain presentation engine
        VkPresentInfoKHR presentInfo{};
        presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;

        presentInfo.waitSemaphoreCount = 1;
        presentInfo.pWaitSemaphores = signalSemaphores; // Wait until rendering is finished

        VkSwapchainKHR swapchains[] = { m_Swapchain };
        presentInfo.swapchainCount = 1;
        presentInfo.pSwapchains = swapchains;
        presentInfo.pImageIndices = &imageIndex;

        vkQueuePresentKHR(m_PresentQueue, &presentInfo);

        // 6. Advance to the next frame-in-flight slot (e.g., toggling between 0 and 1)
        m_CurrentFrame = (m_CurrentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
    }

    inline void Renderer::DestroyBuffer(AllocatedBuffer buffer) {
        vmaDestroyBuffer(m_Allocator, buffer.buffer, buffer.allocation);
    }

    inline void Renderer::SubmitCommandBuffersAndWait(int32_t cmdBufferCount) {
        VkSubmitInfo submitInfo{
            .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
            .commandBufferCount = cmdBufferCount,
            .pCommandBuffers = &m_CommandBuffer
        };
        vkQueueSubmit(m_GraphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);
        vkDeviceWaitIdle(m_Device); // Block until GPU finishes copying
    }

    [[nodiscard]] inline VkBufferCopy Renderer::CreateBufferCopyRegion(uint32_t srcOffset, uint32_t dstOffset, VkDeviceSize size) {
        return VkBufferCopy{
            .srcOffset = srcOffset,
            .dstOffset = dstOffset,
            .size = size
        };
    }

    void Renderer::RunComputeShaders()
    {
        BeginCommandBuffer();

        // Reset indirect draw command values on the GPU
        // vertexCount = 6 (for sprite quad), instanceCount = 0 (reset every frame so atomic add starts from 0)
        VkDrawIndirectCommand indirectCmd{
            .vertexCount = 6,
            .instanceCount = 0,
            .firstVertex = 0,
            .firstInstance = 0
        };

        vkCmdUpdateBuffer(
            m_CommandBuffer, 
            m_IndirectDrawBuffer.buffer, 
            0,                               // Offset
            sizeof(VkDrawIndirectCommand),   // Size
            &indirectCmd                     // Host data pointer
        );
    
        vkCmdBindPipeline(m_CommandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, m_RenderCullingPipeline);

        vkCmdBindDescriptorSets(
            m_CommandBuffer, 
            VK_PIPELINE_BIND_POINT_COMPUTE, 
            m_PipelineLayout, 
            0, 1, 
            &m_DescriptorSet,
            0, nullptr
        );

        ConstantFrameData frameData{
            .deltaTime = m_Engine.m_DeltaTime.secs,
            .screenDimensions = m_Window.GetDimensions(),
            .cameraPosition = m_Engine.Camera.position,
            .objectCounts = m_GPU_ObjectCount
        };

        vkCmdPushConstants(
            m_CommandBuffer,
            m_PipelineLayout,
            VK_SHADER_STAGE_COMPUTE_BIT,
            0,
            sizeof(ConstantFrameData),
            &frameData
        );

        uint32_t groupCountX = (m_ObjectCount + 255) / 256;
        vkCmdDispatch(m_CommandBuffer, groupCountX, 1, 1);

        // Create an array of barriers for all buffers written by compute that the draw call/vertex shader will read
        VkBufferMemoryBarrier computeToDrawBarriers[] = {
            // 1. Indirect arguments barrier (Must allow INDIRECT_COMMAND_READ)
            {
                .sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER,
                .srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT,
                .dstAccessMask = VK_ACCESS_INDIRECT_COMMAND_READ_BIT,
                .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
                .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
                .buffer = m_IndirectDrawBuffer.buffer,
                .offset = 0,
                .size = VK_WHOLE_SIZE
            },
            // 2. Visible indexes buffer barrier (Read by vertex shader)
            {
                .sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER,
                .srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT,
                .dstAccessMask = VK_ACCESS_SHADER_READ_BIT,
                .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
                .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
                .buffer = m_VisibleIndexesBuffer.buffer,
                .offset = 0,
                .size = VK_WHOLE_SIZE
            },
            // 3. Positions / RenderPositions buffer barrier
            {
                .sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER,
                .srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT,
                .dstAccessMask = VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT,
                .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
                .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
                .buffer = m_PositionsBuffer.buffer,
                .offset = 0,
                .size = VK_WHOLE_SIZE
            }
        };

        vkCmdPipelineBarrier(
            m_CommandBuffer,
            VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
            VK_PIPELINE_STAGE_VERTEX_SHADER_BIT | VK_PIPELINE_STAGE_DRAW_INDIRECT_BIT, // Must include DRAW_INDIRECT stage!
            0,
            0, nullptr,
            3, computeToDrawBarriers, // Pass all 3 barriers
            0, nullptr
        );

        // Draw call using the indirect buffer generated by the compute shader
        vkCmdDrawIndirect(
            m_CommandBuffer, 
            m_IndirectDrawBuffer.buffer, 
            0,                           
            1,                           
            sizeof(VkDrawIndirectCommand)
        );
    }



    inline void Renderer::EndCommandBuffer() const {
        vkEndCommandBuffer(m_CommandBuffer);
    }

    inline void Renderer::BeginCommandBuffer() const {
        if (vkBeginCommandBuffer(m_CommandBuffer, &m_CommandBufferBeginInfo) != VK_SUCCESS) {
            throw std::runtime_error("Failed to begin command buffer recording!");
        }
    }

    

    void Renderer::LoadTextures(const std::vector<std::filesystem::path>& filePaths)
    {
        BeginCommandBuffer();

        struct StagingCopy {
            VkBuffer buffer;
            VmaAllocation allocation;
        };
        std::vector<StagingCopy> stagingUploads;
        stagingUploads.reserve(filePaths.size());

        for (size_t i = 0; i < filePaths.size(); ++i)
        {
            int width = 0, height = 0, channels = 0;
            unsigned char* pixelData = stbi_load(filePaths[i].string().c_str(), &width, &height, &channels, STBI_rgb_alpha);
            if (pixelData == nullptr) {
                throw std::runtime_error(std::format("Failed to load texture at path: {}", filePaths[i].string()));
            }

            VkDeviceSize imageSize = static_cast<VkDeviceSize>(width * height * 4);
        
            // Staging buffer
            VkBufferCreateInfo stagingBufferInfo{
                .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
                .size = imageSize,
                .usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                .sharingMode = VK_SHARING_MODE_EXCLUSIVE
            };

            VmaAllocationCreateInfo stagingAllocInfo{
                .flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT,
                .usage = VMA_MEMORY_USAGE_AUTO
            };

            VkBuffer stagingBuffer;
            VmaAllocation stagingAllocation;
            vmaCreateBuffer(m_Allocator, &stagingBufferInfo, &stagingAllocInfo, &stagingBuffer, &stagingAllocation, nullptr);
            stagingUploads.push_back({stagingBuffer, stagingAllocation});

            void* mappedData = nullptr;
            vmaMapMemory(m_Allocator, stagingAllocation, &mappedData);
            std::memcpy(mappedData, pixelData, static_cast<size_t>(imageSize));
            vmaUnmapMemory(m_Allocator, stagingAllocation);

            stbi_image_free(pixelData);

            // GPU image
            VkImageCreateInfo imageInfo{
                .sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
                .imageType = VK_IMAGE_TYPE_2D,
                .format = VK_FORMAT_R8G8B8A8_UNORM,
                .extent = { static_cast<uint32_t>(width), static_cast<uint32_t>(height), 1 },
                .mipLevels = 1,
                .arrayLayers = 1,
                .samples = VK_SAMPLE_COUNT_1_BIT,
                .tiling = VK_IMAGE_TILING_OPTIMAL,
                .usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
                .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
                .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED
            };

            VmaAllocationCreateInfo imageAllocInfo{
                .usage = VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE
            };

            VkImage textureImage;
            VmaAllocation textureAllocation;
            if (vmaCreateImage(m_Allocator, &imageInfo, &imageAllocInfo, &textureImage, &textureAllocation, nullptr) != VK_SUCCESS) {
                throw std::runtime_error("Failed to allocate GPU texture image via VMA!");
            }

            // Record barriers and copy into shared command buffer
            VkImageMemoryBarrier barrierToTransfer{
                .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
                .srcAccessMask = 0,
                .dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT,
                .oldLayout = VK_IMAGE_LAYOUT_UNDEFINED,
                .newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
                .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
                .image = textureImage,
                .subresourceRange = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 }
            };

            vkCmdPipelineBarrier(
                m_CommandBuffer,
                VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
                VK_PIPELINE_STAGE_TRANSFER_BIT,
                0, 0, nullptr, 0, nullptr, 1, &barrierToTransfer
            );

            VkBufferImageCopy region{
                .bufferOffset = 0,
                .bufferRowLength = 0,
                .bufferImageHeight = 0,
                .imageSubresource = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1 },
                .imageOffset = {0, 0, 0},
                .imageExtent = { static_cast<uint32_t>(width), static_cast<uint32_t>(height), 1 }
            };

            vkCmdCopyBufferToImage(
                m_CommandBuffer,
                stagingBuffer,
                textureImage,
                VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                1,
                &region
            );

            VkImageMemoryBarrier barrierToShaderRead{
                .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
                .srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT,
                .dstAccessMask = VK_ACCESS_SHADER_READ_BIT,
                .oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                .newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
                .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
                .image = textureImage,
                .subresourceRange = barrierToTransfer.subresourceRange
            };

            vkCmdPipelineBarrier(
                m_CommandBuffer,
                VK_PIPELINE_STAGE_TRANSFER_BIT,
                VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
                0, 0, nullptr, 0, nullptr, 1, &barrierToShaderRead
            );

            // Image view
            VkImageViewCreateInfo viewInfo{
                .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
                .image = textureImage,
                .viewType = VK_IMAGE_VIEW_TYPE_2D,
                .format = VK_FORMAT_R8G8B8A8_UNORM,
                .subresourceRange = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 }
            };

            VkImageView imageView;
            if (vkCreateImageView(m_Device, &viewInfo, nullptr, &imageView) != VK_SUCCESS) {
                throw std::runtime_error("Failed to create texture image view!");
            }

            // Bindless descriptor update
            VkDescriptorImageInfo imageInfoDescriptor{
                .sampler = m_GlobalTextureSampler,
                .imageView = imageView,
                .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
            };

            VkWriteDescriptorSet descriptorWrite{
                .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
                .dstSet = m_GlobalBindlessDescriptorSet,
                .dstBinding = 0,
                .dstArrayElement = static_cast<uint32_t>(i),
                .descriptorCount = 1,
                .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                .pImageInfo = &imageInfoDescriptor
            };

            vkUpdateDescriptorSets(m_Device, 1, &descriptorWrite, 0, nullptr);
        
            // Track handles in your cleanup members here if needed
        }

        // Submit everything at once
        EndCommandBuffer();
        SubmitCommandBuffersAndWait(1);

        // Destroy all staging buffers now that GPU is done
        for (const auto& upload : stagingUploads) {
            vmaDestroyBuffer(m_Allocator, upload.buffer, upload.allocation);
        }
    }

    // Load multiple textures
    void Renderer::LoadAnimation(const std::vector<std::filesystem::path>& paths, const std::string& name) 
{
    uint32_t startIndex = static_cast<uint32_t>(m_Frames.size());

    // Batch load all frame textures at once
    LoadTextures(paths);

    // Track the frame count and animation mapping
    m_AnimationStarts.push_back(startIndex);
    m_AnimationNames[name] = m_AnimationCount;
    m_AnimationCount++;
}
    

    // Get an animation's size (number of frames)
    [[nodiscard]] inline uint32_t Renderer::GetAnimationSize(uint32_t idx) const 
    {
        // Throw error (if debug mode) if loading failed
        AssertWhenDebug(idx < AnimationStarts.size(),
            std::format("Animation does not exist at index {} in Renderer::getAnimationSize", idx));

        if (idx == AnimationStarts.size() - 1) {
            return Frames.size() - AnimationStarts[idx];
        } else {
            return AnimationStarts[idx + 1] - AnimationStarts[idx];
        }
    }
    [[nodiscard]] inline uint32_t Renderer::GetAnimationSize(std::string name) const {
        GetAnimationSize(GetAnimationIndex(name));
    }
    

    // Returns the index of the animation in the renderer's data
    template <bool IsVerified = false>
    [[nodiscard]] inline Renderer::GetAnimationIndex(const std::string& name) 
    {
        // Don't bother to check if garunteed to exist
        if constexpr (IsVerified) {
            return AnimationNames[name];
        } 
        else {
            // Find animation
            auto It = AnimationNames.find(name);

            // Throw error (if debug mode) if animation doesn't exist
            AssertWhenDebug(It != AnimationNames.end(),
                std::format("Animation '{}' doesn't exist in Renderer::GetFramesForAnimation()", name));

            // Get index using name from data
            return It->second;
        }
    }

    void Renderer::SetWindow(Window& Window) {
        window = Window;
    }

} // namespace Blokk
