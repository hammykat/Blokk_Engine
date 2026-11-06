#include "Rendering.hpp"

#include <GLFW/glfw3.h>
#include <VkBootstrap.h>

#include "../EngineClassData.hpp"

namespace Blokk {

    Renderer::Renderer(GLFWwindow& p_Window, EngineCore& p_Engine) : 
        m_Window(p_Window), 
        m_Engine(p_Engine) 
    {
        m_GPU_UpdatingObjectCounts = {m_Engine.TotalObjects, m_Engine.DynamicObjectCount, m_Engine.StaticObjectCount};
        init_vulkan(&p_Window);
    }
    
    Renderer::~Renderer() {
        // Clean up
        vkDestroyPipelineLayout(m_Device, m_PipelineLayout, nullptr);
        vkDestroyDescriptorSetLayout(m_Device, m_DescriptorSetLayout, nullptr);
        vkDestroySwapchainKHR(m_Device, m_Swapchain, nullptr);
        vkDestroyDebugUtilsMessengerEXT(m_Instance, m_DebugMessager, nullptr);
        vkDestroySurfaceKHR(m_Instance, m_Surface, nullptr);
        vkDestroyPipeline(m_Device, m_VelocitiesPipeline, nullptr);
        vkDestroyPipeline(m_Device, m_RenderCullingPipeline, nullptr);
        vkDestroyDevice(m_Device, nullptr);
        vkDestroyInstance(m_Instance, nullptr);
        
        // Command pool
        if (m_CommandPool != VK_NULL_HANDLE) {
            vkDestroyCommandPool(m_Device, m_CommandPool, nullptr);
            m_CommandPool = VK_NULL_HANDLE;
        }

        // Semaphores and fences
        for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
            vkDestroySemaphore(m_Device, m_ImageAvailableSemaphores[i], nullptr);
            vkDestroySemaphore(m_Device, m_RenderFinishedSemaphores[i], nullptr);
            vkDestroyFence(m_Device, m_InFlightFences[i], nullptr);
        }
    }

    
    void Renderer::CreateAndMapAllBuffers() {
        uint32_t objectCount = m_Engine.ObjectCount; // Get the number of objects from the engine

        // Create and map buffers for compute shaders ----------------------------
        {
            // Buffer for indirect draw commands (VkDrawIndirectCommand) to be read by the GPU
            m_IndirectDrawBuffer = CreateAndMapBuffer(sizeof(VkDrawIndirectCommand), 
                UsageFlags::STORAGE_BUFFER | UsageFlags::INDIRECT_BUFFER | 
                UsageFlags::TRANSFER_DST, MemoryUsage::GPU_ONLY);

            m_ValidIndexesBuffer = CreateAndMapBuffer(objectCount * sizeof(uint32_t), UsageFlags::TRANSFER_SRC, MemoryUsage::GPU_ONLY);
            vkCmdFillBuffer(m_CommandBuffer, m_ValidIndexesBuffer.buffer, 0, objectCount * sizeof(uint32_t), 0);

            // Buffers that are sent ONCE at the start to the GPU. These are used to initialize the GPU's buffers for the compute shaders
            auto StartingVelocities = CreateAndMapBuffer(objectCount * sizeof(glm::vec2), UsageFlags::TRANSFER_SRC, MemoryUsage::CPU_TO_GPU);
            auto StartingPositions = CreateAndMapBuffer(objectCount * sizeof(glm::vec2), UsageFlags::TRANSFER_SRC, MemoryUsage::CPU_TO_GPU);
            auto StartingFrameNumbers = CreateAndMapBuffer(objectCount * sizeof(uint32_t), UsageFlags::TRANSFER_SRC, MemoryUsage::CPU_TO_GPU);

            // Buffers that are used for the compute shaders to read/write to. These are persistent buffers that the compute shaders will use every frame
            m_VelocitiesBuffer = CreateAndMapBuffer(objectCount * sizeof(glm::vec2), UsageFlags::STORAGE_BUFFER | UsageFlags::TRANSFER_DST, MemoryUsage::CPU_TO_GPU);
            m_PositionsBuffer = CreateAndMapBuffer(objectCount * sizeof(glm::vec2), UsageFlags::STORAGE_BUFFER | UsageFlags::TRANSFER_DST, MemoryUsage::GPU_ONLY);
            m_AnimationNumbersBuffer = CreateAndMapBuffer(objectCount * sizeof(uint32_t), UsageFlags::STORAGE_BUFFER, MemoryUsage::GPU_ONLY);
            m_FrameNumbersBuffer = CreateAndMapBuffer(objectCount * sizeof(uint32_t), UsageFlags::STORAGE_BUFFER, MemoryUsage::GPU_ONLY);

            // Populate the starting buffers with data from the engine
            SendDataToBuffer<glm::vec2>(StartingVelocities, m_Engine.Velocities);
            SendDataToBuffer<glm::vec2>(StartingPositions, m_Engine.Positions);
            SendDataToBuffer<uint32_t>(StartingFrameNumbers, m_Engine.FrameNumbers);
            

            // Begin the command buffer
            BeginCommandBuffer();

            // Region to copy from the starting buffers to the persistent buffers
            VkBufferCopy copyRegion = CreateBufferCopyRegion(0, 0, objectCount * sizeof(glm::vec2));

            vkCmdCopyBuffer(m_CommandBuffer, StartingVelocities.buffer, m_VelocitiesBuffer.buffer, 1, &copyRegion);
            vkCmdCopyBuffer(m_CommandBuffer, StartingPositions.buffer, m_PositionsBuffer.buffer, 1, &copyRegion);

            copyRegion.size = objectCount * sizeof(uint32_t);
            vkCmdCopyBuffer(m_CommandBuffer, StartingFrameNumbers.buffer, m_FrameNumbersBuffer.buffer, 1, &copyRegion);

            EndCommandBuffer(); // End the command buffer recording
            SubmitCommandBuffersAndWait(1); // Submit the command buffer and wait for completion

            // Clean up the starting buffers after copying
            DestroyBuffer(StartingVelocities);
            DestroyBuffer(StartingPositions);
            DestroyBuffer(StartingFrameNumbers);
        }
    }


    Renderer::AllocatedBuffer Renderer::CreateAndMapBuffer(VkDeviceSize size, VkBufferUsageFlags VkUsage, VmaMemoryUsage memoryUsage) 
    {
        // Tell Vulkan what the buffer will be used for and how large it is (in bytes)
        VkBufferCreateInfo bufferInfo{
            .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
            .size = size, // Total size in BYTES (e.g., count * sizeof(DataType))
            .usage = VkUsage,
            .sharingMode = VK_SHARING_MODE_EXCLUSIVE
        };

        // Tell VMA how to allocate memory
        VmaAllocationCreateInfo allocInfo{
            .flags = VMA_ALLOCATION_CREATE_MAPPED_BIT, // Automatically maps a CPU pointer for us
            .usage = memoryUsage 
        };

        AllocatedBuffer gpuBuffer;
        VmaAllocationInfo allocResultInfo;

        // Create the buffer and allocate memory via VMA
        if (vmaCreateBuffer(m_Allocator, &bufferInfo, &allocInfo, 
                            &gpuBuffer.buffer, &gpuBuffer.allocation, &allocResultInfo) != VK_SUCCESS) {
            throw std::runtime_error("Failed to allocate and map buffer via VMA!");
        }

        // Grab the pointer provided by VMA
        gpuBuffer.mappedData = allocResultInfo.pMappedData;

        return gpuBuffer;
    }

    // Initializes Vulkan. Uses vk-bootstrap to reduce a lot of code. 
    //   Builds the app, gets the window surface, selects the device, gets 
    //   the logical device, queues and swapchain
    void Renderer::init_vulkan(GLFWwindow* window) 
    {
        // Building the app
        vkb::InstanceBuilder builder;
        auto instance_result = builder.set_app_name("Blokk Engine")
                                      .request_validation_layers(true)
                                      .use_default_debug_messenger()
                                      .require_api_version(1, 3, 0) // Vulkan 1.3 
                                      .build();

        // Make sure nothing failed
        if (!instance_result) {
            std::cerr << "Failed to create Vulkan instance: " 
                      << instance_result.error().message() << "\n";
            return;
        }

        // Getting the window surface ------------------------------------

        // Unpack fully initialized instance helper
        vkb::Instance vkb_instance = instance_result.value();
        m_Instance = vkb_instance.instance();

        VkSurfaceKHR surface; // Window surface
        if (glfwCreateWindowSurface(vkb_instance.instance, window, nullptr, &surface) != VK_SUCCESS) {
            std::cerr << "Failed to create window surface!\n";
            return;
        }

        // Device selector on hardware 
        vkb::PhysicalDevice vkb_phys_device = SelectPhysicalDevice();

        // Logical device builder from selector
        m_VkbDevice = BuildLogicalDevice(vkb_phys_device);

        // Queues 
        CreateGraphicsQueues();

        // Swapchain 
        CreateSwapchain();

        // Compute shader layout
        CreateComputeShaderLayout();

        // Memory allocator (VMA) 
        CreateVMAMemoryAllocator();
        
        // Create command pool and buffers
        CreateCommandPoolAndBuffers();

        // Create bindless descriptor layout
        CreateBindlessDescriptorLayout();
    }

    void Renderer::CreateSemaphoresAndFences() {
        m_ImageAvailableSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
        m_RenderFinishedSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
        m_InFlightFences.resize(MAX_FRAMES_IN_FLIGHT);

        // Semaphore create info (pNext is usually nullptr)
        VkSemaphoreCreateInfo semaphoreInfo{};
        semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

        // Fence create info - IMPORTANT: Flag it as signaled so the first frame doesn't hang!
        VkFenceCreateInfo fenceInfo{};
        fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
        fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

        for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
            if (vkCreateSemaphore(m_Device, &semaphoreInfo, nullptr, &m_ImageAvailableSemaphores[i]) != VK_SUCCESS ||
                vkCreateSemaphore(m_Device, &semaphoreInfo, nullptr, &m_RenderFinishedSemaphores[i]) != VK_SUCCESS ||
                vkCreateFence(m_Device, &fenceInfo, nullptr, &m_InFlightFences[i]) != VK_SUCCESS) {
                throw std::runtime_error("Failed to create synchronization objects for a frame!");
            }
        }
    }

    void Renderer::CreateCommandPoolAndBuffers() 
    {
        // Create the Command Pool
        VkCommandPoolCreateInfo poolInfo{
            .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
            .flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT, // Allows resetting individual command buffers every frame
            .queueFamilyIndex = m_GraphicsQueueIndex // Tied directly to graphics/compute queue family
        };

        if (vkCreateCommandPool(m_Device, &poolInfo, nullptr, &m_CommandPool) != VK_SUCCESS) {
            throw std::runtime_error("Failed to create Vulkan command pool!");
        }

        // Allocate persistent per-frame command buffer
        VkCommandBufferAllocateInfo allocInfo{
            .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
            .commandPool = m_CommandPool,
            .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY, // Primary can be submitted directly to m_GraphicsQueue
            .commandBufferCount = 1
        };

        if (vkAllocateCommandBuffers(m_Device, &allocInfo, &m_CommandBuffer) != VK_SUCCESS) {
            throw std::runtime_error("Failed to allocate frame command buffer!");
        }

        m_CommandBufferBeginInfo = {
            .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
            .flags = 0
        };
    }

    std::vector<uint32_t> Renderer::LoadShaderFile(const std::string& _filePath) {
        std::ifstream file(_filePath, std::ios::ate | std::ios::binary);

        if (!file.is_open()) {
            auto err = std::format("Tried to open file at: '{}' but failed", _filePath);
            throw std::runtime_error(err);
        }

        uint32_t fileSize = static_cast<uint32_t>(file.tellg());
        std::vector<uint32_t> buffer(fileSize / sizeof(uint32_t));

        // Read into buffer
        file.seekg(0, std::ios::beg);
        file.read(reinterpret_cast<char*>(buffer.data()), fileSize);
        file.close(); // Make sure to close:)

        return buffer;
    }

    vkb::Device Renderer::BuildLogicalDevice(vkb::PhysicalDevice vkb_phys_device) {
        // Create a builder for the Logical Device using chosen GPU
        vkb::DeviceBuilder device_builder{ vkb_phys_device };
        auto device_result = device_builder.build();
    
        // Handle errors
        if (!device_result) {
            std::cerr << "Failed to create Logical Device: " 
                        << device_result.error().message() << "\n";
            return;
        }
    
        // Unpack fully initialized device helper
        vkb::Device vkb_device = device_result.value();

        // Store the raw Vulkan device handle (for future use)
        m_Device = vkb_device.device;

        return vkb_device;
    }

    vkb::PhysicalDevice Renderer::SelectPhysicalDevice(vkb::Instance vkb_instance) {
        vkb::PhysicalDeviceSelector selector{ vkb_instance };
    
        auto phys_device_result = selector.set_surface(surface)
                                          .set_minimum_version(1, 3)
                                          .select();

        if (!phys_device_result) {
            std::cerr << "Failed to select a Physical Device: " 
                      << phys_device_result.error().message() << "\n";
            return;
        }
    
        vkb::PhysicalDevice vkb_phys_device = phys_device_result.value();
        m_PhysicalDevice = vkb_phys_device.physical_device();

        return vkb_phys_device;
    }

    void Renderer::CreateVMAMemoryAllocator() {
        VmaAllocatorCreateInfo allocatorCreateInfo{
            .physicalDevice = m_PhysicalDevice,
            .device = m_Device,
            .instance = m_Instance,
            .vulkanApiVersion = VK_API_VERSION_1_3
        };

        if (vmaCreateAllocator(&allocatorCreateInfo, &m_Allocator) != VK_SUCCESS) {
            throw std::runtime_error("Failed to create VMA allocator!");
        }
    }

    void Renderer::CreateGraphicsQueues() {
        // Graphics Queue from logical device
        auto graphics_queue_result = m_VkbDevice.get_queue(vkb::QueueType::graphics);
        if (!graphics_queue_result) {
            std::cerr << "Failed to get graphics queue: " 
                        << graphics_queue_result.error().message() << "\n";
            return;
        }
        m_GraphicsQueue = graphics_queue_result.value();

        // Get the index of the graphics queue for future use
        auto graphics_index_result = m_VkbDevice.get_queue_index(vkb::QueueType::graphics);
        if (graphics_index_result.has_value()) {
            m_GraphicsQueueIndex = graphics_index_result.value();
        }

        // Present Queue (to push frames to the screen)
        auto present_queue_result = m_VkbDevice.get_queue(vkb::QueueType::present);
        if (!present_queue_result) {
            std::cerr << "Failed to get present queue: " 
                        << present_queue_result.error().message() << "\n";
            return;
        }

        m_PresentQueue = present_queue_result.value();
    }

    void Renderer::CreateSwapchain() {
        // Create a builder for the Swapchain
        vkb::SwapchainBuilder swapchain_builder{ m_VkbDevice };
    
        // Images need to match window's width and height (e.g., 800x600)
        auto width = static_cast<uint32_t>(m_Window.GetWidth()); auto height = static_cast<uint32_t>(m_Window.GetHeight());
        auto swapchain_result = swapchain_builder.set_desired_extent(width, height).build();

        if (!swapchain_result) {
            auto err = std::format("Failed to create swapchain: {} \n", swapchain_result.error().message());
            throw std::runtime_error(err);
        }
    
        // Unpack fully initialized swapchain helper
        vkb::Swapchain vkb_swapchain = swapchain_result.value();

        // Storing into members
        m_Swapchain = vkb_swapchain.swapchain; // Raw handles needed for actual loop
        m_SwapchainImages = vkb_swapchain.get_images().value(); // Get the literal image buffers we will paint on
        m_SwapchainImageViews = vkb_swapchain.get_image_views().value(); // Get the "Views" required to interact with those images
    }

    void Renderer::CreateComputeShaderLayout() {
        std::vector<VkDescriptorSetLayoutBinding> bindings = {
            {0, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1, VK_SHADER_STAGE_COMPUTE_BIT, nullptr}, // t0: Velocities
            {1, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1, VK_SHADER_STAGE_COMPUTE_BIT, nullptr}, // t1: Positions
            {2, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1, VK_SHADER_STAGE_COMPUTE_BIT, nullptr}, // t2: AnimationSizes
            {3, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1, VK_SHADER_STAGE_COMPUTE_BIT, nullptr}, // t3: FrameDimensions
            {4, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1, VK_SHADER_STAGE_COMPUTE_BIT, nullptr}, // t4: FrameNumbers
            {5, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1, VK_SHADER_STAGE_COMPUTE_BIT, nullptr}, // t5: AnimationNumbers
            {6, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1, VK_SHADER_STAGE_COMPUTE_BIT, nullptr}, // t6: AnimationStarts
            {7, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1, VK_SHADER_STAGE_COMPUTE_BIT, nullptr}, // t7: IsVisible
            {8, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1, VK_SHADER_STAGE_COMPUTE_BIT, nullptr}, // t8: ToRender
            {9, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1, VK_SHADER_STAGE_COMPUTE_BIT, nullptr}  // t9: RenderPositions
        };

        VkDescriptorSetLayoutCreateInfo layoutInfo{
            .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
            .bindingCount = static_cast<uint32_t>(bindings.size()),
            .pBindings = bindings.data()
        };

        // Create and store descriptor set layout
        if (vkCreateDescriptorSetLayout(m_Device, &layoutInfo, nullptr, &m_DescriptorSetLayout) != VK_SUCCESS) {
            throw std::runtime_error("Failed to create descriptor set layout!"); // Handle failiure
        }

        // Push constants
        VkPushConstantRange pushConstantRange{
            .stageFlags = VK_SHADER_STAGE_COMPUTE_BIT,
            .offset = 0,
            .size = sizeof(ComputeShaderConstantFrameData) // 36 bytes; check Rendering.hpp
        };

        VkPipelineLayoutCreateInfo pipelineLayoutInfo{
            .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
            .setLayoutCount = 1,
            .pSetLayouts = &m_DescriptorSetLayout,
            .pushConstantRangeCount = 1,
            .pPushConstantRanges = &pushConstantRange
        };

        // Create and store pipeline layout
        if (vkCreatePipelineLayout(m_Device, &pipelineLayoutInfo, nullptr, &m_PipelineLayout) != VK_SUCCESS) {
            throw std::runtime_error("Failed to create compute pipeline layout!"); // Handle failiure
        }
    }

    VkShaderModule Renderer::CreateShaderModule(VkDevice device, const std::vector<uint32_t>& code) {
        VkShaderModuleCreateInfo createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
        createInfo.codeSize = code.size() * sizeof(uint32_t); // Size in bytes
        createInfo.pCode = code.data();

        VkShaderModule shaderModule;
        if (vkCreateShaderModule(m_Device, &createInfo, nullptr, &shaderModule) != VK_SUCCESS) {
            throw std::runtime_error("Failed to create shader module!");
        }

        return shaderModule;
    }

    void Renderer::LoadAndStoreShaders() {
        auto velocityComputeShaderSource = LoadShaderFile("Velocities.comp.spv");
        VkShaderModule velocityShader = CreateShaderModule(m_Device, velocityComputeShaderSource);

        // Configure pipeline shader stage
        VkPipelineShaderStageCreateInfo shaderStageCreationInfo{
            .sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO,
            .stage = VK_SHADER_STAGE_COMPUTE_BIT,
            .module = velocityShader,
            .pName = "main"
        };

        // Set up pipeline create info
        VkComputePipelineCreateInfo pipelineCreationInfo{
            .sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO,
            .stage = shaderStageCreationInfo,
            .layout = m_PipelineLayout,
            .basePipelineHandle = VK_NULL_HANDLE,
            .basePipelineIndex = -1
        };

        // Create pipeline
        if (vkCreateComputePipelines(m_Device, VK_NULL_HANDLE, 1, &pipelineCreationInfo, nullptr, &m_VelocitiesPipeline) != VK_SUCCESS) {
            throw std::runtime_error("Failed to create compute pipeline!");
        }

        // Load compiled SPIR-V binary for your culling shader 
        auto cullingShaderCode = LoadShaderFile("Culling.comp.spv");
        VkShaderModule cullingShaderModule = CreateShaderModule(m_Device, cullingShaderCode);

        // Configure pipeline shader stage
        VkPipelineShaderStageCreateInfo cullingStageInfo{
            .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
            .stage = VK_SHADER_STAGE_COMPUTE_BIT,
            .module = cullingShaderModule,
            .pName = "main"
        };

        // Set up pipeline create info (reusing m_PipelineLayout!)
        VkComputePipelineCreateInfo cullingPipelineInfo{
            .sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO,
            .stage = cullingStageInfo,
            .layout = m_PipelineLayout,
            .basePipelineHandle = VK_NULL_HANDLE,
            .basePipelineIndex = -1
        };

        // Create the second compute pipeline
        if (vkCreateComputePipelines(m_Device, VK_NULL_HANDLE, 1, &cullingPipelineInfo, nullptr, &m_RenderCullingPipeline) != VK_SUCCESS) {
            throw std::runtime_error("Failed to create render culling compute pipeline!");
        }

        // Clean up the shader modules
        vkDestroyShaderModule(m_Device, cullingShaderModule, nullptr);
        vkDestroyShaderModule(m_Device, velocityShader, nullptr);
    }


    void Renderer::CreateBindlessDescriptorLayout()
    {
        // 1. Define binding for a large array of combined image samplers
        VkDescriptorSetLayoutBinding bindlessBinding{
            .binding = 0,
            .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
            .descriptorCount = 1024, // Max number of bindless textures
            .stageFlags = VK_SHADER_STAGE_FRAGMENT_SHADER_BIT,
            .pImmutableSamplers = nullptr
        };

        // 2. Enable partial binding and variable descriptor count flags for bindless rendering
        VkDescriptorBindingFlags bindlessFlags = VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT_EXT | 
                                                    VK_DESCRIPTOR_BINDING_VARIABLE_DESCRIPTOR_COUNT_BIT_EXT;

        VkDescriptorSetLayoutBindingFlagsCreateInfoEXT extendedInfo{
            .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_BINDING_FLAGS_CREATE_INFO_EXT,
            .bindingCount = 1,
            .pBindingFlags = &bindlessFlags
        };

        VkDescriptorSetLayoutCreateInfo layoutInfo{
            .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
            .pNext = &extendedInfo,
            .flags = VK_DESCRIPTOR_SET_LAYOUT_CREATE_UPDATE_AFTER_BIND_POOL_BIT_EXT,
            .bindingCount = 1,
            .pBindings = &bindlessBinding
        };

        if (vkCreateDescriptorSetLayout(m_Device, &layoutInfo, nullptr, &m_GlobalDescriptorSetLayout) != VK_SUCCESS) {
            throw std::runtime_error("Failed to create bindless descriptor set layout!");
        }

        // 3. Create a sampler that works across all bindless textures
        VkSamplerCreateInfo samplerInfo{
            .sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
            .magFilter = VK_FILTER_LINEAR,
            .minFilter = VK_FILTER_LINEAR,
            .mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR,
            .addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT,
            .addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT,
            .addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT,
            .maxAnisotropy = 1.0f,
            .minLod = 0.0f,
            .maxLod = 1.0f
        };

        if (vkCreateSampler(m_Device, &samplerInfo, nullptr, &m_GlobalTextureSampler) != VK_SUCCESS) {
            throw std::runtime_error("Failed to create global texture sampler!");
        }
    }
}

