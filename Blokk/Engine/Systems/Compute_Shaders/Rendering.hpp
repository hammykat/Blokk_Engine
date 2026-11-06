#include <cstdint>
#include <filesystem>
#include <glad/glad.h> 
#include <glfw/glfw3.h>
#include <glm/glm.hpp>
#include <iostream>
#include <string> 
#include <unordered_map>
#include <vector>

#include "Window.hpp"

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
		TextureSizes texture_size; // The texture size (16x16, 32x32, etc)
		int32_t frame_count; // The number of frames in the animation
		uint32_t start_index; // The animation's first frame's index in the array
	};

	class EngineCore;

	class Renderer {

	private:
		// Struct for contant frame data. Passed into HLSL compute shaders. Matches layout exactly
		struct ComputeShaderConstantFrameData {
			int objectCounts[3];
			float cameraPosition[2];
			float screenDimensions[2];
			float deltaTime;
		};
		
		// Struct for allocating and writing to buffer (for compute shaders)
		struct AllocatedBuffer {
			VkBuffer buffer;
			VmaAllocation allocation;
			void* mappedData = nullptr; // For persistent mapping
		};

		enum class MemoryUsage {
			CPU_ONLY = VMA_MEMORY_USAGE_CPU_ONLY,
			GPU_ONLY = VMA_MEMORY_USAGE_GPU_ONLY,
			CPU_TO_GPU = VMA_MEMORY_USAGE_CPU_TO_GPU,
			GPU_TO_CPU = VMA_MEMORY_USAGE_GPU_TO_CPU,
			CPU_COPY = VMA_MEMORY_USAGE_CPU_COPY
		};

		enum class UsageFlags {
			TRANSFER_SRC = VK_BUFFER_USAGE_TRANSFER_SRC_BIT, // Source of a copy operation
			TRANSFER_DST = VK_BUFFER_USAGE_TRANSFER_DST_BIT, // Destination of a copy operation
			UNIFORM_TEXEL_BUFFER = VK_BUFFER_USAGE_UNIFORM_TEXEL_BUFFER_BIT, // For reading texel data in shaders
			STORAGE_TEXEL_BUFFER = VK_BUFFER_USAGE_STORAGE_TEXEL_BUFFER_BIT, // For reading/writing texel data in shaders
			UNIFORM_BUFFER = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, // For reading uniform data in shaders
			STORAGE_BUFFER = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, // For reading/writing storage data in shaders
			INDEX_BUFFER = VK_BUFFER_USAGE_INDEX_BUFFER_BIT, // For reading index data in shaders
			VERTEX_BUFFER = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, // For reading vertex data in shaders
			INDIRECT_BUFFER = VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT // For reading indirect draw data in shaders
		};

		Window& m_Window;
		EngineCore& m_Engine;

		// The number of objects to render (used for compute shader dispatch)
		// First index is total, second is dynamic, vthird is static
		uint32_t m_GPU_UpdatingObjectCounts[3]; 

		GLuint m_TexturePoolHandles[TextureSizeCount]; // Stores the first item in each pool
		std::vector<Animation> m_AnimationData; 
		std::unordered_map<std::string, uint32_t> m_AnimationNames; // Stores it's index to the animation starts

		// Synchronization & Presentation
		std::vector<VkSemaphore> m_ImageAvailableSemaphores;
		std::vector<VkSemaphore> m_RenderFinishedSemaphores;
		std::vector<VkFence> m_InFlightFences;
		uint32_t m_CurrentFrame = 0;

		// Core Bootstrapping & Device State
		VkInstance m_Instance;
		VkDebugUtilsMessengerEXT m_DebugMessager; // Handled automatically by vkb if using default
		VkSurfaceKHR m_Surface;
		VkPhysicalDevice m_PhysicalDevice;
		VkDevice m_Device;
		vkb::Device m_VkbDevice; // Logical device wrapper from vk-bootstrap

		// Queues
		VkQueue m_GraphicsQueue;
		uint32_t m_GraphicsQueueIndex; 
		VkQueue m_PresentQueue;

		VkCommandPool m_CommandPool; // For allocating command buffers
		VkCommandBuffer m_CommandBuffer; // For recording commands to send to the GPU
		VkCommandBufferBeginInfo m_CommandBufferBeginInfo; // For starting command buffer recording

		// Swapchain & Rendering
		VkSwapchainKHR m_Swapchain;
		VkFormat m_SwapchainImageFormat;
		VkExtent2D swapchain_extent;
		std::vector<VkImage> m_SwapchainImages;
		std::vector<VkImageView> m_SwapchainImageViews;

		// Compute shader pipeline
		VkDescriptorSetLayout m_DescriptorSetLayout;
		VkPipelineLayout m_PipelineLayout;
		VkPipeline m_VelocitiesPipeline;
		VkPipeline m_RenderCullingPipeline;
		
		// Memory allocator
		VmaAllocator m_Allocator;

		// Compute shader buffers
		AllocatedBuffer m_VelocitiesBuffer; // Object's velocity (x, y)
		AllocatedBuffer m_PositionsBuffer; // Object's position (x, y)
		AllocatedBuffer m_AnimationNumbersBuffer; // Object's animation number (index to the animation data)
		AllocatedBuffer m_FrameNumbersBuffer; // Object's current frame number (index to the animation data)
		AllocatedBuffer m_IndirectDrawBuffer; // Buffer for indirect draw commands (VkDrawIndirectCommand) to be read by the GPU
		AllocatedBuffer m_ValidIndexesBuffer; // Buffer for valid indexes of objects to render (compacted list of object IDs)

		// Bindless Texture Infrastructure
		VkDescriptorSetLayout m_GlobalDescriptorSetLayout; // Layout for the bindless array
		VkDescriptorSet m_GlobalBindlessDescriptorSet;     // The global bindless descriptor set
		VkSampler m_GlobalTextureSampler;                  // Global sampler for all textures
		std::vector<VkImageView> m_LoadedImageViews;       // Tracks all created image views for cleanup
		std::vector<VkImage> m_LoadedImages;               // Tracks all raw VRAM images for cleanup
		std::vector<VmaAllocation> m_LoadedAllocations;    // Tracks all VMA image allocations for cleanup

		// Constructor & Destructor
		Renderer(Window& p_Window, EngineCore& p_Engine);
		~Renderer();

		// Initializing vulkan
		void init_vulkan(GLFWwindow* window);

		vkb::PhysicalDevice SelectPhysicalDevice();
		vkb::Device BuildLogicalDevice(vkb::PhysicalDevice vkb_phys_device);
		void CreateVMAMemoryAllocator();

		// 2. Queues & Presentation
		void CreateGraphicsQueues(vkb::Device vkb_device);
		void CreateSwapchain(vkb::Device vkb_device);

		// 3. Shaders, Layouts & Pipelines
		void CreateComputeShaderLayout();
		VkShaderModule CreateShaderModule(VkDevice device, const std::vector<uint32_t>& code);

		// Creating and mapping a buffer for compute shaders
		AllocatedBuffer CreateAndMapBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VmaMemoryUsage memoryUsage);
		void CreateAndMapAllBuffers();
		void SendDataToBuffer(VAllocatedBuffer buffer, const std::vector<T>& data);
		void SubmitCommandBuffersAndWait(int32_t cmdBufferCount);

		void CreateBindlessDescriptorLayout();

		[[inline]] void BeginCommandBuffer();
		[[nodiscard]] inline VkBufferCopy CreateBufferCopyRegion(uint32_t srcOffset, uint32_t dstOffset, VkDeviceSize size);

		// Loading and storing shader files
		std::vector<uint32_t> LoadShaderFile(const std::string& _filePath);
		VkShaderModule CreateShaderModule(VkDevice device, const std::vector<uint32_t>& code);
		void LoadAndStoreShaders();

		// Get an animation's index in the data
		template<bool IsVerified = false> [[nodiscard]] 
		inline GetAnimationIndex(const std::string& name)

		// Get an animation's number of frames
		[[nodiscard]] inline uint32_t GetAnimationSize(uint32_t idx) const;
		[[nodiscard]] inline uint32_t GetAnimationSize(std::string name) const;

	public:
		friend class EngineCore;

		Renderer(Window& p_Window);

		RunComputeShaders(uint32_t objectCount, const ConstantFrameData& frameData);
		GLuint LoadAnimation(const std::vector<std::filesystem::path>& Paths, const std::string& name);
		[[nodiscard]] std::span<const GLuint> GetFramesForAnimation(const std::string& name);

		Renderer();
		~Renderer();

		void setWindow(Window& Window);

		void RenderObjects();
		void SetClearColor(); // Fix

		void ClearScreen();
		void Present();

	};
}