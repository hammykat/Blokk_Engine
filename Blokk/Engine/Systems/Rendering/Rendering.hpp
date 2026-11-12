#pragma once

#define VMA_IMPLEMENTATION
#include <vk_mem_alloc.h>

#include <GLFW/glfw3.h>
#include <VkBootstrap.h>
#include <glm/glm.hpp>

#include <cassert>
#include <filesystem>
#include <iostream>
#include <span>
#include <string>
#include <unordered_map>
#include <vector>

#include "../EngineClassData.hpp"
#include "Rendering.hpp"
#include "types.hpp"

namespace Blokk {

	constexpr uint32_t TextureSizeCount = 6;
	enum class TextureSizes {
		s_16, 
		s_32,
		s_64,
		s_128,
		s_256,
		s_512
	};

	struct Animation {
		TextureSizes texture_size;
		int32_t frame_count;
		uint32_t start_index;
	};

	class EngineCore;

	class Renderer {
	private:
		struct ComputeShaderConstantFrameData {
			int objectCounts[3];
			float cameraPosition[2];
			float screenDimensions[2];
			float deltaTime;
		};
		
		struct AllocatedBuffer {
			VkBuffer buffer;
			VmaAllocation allocation;
			void* mappedData = nullptr;
		};

		enum class MemoryUsage {
			CPU_ONLY = VMA_MEMORY_USAGE_CPU_ONLY,
			GPU_ONLY = VMA_MEMORY_USAGE_GPU_ONLY,
			CPU_TO_GPU = VMA_MEMORY_USAGE_CPU_TO_GPU,
			GPU_TO_CPU = VMA_MEMORY_USAGE_GPU_TO_CPU,
			CPU_COPY = VMA_MEMORY_USAGE_CPU_COPY
		};

		enum class UsageFlags {
			TRANSFER_SRC = VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
			TRANSFER_DST = VK_BUFFER_USAGE_TRANSFER_DST_BIT,
			UNIFORM_TEXEL_BUFFER = VK_BUFFER_USAGE_UNIFORM_TEXEL_BUFFER_BIT,
			STORAGE_TEXEL_BUFFER = VK_BUFFER_USAGE_STORAGE_TEXEL_BUFFER_BIT,
			UNIFORM_BUFFER = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
			STORAGE_BUFFER = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
			INDEX_BUFFER = VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
			VERTEX_BUFFER = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
			INDIRECT_BUFFER = VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT
		};

		Window& m_Window;
		EngineCore& m_Engine;

		uint32_t m_GPU_ObjectCount; // Number of objects being updated by the GPU
		uint32_t m_AnimationCount = 0;

		GLuint m_TexturePoolHandles[TextureSizeCount];
		std::vector<Animation> m_AnimationData; 
		std::unordered_map<std::string, uint32_t> m_AnimationNames;
		std::vector<uint32_t> m_AnimationStarts;
		std::vector<GLuint> m_Frames;

		std::vector<VkSemaphore> m_ImageAvailableSemaphores;
		std::vector<VkSemaphore> m_RenderFinishedSemaphores;
		std::vector<VkFence> m_InFlightFences;
		uint32_t m_CurrentFrame = 0;

		// Basics
		VkInstance m_Instance;
		VkDebugUtilsMessengerEXT m_DebugMessager;
		VkSurfaceKHR m_Surface;
		VkPhysicalDevice m_PhysicalDevice;
		VkDevice m_Device;
		vkb::Device m_VkbDevice;

		// Queues
		VkQueue m_GraphicsQueue;
		uint32_t m_GraphicsQueueIndex; 
		VkQueue m_PresentQueue;

		// Commands
		VkCommandPool m_CommandPool;
		VkCommandBuffer m_CommandBuffer;
		VkCommandBufferBeginInfo m_CommandBufferBeginInfo;

		// Swapchain & presentation
		VkSwapchainKHR m_Swapchain;
		VkFormat m_SwapchainImageFormat;
		VkExtent2D swapchain_extent;
		std::vector<VkImage> m_SwapchainImages;
		std::vector<VkImageView> m_SwapchainImageViews;

		VkDescriptorSetLayout m_DescriptorSetLayout;
		VkPipelineLayout m_PipelineLayout;
		VkPipeline m_VelocitiesPipeline;
		VkPipeline m_RenderCullingPipeline;
		VkDescriptorSet m_DescriptorSet;
		
		VmaAllocator m_Allocator;

		// Buffers
		AllocatedBuffer m_VelocitiesBuffer;
		AllocatedBuffer m_PositionsBuffer;
		AllocatedBuffer m_AnimationNumbersBuffer;
		AllocatedBuffer m_FrameNumbersBuffer;
		AllocatedBuffer m_IndirectDrawBuffer;
		AllocatedBuffer m_VisibleIndexesBuffer;

		VkDescriptorSetLayout m_GlobalDescriptorSetLayout;
		VkDescriptorSet m_GlobalBindlessDescriptorSet;
		VkSampler m_GlobalTextureSampler;
		std::vector<VkImageView> m_LoadedImageViews;
		std::vector<VkImage> m_LoadedImages;
		std::vector<VmaAllocation> m_LoadedAllocations;

		void init_vulkan(GLFWwindow* window);
		vkb::PhysicalDevice SelectPhysicalDevice();
		vkb::Device BuildLogicalDevice(vkb::PhysicalDevice vkb_phys_device);
		void CreateVMAMemoryAllocator();

		void CreateGraphicsQueues(vkb::Device vkb_device);
		void CreateSwapchain();

		void CreateComputeShaderLayout();
		VkShaderModule CreateShaderModule(VkDevice device, const std::vector<uint32_t>& code);

		AllocatedBuffer CreateAndMapBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VmaMemoryUsage memoryUsage);
		void CreateAndMapAllBuffers(uint32_t objectCount);
		void SubmitCommandBuffersAndWait(int32_t cmdBufferCount);

		void CreateBindlessDescriptorLayout();

		inline void BeginCommandBuffer();
		inline void EndCommandBuffer();
		[[nodiscard]] inline VkBufferCopy CreateBufferCopyRegion(uint32_t srcOffset, uint32_t dstOffset, VkDeviceSize size);
		template<typename T>
		void SendDataToCPUBuffer(AllocatedBuffer buffer, const std::vector<T>& data) {
			AssertWhenDebug(buffer.mappedData != nullptr, "Buffer is not mapped in Renderer::SendDataToBuffer!");
			std::memcpy(buffer.mappedData, data.data(), data.size() * sizeof(T));
		}
		template<typename T>
		void SendDataToCPUBuffer(AllocatedBuffer buffer, const std::span<T>& data) {
			AssertWhenDebug(buffer.mappedData != nullptr, "Buffer is not mapped in Renderer::SendDataToBuffer!");
			std::memcpy(buffer.mappedData, data.data(), data.size() * sizeof(T));
		}
		template<typename T>
		void SendDataToGPUBuffer(const std::vector<T>& data, AllocatedBuffer buffer, uint32_t offset);
		void BulkAddDataToGPUBuffers(const BulkObjectCreationData& Data);
		void BulkDeleteObjects(std::vector<uint32_t>& Indexes);

		std::vector<uint32_t> LoadShaderFile(const std::string& _filePath);
		void LoadAndStoreShaders();
		void LoadTextures(const std::vector<std::filesystem::path>& filePaths);

		template<bool IsVerified = false> 
		[[nodiscard]] inline uint32_t GetAnimationIndex(const std::string& name) {
			if constexpr (IsVerified) {
				return m_AnimationNames[name];
			} 
			else {
				auto it = m_AnimationNames.find(name);
				return it->second;
			}
		}

		[[nodiscard]] inline uint32_t GetAnimationSize(uint32_t idx) const {
			if (idx == m_AnimationStarts.size() - 1) {
				return static_cast<uint32_t>(m_Frames.size()) - m_AnimationStarts[idx];
			} else {
				return m_AnimationStarts[idx + 1] - m_AnimationStarts[idx];
			}
		}

		[[nodiscard]] inline uint32_t GetAnimationSize(const std::string& name) const {
			// Implementation relies on index lookup
			return 0; 
		}

		void RunComputeShaders();
		inline void DestroyBuffer(AllocatedBuffer buffer);


	public:
		friend class EngineCore;

		Renderer(Window& p_Window, EngineCore& p_Engine);
		~Renderer();

		void DrawFrame();
		void LoadAnimation(const std::vector<std::filesystem::path>& paths, const std::string& name);

		void SetWindow(Window& Window);

		void RenderObjects();
		void SetClearColor();

		void ClearScreen();
		void Present();
	};

}