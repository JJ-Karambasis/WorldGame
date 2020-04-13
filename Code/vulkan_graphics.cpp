#include "vulkan_graphics.h"
#include "geometry.cpp"
#include "graphics.cpp"

b32 IsCompatibleDevice(physical_device* GPU)
{
    b32 Result = ((GPU->GraphicsFamilyIndex != -1) &&
                  (GPU->PresentFamilyIndex != -1) &&
                  (GPU->SurfaceFormat.format != VK_FORMAT_UNDEFINED) &&
                  (GPU->ColorImageCount > 1) &&
                  (GPU->DepthFormat != VK_FORMAT_UNDEFINED));
    
#if DEVELOPER_BUILD
    Result &= GPU->Features.geometryShader;
#endif
    
    return Result;
}

inline void* 
Push(upload_buffer* Buffer, ptr Size)
{
    ASSERT((Buffer->Used + Size) < Buffer->Size);
    void* Result = (u8*)Buffer->MappedMemory + Buffer->Used;
    Buffer->Used += Size;
    return Result;
}

inline VkDeviceSize 
PushWrite(upload_buffer* Buffer, void* Data, ptr Size)
{
    VkDeviceSize Result = Buffer->Used;
    void* MappedData = Push(Buffer, Size);
    CopyMemory(MappedData, Data, Size);
    return Result;
}

inline void 
Reset(upload_buffer* Buffer)
{
    Buffer->Used = 0;
}

VkBufferCreateInfo GetBufferInfo(VkDeviceSize Size, VkBufferUsageFlags Usage)
{
    VkBufferCreateInfo Result = {};
    Result.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    Result.size = Size;
    Result.usage = Usage;
    Result.sharingMode = VK_SHARING_MODE_EXCLUSIVE;    
    return Result;
}

VkCommandBufferBeginInfo* OneTimeCommandBufferBeginInfo()
{
    local VkCommandBufferBeginInfo CommandBufferBeginInfo;
    CommandBufferBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    CommandBufferBeginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    return &CommandBufferBeginInfo;
}

VkPipelineDynamicStateCreateInfo* ViewportScissorDynamicState()
{    
    local VkDynamicState DynamicStates[2] = {VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR};    
    
    local VkPipelineDynamicStateCreateInfo DynamicState;
    DynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    DynamicState.dynamicStateCount = ARRAYCOUNT(DynamicStates);
    DynamicState.pDynamicStates = DynamicStates;
    return &DynamicState;
}

VkPipelineDepthStencilStateCreateInfo* DepthOffState()
{
    local VkPipelineDepthStencilStateCreateInfo DepthStencilState;
    DepthStencilState.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;    
    return &DepthStencilState;
}

VkPipelineDepthStencilStateCreateInfo* DepthOnState()
{
    local VkPipelineDepthStencilStateCreateInfo DepthStencilState;
    DepthStencilState.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    DepthStencilState.depthTestEnable = VK_TRUE;
    DepthStencilState.depthWriteEnable = VK_TRUE;
    DepthStencilState.depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL;    
    return &DepthStencilState;
}

VkPipelineColorBlendStateCreateInfo* DefaultColorBlendState()
{    
    local VkPipelineColorBlendAttachmentState ColorBlendAttachment;
    ColorBlendAttachment.blendEnable = VK_FALSE;
    ColorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT|VK_COLOR_COMPONENT_G_BIT|VK_COLOR_COMPONENT_B_BIT|VK_COLOR_COMPONENT_A_BIT;
    
    local VkPipelineColorBlendStateCreateInfo ColorBlendState;
    ColorBlendState.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    ColorBlendState.attachmentCount = 1;
    ColorBlendState.pAttachments = &ColorBlendAttachment;    
    
    return &ColorBlendState;
}

VkPipelineMultisampleStateCreateInfo* DefaultMultisampleState()
{    
    local VkPipelineMultisampleStateCreateInfo MultisampleState;
    MultisampleState.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    MultisampleState.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;    
    return &MultisampleState;
}

VkPipelineRasterizationStateCreateInfo* DefaultCullNoneRasterizationState()
{    
    local VkPipelineRasterizationStateCreateInfo RasterizationState;
    RasterizationState.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    RasterizationState.polygonMode = VK_POLYGON_MODE_FILL;
    RasterizationState.cullMode = VK_CULL_MODE_NONE;
    RasterizationState.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
    RasterizationState.lineWidth = 1.0f;
    return &RasterizationState;
};

VkPipelineRasterizationStateCreateInfo* DefaultCullBackRasterizationState()
{    
    local VkPipelineRasterizationStateCreateInfo RasterizationState;
    RasterizationState.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    RasterizationState.polygonMode = VK_POLYGON_MODE_FILL;
    RasterizationState.cullMode = VK_CULL_MODE_BACK_BIT;
    RasterizationState.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
    RasterizationState.lineWidth = 1.0f;
    return &RasterizationState;
};

VkPipelineViewportStateCreateInfo* SingleViewportState()
{    
    local VkPipelineViewportStateCreateInfo ViewportState;
    ViewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    ViewportState.viewportCount = 1;
    ViewportState.scissorCount = 1;    
    return &ViewportState;
}

VkPipelineInputAssemblyStateCreateInfo* TriangleListInputAssemblyState()
{    
    local VkPipelineInputAssemblyStateCreateInfo InputAssemblyState;
    InputAssemblyState.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    InputAssemblyState.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    return &InputAssemblyState;
}

VkPipelineInputAssemblyStateCreateInfo* LineListInputAssemblyState()
{    
    local VkPipelineInputAssemblyStateCreateInfo InputAssemblyState;
    InputAssemblyState.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    InputAssemblyState.topology = VK_PRIMITIVE_TOPOLOGY_LINE_LIST;
    return &InputAssemblyState;
}

VkPipelineInputAssemblyStateCreateInfo* PointListInputAssemblyState()
{    
    local VkPipelineInputAssemblyStateCreateInfo InputAssemblyState;
    InputAssemblyState.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    InputAssemblyState.topology = VK_PRIMITIVE_TOPOLOGY_POINT_LIST;
    return &InputAssemblyState;
}

VkPipelineVertexInputStateCreateInfo* EmptyVertexInputState()
{    
    local VkPipelineVertexInputStateCreateInfo VertexInputState;
    VertexInputState.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;    
    return &VertexInputState;
}

extensions_array GetDeviceExtensions(physical_device* GPU)
{
    extensions_array Result = {};    
    const u32 MaxExtensionsCount = 3;
    Result.Ptr = PushArray(MaxExtensionsCount, char*, Clear, 0);
    
    Result.Ptr[Result.Count++] = VK_KHR_SWAPCHAIN_EXTENSION_NAME;    
    if(GPU->FoundDedicatedMemoryExtension)    
    {
        Result.Ptr[Result.Count++] = VK_KHR_DEDICATED_ALLOCATION_EXTENSION_NAME;
        Result.Ptr[Result.Count++] = VK_KHR_GET_MEMORY_REQUIREMENTS_2_EXTENSION_NAME;
    }
    
    return Result;
}

memory_info GetImageMemoryInfo(VkImage Image)
{
    vulkan_graphics* Graphics = GetVulkanGraphics();
    memory_info Result = {};    
    
    if(Graphics->SelectedGPU->FoundDedicatedMemoryExtension)
    {        
        VkMemoryDedicatedRequirements DedicatedRequirements = {};
        DedicatedRequirements.sType = VK_STRUCTURE_TYPE_MEMORY_DEDICATED_REQUIREMENTS;
        
        VkMemoryRequirements2 MemoryRequirements2 = {};
        MemoryRequirements2.sType = VK_STRUCTURE_TYPE_MEMORY_REQUIREMENTS_2;        
        MemoryRequirements2.pNext = &DedicatedRequirements;
        
        VkImageMemoryRequirementsInfo2 MemoryRequirementsInfo2 = {};
        MemoryRequirementsInfo2.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_REQUIREMENTS_INFO_2;
        MemoryRequirementsInfo2.image = Image;
        
        vkGetImageMemoryRequirements2KHR(Graphics->Device, &MemoryRequirementsInfo2, &MemoryRequirements2);
        Result.Requirements = MemoryRequirements2.memoryRequirements;
        Result.DedicatedAllocation = (DedicatedRequirements.requiresDedicatedAllocation != VK_FALSE) || (DedicatedRequirements.prefersDedicatedAllocation != VK_FALSE);
    }
    else
    {
        vkGetImageMemoryRequirements(Graphics->Device, Image, &Result.Requirements);
    }    
    
    return Result;
}

memory_info GetBufferMemoryInfo(VkBuffer Buffer)
{
    vulkan_graphics* Graphics = GetVulkanGraphics();
    memory_info Result = {};    
    
    if(Graphics->SelectedGPU->FoundDedicatedMemoryExtension)
    {        
        VkMemoryDedicatedRequirements DedicatedRequirements = {};
        DedicatedRequirements.sType = VK_STRUCTURE_TYPE_MEMORY_DEDICATED_REQUIREMENTS;
        
        VkMemoryRequirements2 MemoryRequirements2 = {};
        MemoryRequirements2.sType = VK_STRUCTURE_TYPE_MEMORY_REQUIREMENTS_2;        
        MemoryRequirements2.pNext = &DedicatedRequirements;
        
        
        VkBufferMemoryRequirementsInfo2 MemoryRequirementsInfo2 = {};
        MemoryRequirementsInfo2.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_REQUIREMENTS_INFO_2;
        MemoryRequirementsInfo2.buffer = Buffer;
        
        vkGetBufferMemoryRequirements2KHR(Graphics->Device, &MemoryRequirementsInfo2, &MemoryRequirements2);
        Result.Requirements = MemoryRequirements2.memoryRequirements;
        Result.DedicatedAllocation = (DedicatedRequirements.requiresDedicatedAllocation != VK_FALSE) || (DedicatedRequirements.prefersDedicatedAllocation != VK_FALSE);
    }
    else
    {
        vkGetBufferMemoryRequirements(Graphics->Device, Buffer, &Result.Requirements);
    }    
    
    return Result;
}

i32 FindMemoryTypeIndex(VkPhysicalDeviceMemoryProperties* MemoryProperties, 
                        VkMemoryRequirements* MemoryRequirements, 
                        VkMemoryPropertyFlags TargetProperty)
{
    for(u32 MemoryIndex = 0; MemoryIndex < MemoryProperties->memoryTypeCount; MemoryIndex++)
    {
        u32 MemoryBitIndex = BIT_SET(MemoryIndex);
        if(MemoryBitIndex & MemoryRequirements->memoryTypeBits)
        {
            b32 HasRequiredProperties = (TargetProperty & MemoryProperties->memoryTypes[MemoryIndex].propertyFlags);
            if(HasRequiredProperties)
            {
                return (i32)MemoryIndex;
            }            
        }
    }    
    return -1;
}

VkDeviceMemory AllocateBufferMemory(VkBuffer Buffer, VkMemoryPropertyFlags Flags)
{
    memory_info MemoryInfo = GetBufferMemoryInfo(Buffer);
    
    i32 MemoryTypeIndex = FindMemoryTypeIndex(&GetVulkanGraphics()->SelectedGPU->MemoryProperties, &MemoryInfo.Requirements, Flags);
    if(MemoryTypeIndex == -1)
        WRITE_AND_HANDLE_ERROR("Failed to find a valid memory type for the imgui vertex buffer.");    
    
    VkMemoryAllocateInfo AllocateInfo = {};
    AllocateInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    AllocateInfo.memoryTypeIndex = MemoryTypeIndex;    
    AllocateInfo.allocationSize = MemoryInfo.Requirements.size;
    
    VkMemoryDedicatedAllocateInfo DedicatedAllocateInfo = {};
    if(MemoryInfo.DedicatedAllocation)
    {        
        DedicatedAllocateInfo.sType = VK_STRUCTURE_TYPE_MEMORY_DEDICATED_ALLOCATE_INFO;
        DedicatedAllocateInfo.buffer = Buffer;
        AllocateInfo.pNext = &DedicatedAllocateInfo;
    }
    
    VkDeviceMemory Memory;
    VULKAN_CHECK_AND_HANDLE(vkAllocateMemory(GetVulkanGraphics()->Device, &AllocateInfo, VK_NULL_HANDLE, &Memory),
                            "Failed to allocate the depth buffer memory.");
    
    return Memory;
    
    handle_error:
    return VK_NULL_HANDLE;
}

VkDeviceMemory AllocateImageMemory(VkImage Image, VkMemoryPropertyFlags Flags)
{
    memory_info MemoryInfo = GetImageMemoryInfo(Image);
    
    i32 MemoryTypeIndex = FindMemoryTypeIndex(&GetVulkanGraphics()->SelectedGPU->MemoryProperties, &MemoryInfo.Requirements, Flags);
    if(MemoryTypeIndex == -1)
        WRITE_AND_HANDLE_ERROR("Failed to find a valid memory type for the imgui vertex buffer.");    
    
    VkMemoryAllocateInfo AllocateInfo = {};
    AllocateInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    AllocateInfo.memoryTypeIndex = MemoryTypeIndex;    
    AllocateInfo.allocationSize = MemoryInfo.Requirements.size;
    
    VkMemoryDedicatedAllocateInfo DedicatedAllocateInfo = {};
    if(MemoryInfo.DedicatedAllocation)
    {        
        DedicatedAllocateInfo.sType = VK_STRUCTURE_TYPE_MEMORY_DEDICATED_ALLOCATE_INFO;
        DedicatedAllocateInfo.image = Image;
        AllocateInfo.pNext = &DedicatedAllocateInfo;
    }
    
    VkDeviceMemory Memory;
    VULKAN_CHECK_AND_HANDLE(vkAllocateMemory(GetVulkanGraphics()->Device, &AllocateInfo, VK_NULL_HANDLE, &Memory),
                            "Failed to allocate the depth buffer memory.");
    
    return Memory;
    
    handle_error:
    return VK_NULL_HANDLE;
}

VkDeviceMemory AllocateAndBindMemory(VkBuffer Buffer, VkMemoryPropertyFlags Flags)
{    
    VkDeviceMemory Memory = AllocateBufferMemory(Buffer, Flags);
    BOOL_CHECK_AND_HANDLE(Memory, "Failed to allocate the buffer memory.");    
    VULKAN_CHECK_AND_HANDLE(vkBindBufferMemory(GetVulkanGraphics()->Device, Buffer, Memory, 0),"Failed to bind the buffer to the memory.");
    
    return Memory;
    
    handle_error:
    return VK_NULL_HANDLE;
}

VkDeviceMemory AllocateAndBindMemory(VkImage Image, VkMemoryPropertyFlags Flags)
{
    VkDeviceMemory Memory = AllocateImageMemory(Image, Flags);
    BOOL_CHECK_AND_HANDLE(Memory, "Failed to allocate the image memory.");    
    VULKAN_CHECK_AND_HANDLE(vkBindImageMemory(GetVulkanGraphics()->Device, Image, Memory, 0),"Failed to bind the image to the memory.");
    
    return Memory;
    
    handle_error:
    return VK_NULL_HANDLE;    
}

upload_buffer CreateUploadBuffer(VkDeviceSize Size)
{   
    upload_buffer Result = {};
    
    vulkan_graphics* Graphics = GetVulkanGraphics();
    
    VkBufferCreateInfo BufferInfo = GetBufferInfo(Size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT);
    VULKAN_CHECK_AND_HANDLE(vkCreateBuffer(Graphics->Device, &BufferInfo, VK_NULL_HANDLE, &Result.Buffer), "Failed to create the upload buffer.");
    
    Result.Memory = AllocateAndBindMemory(Result.Buffer, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);        
    BOOL_CHECK_AND_HANDLE(Result.Memory, "Failed to allocate the upload buffer memory.");            
    
    VULKAN_CHECK_AND_HANDLE(vkMapMemory(Graphics->Device, Result.Memory, 0, Size, 0, (void**)(&Result.MappedMemory)),
                            "Failed to map the upload buffer to cpu memory.");
    
    Result.Size = Size;
    
    return Result;
    
    handle_error:
    return {};
}

VkShaderModule CreateShader(char* Path)
{
    buffer ShaderFile = Global_Platform->ReadEntireFile(Path);
    BOOL_CHECK_AND_HANDLE(ShaderFile.Data, "Failed to load the shader file %s", Path);
    
    VkShaderModuleCreateInfo ShaderModuleInfo = {};
    ShaderModuleInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    ShaderModuleInfo.codeSize = ShaderFile.Size;
    ShaderModuleInfo.pCode = (u32*)ShaderFile.Data;
    
    VkShaderModule Result;
    VULKAN_CHECK_AND_HANDLE(vkCreateShaderModule(GetVulkanGraphics()->Device, &ShaderModuleInfo, VK_NULL_HANDLE, &Result),
                            "Failed to create the vulkan shader module.");
    
    return Result;
    
    handle_error:
    return VK_NULL_HANDLE;
}

#if DEVELOPER_BUILD
VkBool32 DebugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT MessageSeverity, VkDebugUtilsMessageTypeFlagsEXT MessageTypes,
                       const VkDebugUtilsMessengerCallbackDataEXT* CallbackData, void* UserData)
{
    CONSOLE_LOG("Debug event occurred: %s\n", CallbackData->pMessage);    
    ASSERT(false);
    //NOTE(EVERYONE): According to the specification this always needs to return VK_FALSE -_-
    return VK_FALSE;
}
#endif

inline VkSemaphore 
CreateSemaphore()
{
    VkSemaphore Result = VK_NULL_HANDLE;
    VkSemaphoreCreateInfo SemaphoreInfo = {VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO};    
    if(vkCreateSemaphore(GetVulkanGraphics()->Device, &SemaphoreInfo, VK_NULL_HANDLE, &Result) != VK_SUCCESS)
        return VK_NULL_HANDLE;
    return Result;
}

render_buffer CreateRenderBuffer(v2i Dimensions, VkSwapchainKHR OldSwapchain)
{
    vulkan_graphics* Graphics = GetVulkanGraphics();    
    physical_device* GPU = Graphics->SelectedGPU;        
    
    if(Graphics->RenderBuffer.DepthImage)
        vkDestroyImage(Graphics->Device, Graphics->RenderBuffer.DepthImage, VK_NULL_HANDLE);
    
    if(Graphics->RenderBuffer.DepthImageView)
        vkDestroyImageView(Graphics->Device, Graphics->RenderBuffer.DepthImageView, VK_NULL_HANDLE);
    
    if(Graphics->RenderBuffer.DepthMemory)
        vkFreeMemory(Graphics->Device, Graphics->RenderBuffer.DepthMemory, VK_NULL_HANDLE);
    
    VkImageCreateInfo DepthImageInfo = {};
    DepthImageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    DepthImageInfo.imageType = VK_IMAGE_TYPE_2D;
    DepthImageInfo.format = GPU->DepthFormat;
    DepthImageInfo.extent = {(u32)Dimensions.width, (u32)Dimensions.height, 1};
    DepthImageInfo.mipLevels = 1;
    DepthImageInfo.arrayLayers = 1;
    DepthImageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
    DepthImageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
    DepthImageInfo.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
    DepthImageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;    
    
    VkImage DepthImage;
    VULKAN_CHECK_AND_HANDLE(vkCreateImage(Graphics->Device, &DepthImageInfo, VK_NULL_HANDLE, &DepthImage), 
                            "Failed to create the depth image.");
    
    VkDeviceMemory DepthMemory = AllocateAndBindMemory(DepthImage, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    BOOL_CHECK_AND_HANDLE(DepthMemory, "Failed to allocate the depth buffer memory.");
    
    VkImageViewCreateInfo DepthImageViewInfo = {};
    DepthImageViewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    DepthImageViewInfo.image = DepthImage;
    DepthImageViewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    DepthImageViewInfo.format = GPU->DepthFormat;
    DepthImageViewInfo.components = 
    {
        VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY, 
        VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY
    };
    DepthImageViewInfo.subresourceRange = {VK_IMAGE_ASPECT_DEPTH_BIT, 0, 1, 0, 1};        
    
    VkImageView DepthImageView;
    VULKAN_CHECK_AND_HANDLE(vkCreateImageView(Graphics->Device, &DepthImageViewInfo, VK_NULL_HANDLE, &DepthImageView),
                            "Failed to create the depth image view.");
    
    VkSwapchainCreateInfoKHR SwapchainInfo = {};
    SwapchainInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    SwapchainInfo.surface = Graphics->Surface;
    SwapchainInfo.minImageCount = GPU->ColorImageCount;
    SwapchainInfo.imageFormat = GPU->SurfaceFormat.format;
    SwapchainInfo.imageColorSpace = GPU->SurfaceFormat.colorSpace;
    SwapchainInfo.imageExtent = {(u32)Dimensions.x, (u32)Dimensions.y};
    SwapchainInfo.imageArrayLayers = 1;
    SwapchainInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;  
    SwapchainInfo.preTransform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;
    SwapchainInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    SwapchainInfo.presentMode = VK_PRESENT_MODE_FIFO_KHR;
    SwapchainInfo.oldSwapchain = OldSwapchain;
    
    u32 QueueFamilyIndices[2] = {(u32)GPU->PresentFamilyIndex, (u32)GPU->GraphicsFamilyIndex};
    if(GPU->PresentFamilyIndex != GPU->GraphicsFamilyIndex)
    {
        SwapchainInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
        SwapchainInfo.queueFamilyIndexCount = 2;
        SwapchainInfo.pQueueFamilyIndices = QueueFamilyIndices;
    }
    else    
        SwapchainInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;            
    
    VkSwapchainKHR Swapchain = VK_NULL_HANDLE;
    VULKAN_CHECK_AND_HANDLE(vkCreateSwapchainKHR(Graphics->Device, &SwapchainInfo, VK_NULL_HANDLE, &Swapchain),
                            "Failed to create the vulkan swapchain.");
    
    if(OldSwapchain)    
        vkDestroySwapchainKHR(Graphics->Device, OldSwapchain, VK_NULL_HANDLE);    
    
    u32 ColorImageCount = 0;
    VULKAN_CHECK_AND_HANDLE(vkGetSwapchainImagesKHR(Graphics->Device, Swapchain, &ColorImageCount, VK_NULL_HANDLE),
                            "Failed to retrieve the amount of images from the swapchain.");
    
    VkImage ColorImages[3];
    VULKAN_CHECK_AND_HANDLE(vkGetSwapchainImagesKHR(Graphics->Device, Swapchain, &ColorImageCount, ColorImages),
                            "Failed to retrieve the images from the swapchain.");
    
    VkImageView ColorImageViews[3];
    VkFramebuffer Framebuffers[3];
    
    for(u32 ImageIndex = 0; ImageIndex < ColorImageCount; ImageIndex++)
    {
        if(Graphics->RenderBuffer.ColorImageViews[ImageIndex])        
            vkDestroyImageView(Graphics->Device, Graphics->RenderBuffer.ColorImageViews[ImageIndex], VK_NULL_HANDLE);        
        
        if(Graphics->RenderBuffer.Framebuffers[ImageIndex])
            vkDestroyFramebuffer(Graphics->Device, Graphics->RenderBuffer.Framebuffers[ImageIndex], VK_NULL_HANDLE);
        
        VkImageViewCreateInfo ImageViewInfo = {};
        ImageViewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        ImageViewInfo.image = ColorImages[ImageIndex];
        ImageViewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        ImageViewInfo.format = GPU->SurfaceFormat.format;
        ImageViewInfo.components = 
        {
            VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY, 
            VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY
        };
        ImageViewInfo.subresourceRange = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1};        
        VULKAN_CHECK_AND_HANDLE(vkCreateImageView(Graphics->Device, &ImageViewInfo, VK_NULL_HANDLE, &ColorImageViews[ImageIndex]),
                                "Failed to create the color image view, index %d", ImageIndex);
        
        VkImageView AttachmentViews[] = {ColorImageViews[ImageIndex], DepthImageView};
        
        VkFramebufferCreateInfo FramebufferInfo = {};
        FramebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        FramebufferInfo.renderPass = Graphics->RenderPass;
        FramebufferInfo.attachmentCount = ARRAYCOUNT(AttachmentViews);
        FramebufferInfo.pAttachments = AttachmentViews;
        FramebufferInfo.width = Dimensions.width;
        FramebufferInfo.height = Dimensions.height;
        FramebufferInfo.layers = 1;
        VULKAN_CHECK_AND_HANDLE(vkCreateFramebuffer(Graphics->Device, &FramebufferInfo, VK_NULL_HANDLE, &Framebuffers[ImageIndex]),
                                "Failed to create the frame buffer, index %d", ImageIndex);
    }
    
    render_buffer Result;
    Result.Dimensions = Dimensions;
    Result.Swapchain = Swapchain;
    Result.ColorImageCount = ColorImageCount;
    Result.DepthMemory = DepthMemory;
    Result.DepthImage = DepthImage;
    Result.DepthImageView = DepthImageView;
    CopyArray(Result.ColorImages, ColorImages, 3, VkImage);
    CopyArray(Result.ColorImageViews, ColorImageViews, 3, VkImageView);
    CopyArray(Result.Framebuffers, Framebuffers, 3, VkFramebuffer);
    
    return Result;
    
    handle_error:
    return {};
}

VkDevice CreateLogicalDevice(physical_device* GPU)
{    
    const local f32 QUEUE_PRIORITY = 1.0f;
    
    u32 DeviceQueueInfoCount = 1;
    VkDeviceQueueCreateInfo DeviceQueueInfo[2] = {};
    DeviceQueueInfo[0].sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    DeviceQueueInfo[0].queueFamilyIndex = GPU->GraphicsFamilyIndex;
    DeviceQueueInfo[0].queueCount = 1;
    DeviceQueueInfo[0].pQueuePriorities = &QUEUE_PRIORITY;    
    
    if(GPU->GraphicsFamilyIndex != GPU->PresentFamilyIndex)
    {        
        DeviceQueueInfo[1].sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        DeviceQueueInfo[1].queueFamilyIndex = GPU->PresentFamilyIndex;
        DeviceQueueInfo[1].queueCount = 1;
        DeviceQueueInfo[1].pQueuePriorities = &QUEUE_PRIORITY;
        DeviceQueueInfoCount++;
    }
    
    extensions_array Extensions = GetDeviceExtensions(GPU);        
    
    VkPhysicalDeviceFeatures DeviceFeatures = {};
    DeviceFeatures.geometryShader = GPU->Features.geometryShader;
    
    VkDeviceCreateInfo DeviceInfo = {};
    DeviceInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    DeviceInfo.queueCreateInfoCount = DeviceQueueInfoCount;
    DeviceInfo.pQueueCreateInfos = DeviceQueueInfo;
    DeviceInfo.enabledExtensionCount = Extensions.Count;
    DeviceInfo.ppEnabledExtensionNames = Extensions.Ptr;
    DeviceInfo.pEnabledFeatures = &DeviceFeatures;
    
    VkDevice Result = VK_NULL_HANDLE;
    if(vkCreateDevice(GPU->Handle, &DeviceInfo, VK_NULL_HANDLE, &Result) == VK_SUCCESS)
        return Result;    
    return VK_NULL_HANDLE;
}

physical_device* GetFirstCompatbileGPU(physical_device_array* GPUs)
{
    for(u32 GPUIndex = 0; GPUIndex < GPUs->Count; GPUIndex++)
    {
        physical_device* Device = GPUs->Ptr + GPUIndex;
        if(IsCompatibleDevice(Device))       
            return Device;                    
    }    
    return NULL;
}

physical_device_array CreatePhysicalDevices()
{
    vulkan_graphics* Graphics = GetVulkanGraphics();
    
    physical_device_array Result = {};
    VULKAN_CHECK_AND_HANDLE(vkEnumeratePhysicalDevices(Graphics->Instance, &Result.Count, VK_NULL_HANDLE),
                            "Failed to retrieve the amount of gpu's installed on the machine.");        
    
    VkPhysicalDevice* GPUs = PushArray(Result.Count, VkPhysicalDevice, Clear, 0);
    VULKAN_CHECK_AND_HANDLE(vkEnumeratePhysicalDevices(Graphics->Instance, &Result.Count, GPUs),
                            "Failed to retrieve the gpu's installed on the machine.");
    
    Result.Ptr = PushArray(&Graphics->Storage, Result.Count, physical_device, Clear, 0);
    
    for(u32 GPUIndex = 0; GPUIndex < Result.Count; GPUIndex++)
    {
        VkPhysicalDevice GPU = GPUs[GPUIndex];
        Result.Ptr[GPUIndex].Handle = GPU;
        Result.Ptr[GPUIndex].GraphicsFamilyIndex = -1;
        
        VkPhysicalDeviceProperties GPUProperties = {};
        vkGetPhysicalDeviceProperties(GPU, &GPUProperties);
        
        CONSOLE_LOG("GPU %d\n", GPUIndex);
        CONSOLE_LOG("\tName: %s\n", GPUProperties.deviceName);
        CONSOLE_LOG("\tAPI Version: %d.%d.%d\n", VK_VERSION_MAJOR(GPUProperties.apiVersion), VK_VERSION_MINOR(GPUProperties.apiVersion), VK_VERSION_PATCH(GPUProperties.apiVersion));
        CONSOLE_LOG("\tDriver Version: %d.%d.%d\n", VK_VERSION_MAJOR(GPUProperties.driverVersion), VK_VERSION_MINOR(GPUProperties.driverVersion), VK_VERSION_PATCH(GPUProperties.driverVersion));
        CONSOLE_LOG("\tDevice Type ID: %d\n", (u32)GPUProperties.deviceType);        
        
        u32 QueueFamilyPropertyCount = 0;
        vkGetPhysicalDeviceQueueFamilyProperties(GPU, &QueueFamilyPropertyCount, VK_NULL_HANDLE);
        
        VkQueueFamilyProperties* QueueFamilyProperties = PushArray(QueueFamilyPropertyCount, VkQueueFamilyProperties, Clear, 0);
        vkGetPhysicalDeviceQueueFamilyProperties(GPU, &QueueFamilyPropertyCount, QueueFamilyProperties);
        
        b32 Passed = false;
        
        u32 ExtensionCount = 0;
        VkResult VKResult = vkEnumerateDeviceExtensionProperties(GPU, VK_NULL_HANDLE, &ExtensionCount, VK_NULL_HANDLE);
        if(VKResult != VK_SUCCESS)
        {
            CONSOLE_LOG("Device FAILED!\n\n");
            continue;
        }
        
        VkExtensionProperties* Extensions = PushArray(ExtensionCount, VkExtensionProperties, Clear, 0);
        VKResult = vkEnumerateDeviceExtensionProperties(GPU, VK_NULL_HANDLE, &ExtensionCount, Extensions);
        if(VKResult != VK_SUCCESS)
        {
            CONSOLE_LOG("Device FAILED!\n\n");
            continue;
        }
        
        
        b32 FoundSwapchainExtension = false;
        for(u32 ExtensionIndex = 0; ExtensionIndex < ExtensionCount; ExtensionIndex++)
        {
            if(StringEquals(Extensions[ExtensionIndex].extensionName, VK_KHR_SWAPCHAIN_EXTENSION_NAME))
                FoundSwapchainExtension = true;
            
            if(StringEquals(Extensions[ExtensionIndex].extensionName, VK_KHR_DEDICATED_ALLOCATION_EXTENSION_NAME))
                Result.Ptr[GPUIndex].FoundDedicatedMemoryExtension = true;
        }
        
        if(!FoundSwapchainExtension)
        {
            CONSOLE_LOG("Device FAILED!\n\n");
            continue;
        }
        
        for(u32 QueueFamilyIndex = 0; QueueFamilyIndex < QueueFamilyPropertyCount; QueueFamilyIndex++)
        {
            VkQueueFamilyProperties* QueueFamilyProperty = QueueFamilyProperties + QueueFamilyIndex;
            if(QueueFamilyProperty->queueFlags & VK_QUEUE_GRAPHICS_BIT)
            {                
                Result.Ptr[GPUIndex].GraphicsFamilyIndex = QueueFamilyIndex;
            }            
            
            VkBool32 PresentSupported = VK_FALSE;
            if((vkGetPhysicalDeviceSurfaceSupportKHR(GPU, QueueFamilyIndex, Graphics->Surface, &PresentSupported) == VK_SUCCESS) && (PresentSupported == VK_TRUE))
            {   
                Result.Ptr[GPUIndex].PresentFamilyIndex = QueueFamilyIndex;
                if(Result.Ptr[GPUIndex].GraphicsFamilyIndex == Result.Ptr[GPUIndex].PresentFamilyIndex)                          
                    break;                            
            }
        }
        
        
        if((Result.Ptr[GPUIndex].GraphicsFamilyIndex == -1) || 
           (Result.Ptr[GPUIndex].PresentFamilyIndex == -1))
        {
            CONSOLE_LOG("Device FAILED!\n\n");
            continue;
        }
        
        u32 FormatCount = 0;
        VKResult = vkGetPhysicalDeviceSurfaceFormatsKHR(GPU, Graphics->Surface, &FormatCount, VK_NULL_HANDLE);                        
        if(VKResult != VK_SUCCESS)
        {
            CONSOLE_LOG("Device FAILED!\n\n");
            continue;
        }
        
        
        VkSurfaceFormatKHR* SurfaceFormats = PushArray(FormatCount, VkSurfaceFormatKHR, Clear, 0);
        VKResult = vkGetPhysicalDeviceSurfaceFormatsKHR(GPU, Graphics->Surface, &FormatCount, SurfaceFormats);
        if(VKResult != VK_SUCCESS)
        {
            CONSOLE_LOG("Device FAILED!\n\n");
            continue;
        }
        
        b32 FoundSurfaceFormat = false;
        
        for(u32 SurfaceFormatIndex = 0; SurfaceFormatIndex < FormatCount; SurfaceFormatIndex++)
        {
            if(SurfaceFormats[SurfaceFormatIndex].colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)                                    
            {
                if((SurfaceFormats[SurfaceFormatIndex].format == VK_FORMAT_R8G8B8A8_SRGB) ||
                   (SurfaceFormats[SurfaceFormatIndex].format == VK_FORMAT_B8G8R8A8_SRGB))
                {
                    Result.Ptr[GPUIndex].SurfaceFormat = SurfaceFormats[SurfaceFormatIndex];
                    FoundSurfaceFormat = true;
                    break;
                }
            }
        }
        
        if(!FoundSurfaceFormat)
        {
            CONSOLE_LOG("Device FAILED!\n\n");
            continue;
        }
        
        VkSurfaceCapabilitiesKHR SurfaceCaps = {};
        VKResult = vkGetPhysicalDeviceSurfaceCapabilitiesKHR(GPU, Graphics->Surface, &SurfaceCaps);
        if(VKResult != VK_SUCCESS)
        {
            CONSOLE_LOG("Device FAILED!\n\n");
            continue;
        }               
        
        if(SurfaceCaps.maxImageCount < 2)
        {
            CONSOLE_LOG("Device FAILED!\n\n");
            continue;
        }
        
        if((SurfaceCaps.supportedCompositeAlpha & VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR) == 0)               
        {
            CONSOLE_LOG("Device FAILED!\n\n");
            continue;
        }
        
        if(SurfaceCaps.maxImageCount >= 3)
            Result.Ptr[GPUIndex].ColorImageCount = 3;
        else
            Result.Ptr[GPUIndex].ColorImageCount = 2;
        
        for(u32 FormatIndex = 0; FormatIndex < ARRAYCOUNT(Global_DepthBufferFormats); FormatIndex++)
        {
            VkFormatProperties FormatProperties;
            vkGetPhysicalDeviceFormatProperties(GPU, Global_DepthBufferFormats[FormatIndex], &FormatProperties); 
            if(FormatProperties.optimalTilingFeatures & VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT)
            {
                Result.Ptr[GPUIndex].DepthFormat = Global_DepthBufferFormats[FormatIndex];
                break;
            }
        }
        
        if(Result.Ptr[GPUIndex].DepthFormat == VK_FORMAT_UNDEFINED)
        {
            CONSOLE_LOG("Device FAILED!\n\n");
            continue;
        }
        
        vkGetPhysicalDeviceMemoryProperties(GPU, &Result.Ptr[GPUIndex].MemoryProperties);                        
        vkGetPhysicalDeviceFeatures(GPU, &Result.Ptr[GPUIndex].Features);
        
        CONSOLE_LOG("Device PASSED!\n\n");                               
    }    
    
    return Result;
    
    handle_error:
    return {};
}

VkSurfaceKHR CreateSurface(void* PlatformSurfaceInfo)
{
    VkSurfaceKHR Result = VK_NULL_HANDLE;
    
#ifdef OS_WINDOWS
    
    HWND Window = (HWND)PlatformSurfaceInfo;
    HINSTANCE Instance = (HINSTANCE)GetWindowLongPtr(Window, GWLP_HINSTANCE);
    
    VkWin32SurfaceCreateInfoKHR SurfaceInfo = {};
    SurfaceInfo.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
    SurfaceInfo.hinstance = Instance;
    SurfaceInfo.hwnd = Window;
    
    if(vkCreateWin32SurfaceKHR(GetVulkanGraphics()->Instance, &SurfaceInfo, VK_NULL_HANDLE, &Result) != VK_SUCCESS)    
        return VK_NULL_HANDLE;        
#endif
    
    return Result;
}

VkInstance CreateInstance()
{    
    VkApplicationInfo ApplicationInfo = {};
    ApplicationInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    ApplicationInfo.pApplicationName = GAME_NAME;
    ApplicationInfo.applicationVersion = VK_MAKE_VERSION(0, 1, 0);
    ApplicationInfo.pEngineName = GAME_NAME;
    ApplicationInfo.engineVersion = VK_MAKE_VERSION(0, 1, 0);
    ApplicationInfo.apiVersion = VK_API_VERSION_1_0;
    
    u32 InstancePropertyCount = 0;
    VULKAN_CHECK_AND_HANDLE(vkEnumerateInstanceExtensionProperties(VK_NULL_HANDLE, &InstancePropertyCount, VK_NULL_HANDLE),
                            "Failed to retrieve the amount of instance extensions.");
    
    VkExtensionProperties* InstanceExtensions = PushArray(InstancePropertyCount, VkExtensionProperties, Clear, 0);
    VULKAN_CHECK_AND_HANDLE(vkEnumerateInstanceExtensionProperties(VK_NULL_HANDLE, &InstancePropertyCount, InstanceExtensions),
                            "Failed to retrieve the instance extensions.");
    
    b32 FoundPlatformSurfaceExtension = false;
    
#if DEVELOPER_BUILD
    b32 FoundDebugExtensions = false;
#endif
    
    u32 InstanceExtensionCount = 0;
    char** InstanceExtensionNames = PushArray(InstancePropertyCount, char*, Clear, 0);
    for(u32 InstancePropertyIndex = 0; InstancePropertyIndex < InstancePropertyCount; InstancePropertyIndex++)
    {
#ifdef OS_WINDOWS
        if(StringEquals(InstanceExtensions[InstancePropertyIndex].extensionName, VK_KHR_WIN32_SURFACE_EXTENSION_NAME))
        {
            InstanceExtensionNames[InstanceExtensionCount++] = VK_KHR_WIN32_SURFACE_EXTENSION_NAME;
            InstanceExtensionNames[InstanceExtensionCount++] = VK_KHR_SURFACE_EXTENSION_NAME;
            FoundPlatformSurfaceExtension = true;
        }
#endif
        
#if DEVELOPER_BUILD
        if(StringEquals(InstanceExtensions[InstancePropertyIndex].extensionName, VK_EXT_DEBUG_UTILS_EXTENSION_NAME))
        {
            InstanceExtensionNames[InstanceExtensionCount++] = VK_EXT_DEBUG_UTILS_EXTENSION_NAME;                        
            FoundDebugExtensions = true;
        }                
#endif
    }
    
    u32 InstanceLayerCount = 0;
    char** InstanceLayerNames = NULL;
#if DEVELOPER_BUILD    
    
    u32 LayerPropertyCount;
    if(vkEnumerateInstanceLayerProperties(&LayerPropertyCount, VK_NULL_HANDLE) == VK_SUCCESS)
    {
        VkLayerProperties* LayerProperties = PushArray(LayerPropertyCount, VkLayerProperties, Clear, 0);
        InstanceLayerNames = PushArray(LayerPropertyCount, char*, Clear, 0);
        if(vkEnumerateInstanceLayerProperties(&LayerPropertyCount, LayerProperties) == VK_SUCCESS)
        {
            for(u32 LayerPropertyIndex = 0; LayerPropertyIndex < LayerPropertyCount; LayerPropertyIndex++)
            {
                if(StringEquals(LayerProperties[LayerPropertyIndex].layerName, "VK_LAYER_LUNARG_standard_validation"))                
                    InstanceLayerNames[InstanceLayerCount++] = LayerProperties[LayerPropertyIndex].layerName;                
                
                if(StringEquals(LayerProperties[LayerPropertyIndex].layerName, "VK_LAYER_KHRONOS_validation"))
                    InstanceLayerNames[InstanceLayerCount++] = LayerProperties[LayerPropertyIndex].layerName;
            }
        }
    }
    
#endif
    
    BOOL_CHECK_AND_HANDLE(FoundPlatformSurfaceExtension, "Failed to find the platform surface extension! Cannot initialize vulkan.");    
    
    VkInstanceCreateInfo InstanceInfo = {};
    InstanceInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    InstanceInfo.pApplicationInfo = &ApplicationInfo;
    InstanceInfo.enabledExtensionCount = InstanceExtensionCount;
    InstanceInfo.ppEnabledExtensionNames = InstanceExtensionNames;
    InstanceInfo.enabledLayerCount = InstanceLayerCount;
    InstanceInfo.ppEnabledLayerNames = InstanceLayerCount ? InstanceLayerNames : VK_NULL_HANDLE;
    
    VkInstance Instance;
    VULKAN_CHECK_AND_HANDLE(vkCreateInstance(&InstanceInfo, VK_NULL_HANDLE, &Instance),
                            "Failed to create the vulkan instance.");    
    
#if DEVELOPER_BUILD
    if(FoundDebugExtensions)
    {
        developer_vulkan_graphics* DevGraphics = (developer_vulkan_graphics*)GetVulkanGraphics();
        vkCreateDebugUtilsMessengerEXT = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(Instance, "vkCreateDebugUtilsMessengerEXT");
        
        if(vkCreateDebugUtilsMessengerEXT)
        {
            VkDebugUtilsMessengerCreateInfoEXT DebugUtilsMessengerInfo = {};
            DebugUtilsMessengerInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
            DebugUtilsMessengerInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT|VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
            DebugUtilsMessengerInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT|VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT|VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
            DebugUtilsMessengerInfo.pfnUserCallback = DebugCallback;
            vkCreateDebugUtilsMessengerEXT(Instance, &DebugUtilsMessengerInfo, VK_NULL_HANDLE, &DevGraphics->Messenger);                    
        }
    }
#endif
    
    return Instance;
    
    handle_error:
    return VK_NULL_HANDLE;
}


DEBUG_DRAW_POINT(DEBUGDrawPoint)
{
#if DEVELOPER_BUILD
    debug_primitive_context* Context = &((developer_vulkan_graphics*)GetVulkanGraphics())->PrimitiveContext;    
    ASSERT(Context->PointCount < ARRAYCOUNT(Context->DebugPoints));
    Context->DebugPoints[Context->PointCount++] = {Position, Size, Color};
#endif
}

DEBUG_DRAW_LINE(DEBUGDrawLine)
{
#if DEVELOPER_BUILD
    debug_primitive_context* Context = &((developer_vulkan_graphics*)GetVulkanGraphics())->PrimitiveContext;        
    ASSERT(Context->LineCount < ARRAYCOUNT(Context->DebugLines));
    Context->DebugLines[Context->LineCount++] = {Position0, Position1, Width, Height, Color};
#endif
}


RENDER_GAME(RenderGame)
{
    vulkan_graphics* Graphics = GetVulkanGraphics();
    
    render_buffer* RenderBuffer = &Graphics->RenderBuffer;
    if(!RenderBuffer->Swapchain)
    {
        *RenderBuffer = CreateRenderBuffer(WindowDim, VK_NULL_HANDLE);
        BOOL_CHECK_AND_HANDLE(RenderBuffer->Swapchain, "Failed to create the render buffer.");
    }
    
    u32 ImageIndex;
    VkResult ImageStatus = vkAcquireNextImageKHR(Graphics->Device, RenderBuffer->Swapchain, UINT64_MAX, Graphics->RenderLock, VK_NULL_HANDLE, &ImageIndex);
    if((ImageStatus == VK_ERROR_OUT_OF_DATE_KHR) || (ImageStatus == VK_SUBOPTIMAL_KHR) || (WindowDim != RenderBuffer->Dimensions))
    {
        vkDeviceWaitIdle(Graphics->Device);
        vkDestroySemaphore(Graphics->Device, Graphics->RenderLock, VK_NULL_HANDLE);        
        Graphics->RenderLock = CreateSemaphore();
        BOOL_CHECK_AND_HANDLE(Graphics->RenderLock, "Failed to create the render lock.");
        
        VkSwapchainKHR OldSwapchain = (ImageStatus != VK_SUCCESS) ? RenderBuffer->Swapchain : VK_NULL_HANDLE; 
        *RenderBuffer = CreateRenderBuffer(WindowDim, OldSwapchain);
        BOOL_CHECK_AND_HANDLE(RenderBuffer->Swapchain, "Failed to create the render buffer.");
    }
    else
    {
        BOOL_CHECK_AND_HANDLE(ImageStatus == VK_SUCCESS, "Error acquiring image to present.");
    }
    
    Graphics->CameraBufferData[0] = PerspectiveM4(PI*0.30f, SafeRatio(WindowDim.width, WindowDim.height), 0.01f, 1000.0f);
    
    camera* TargetCamera = &Game->Camera;
    
#if DEVELOPER_BUILD
    development_game* DevGame = (development_game*)Game;
    if(DevGame->InDevelopmentMode)
    {
        TargetCamera = &DevGame->DevCamera;
    }    
#endif
    
    Graphics->CameraBufferData[1] = InverseTransformM4(TargetCamera->Position, TargetCamera->Orientation);    
    
    VULKAN_CHECK_AND_HANDLE(vkQueueWaitIdle(Graphics->GraphicsQueue), "Failed to wait for the graphics queue.");        
    VULKAN_CHECK_AND_HANDLE(vkResetCommandPool(Graphics->Device, Graphics->CommandPool, 0), "Failed to reset the command pool.");    
    Reset(&Graphics->UploadBuffer);
    
    VkCommandBuffer CommandBuffer = Graphics->CommandBuffer;
    
    VkCommandBufferBeginInfo CommandBufferBeginInfo = {};
    CommandBufferBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    CommandBufferBeginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    VULKAN_CHECK_AND_HANDLE(vkBeginCommandBuffer(CommandBuffer, OneTimeCommandBufferBeginInfo()),
                            "Failed to begin the command buffer recording.");
    {
        VkClearColorValue ClearColor = {0.0f, 0.0f, 0.0f, 1.0f};
        VkClearDepthStencilValue ClearDepth = {1.0f, 0};
        
        VkClearValue ClearValues[2] = {};                
        ClearValues[0].color = ClearColor;
        ClearValues[1].depthStencil = ClearDepth;
        
        VkRenderPassBeginInfo RenderPassBeginInfo = {};
        RenderPassBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        RenderPassBeginInfo.renderPass = Graphics->RenderPass;
        RenderPassBeginInfo.framebuffer = RenderBuffer->Framebuffers[ImageIndex];
        RenderPassBeginInfo.renderArea = {{0, 0}, {(u32)WindowDim.width, (u32)WindowDim.height}};
        RenderPassBeginInfo.clearValueCount = ARRAYCOUNT(ClearValues);
        RenderPassBeginInfo.pClearValues = ClearValues;
        
        VkViewport VKViewport = {0.0f, 0.0f, (f32)WindowDim.width, (f32)WindowDim.height, 0.0f, 1.0f};
        
        vkCmdSetViewport(CommandBuffer, 0, 1, &VKViewport);
        vkCmdSetScissor(CommandBuffer, 0, 1, &RenderPassBeginInfo.renderArea);                    
        
        vkCmdBeginRenderPass(CommandBuffer, &RenderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);
        {            
            vkCmdBindPipeline(CommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, Graphics->Pipeline);             
            vkCmdBindDescriptorSets(CommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, Graphics->PipelineLayout, 0, 1, &Graphics->DescriptorSet, 0, VK_NULL_HANDLE);
            
            VkDeviceSize VertexBufferOffset = 0;
            vkCmdBindVertexBuffers(CommandBuffer, 0, 1, &Graphics->VertexBuffer, &VertexBufferOffset);
            vkCmdBindIndexBuffer(CommandBuffer, Graphics->IndexBuffer, 0, VK_INDEX_TYPE_UINT16);
            
            world* World = GetCurrentWorld(Game);
            
            player* Player = &World->Player;
            v3f PlayerZ = V3(0.0f, 0.0f, 1.0f);
            v3f PlayerY = V3(Player->FacingDirection, 0.0f);
            v3f PlayerX = Cross(PlayerY, PlayerZ);
            
            for(entity* Entity = World->Entities.First; Entity; Entity = Entity->Next)
            {                
                m4 Model = TransformM4(Entity->Transform);
                vkCmdPushConstants(CommandBuffer, Graphics->PipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(m4), &Model);                
                vkCmdPushConstants(CommandBuffer, Graphics->PipelineLayout, VK_SHADER_STAGE_FRAGMENT_BIT, sizeof(m4), sizeof(c4), &Entity->Color);                            
                vkCmdDrawIndexed(CommandBuffer, 36, 1, 0, 0, 0);                
            }
            
#if DEVELOPER_BUILD
            developer_vulkan_graphics* DevGraphics = (developer_vulkan_graphics*)Graphics;
            
            debug_volume_context* VolumeContext = &DevGraphics->VolumeContext;
            vkCmdBindPipeline(CommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, VolumeContext->Pipeline);            
            vkCmdBindDescriptorSets(CommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, VolumeContext->PipelineLayout, 0, 1, &VolumeContext->DescriptorSet, 0, VK_NULL_HANDLE);
            
            VkDeviceSize VolumeVertexOffset = 0;
            vkCmdBindVertexBuffers(CommandBuffer, 0, 1, &VolumeContext->VertexBuffer, &VolumeVertexOffset);
            vkCmdBindIndexBuffer(CommandBuffer, VolumeContext->IndexBuffer, 0, VK_INDEX_TYPE_UINT16);            
            
            
            debug_capsule_mesh* CapsuleMesh = &VolumeContext->CapsuleMesh;                
            debug_graphics_mesh* CapsuleCap = &CapsuleMesh->Cap;
            debug_graphics_mesh* CapsuleBody = &CapsuleMesh->Body;
            {                
                vkCmdPushConstants(CommandBuffer, VolumeContext->PipelineLayout, VK_SHADER_STAGE_FRAGMENT_BIT, sizeof(m4), sizeof(c4), &Player->Color);
                
                v3f BodyZ = PlayerZ*Player->Height;
                
                v3f XAxis = PlayerX*Player->Radius;
                v3f YAxis = PlayerY*Player->Radius;
                v3f ZAxis = PlayerZ*Player->Radius;
                
                v3f BottomPosition = Player->Position+ZAxis;
                m4 Model = TransformM4(BottomPosition, M3(XAxis, YAxis, -ZAxis));                
                vkCmdPushConstants(CommandBuffer, VolumeContext->PipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(m4), &Model);                
                vkCmdDrawIndexed(CommandBuffer, CapsuleCap->Indices.Count, 1, 0, 0, 0); 
                
                Model = TransformM4(BottomPosition + BodyZ, 
                                    M3(XAxis, YAxis, ZAxis));
                vkCmdPushConstants(CommandBuffer, VolumeContext->PipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(m4), &Model);
                vkCmdDrawIndexed(CommandBuffer, CapsuleCap->Indices.Count, 1, 0, 0, 0);
                
                Model = TransformM4(BottomPosition+(BodyZ*0.5f), M3(XAxis, YAxis, BodyZ));
                vkCmdPushConstants(CommandBuffer, VolumeContext->PipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(m4), &Model);
                vkCmdDrawIndexed(CommandBuffer, CapsuleBody->Indices.Count, 1, CapsuleCap->Indices.Count, CapsuleCap->Vertices.Count, 0);                
            }
            
            if(DevGame->TurnOnVolumeOutline)
            {
                debug_graphics_mesh* BoxMesh = &VolumeContext->BoxMesh;
                
                u32 DEBUGBoxVertexOffset = CapsuleCap->Vertices.Count+CapsuleBody->Vertices.Count;
                u32 DEBUGBoxIndexOffset = CapsuleCap->Indices.Count+CapsuleBody->Indices.Count;
                for(entity* Entity = World->Entities.First; Entity; Entity = Entity->Next)
                {                   
                    aabb3D AABB = TransformAABB3D(Entity->AABB, Entity->Transform);
                    
                    v3f Dim = GetAABB3DDim(AABB);                    
                    v3f Position = V3(AABB.Min.xy + Dim.xy*0.5f, AABB.Min.z);
                    
                    m4 Model = TransformM4(Position, Dim);                    
                    vkCmdPushConstants(CommandBuffer, VolumeContext->PipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(m4), &Model);                
                    vkCmdPushConstants(CommandBuffer, VolumeContext->PipelineLayout, VK_SHADER_STAGE_FRAGMENT_BIT, sizeof(m4), sizeof(c4), &Global_Blue);                                                
                    vkCmdDrawIndexed(CommandBuffer, BoxMesh->Indices.Count, 1, DEBUGBoxIndexOffset, DEBUGBoxVertexOffset, 0);                                
                }
            }
            
            debug_primitive_context* PrimitiveContext = &DevGraphics->PrimitiveContext;
            
            vkCmdBindPipeline(CommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, PrimitiveContext->LinePipeline);
            vkCmdBindDescriptorSets(CommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, PrimitiveContext->LinePipelineLayout, 0, 1, &PrimitiveContext->DescriptorSet, 0, VK_NULL_HANDLE);
            
            for(u32 LineIndex = 0; LineIndex < PrimitiveContext->LineCount; LineIndex++)
            {
                debug_line* DebugLine = PrimitiveContext->DebugLines + LineIndex;
                
                v4f LinePositions[2];
                LinePositions[0] = V4(DebugLine->Position0, DebugLine->Width);
                LinePositions[1] = V4(DebugLine->Position1, DebugLine->Height);                
                vkCmdPushConstants(CommandBuffer, PrimitiveContext->LinePipelineLayout, VK_SHADER_STAGE_GEOMETRY_BIT, 0, sizeof(v4f)*2, LinePositions);
                vkCmdPushConstants(CommandBuffer, PrimitiveContext->LinePipelineLayout, VK_SHADER_STAGE_FRAGMENT_BIT, sizeof(v4f)*2, sizeof(c4), &DebugLine->Color);
                vkCmdDraw(CommandBuffer, 1, 1, 0, 0);            
            }
            PrimitiveContext->LineCount = 0;
            
            vkCmdBindPipeline(CommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, PrimitiveContext->PointPipeline);
            vkCmdBindDescriptorSets(CommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, PrimitiveContext->PointPipelineLayout, 0, 1, &PrimitiveContext->DescriptorSet, 0, VK_NULL_HANDLE);
            
            for(u32 PointIndex = 0; PointIndex < PrimitiveContext->PointCount; PointIndex++)
            {   
                debug_point* DebugPoint = PrimitiveContext->DebugPoints + PointIndex;                
                v4f PointPosition = V4(DebugPoint->Position, DebugPoint->Size); 
               
                vkCmdPushConstants(CommandBuffer, PrimitiveContext->PointPipelineLayout, VK_SHADER_STAGE_GEOMETRY_BIT, 0, sizeof(v4f), &PointPosition);
                vkCmdPushConstants(CommandBuffer, PrimitiveContext->PointPipelineLayout, VK_SHADER_STAGE_FRAGMENT_BIT, sizeof(v4f), sizeof(c4), &DebugPoint->Color);
                vkCmdDraw(CommandBuffer, 1, 1, 0, 0);
            }
            PrimitiveContext->PointCount = 0;            
            
            debug_imgui_context* ImGuiContext = &DevGraphics->ImGuiContext;            
            ImDrawData* ImGuiData = ImGui::GetDrawData();
            
            ptr VertexSize = ImGuiData->TotalVtxCount * sizeof(ImDrawVert);
            ptr IndexSize = ImGuiData->TotalIdxCount * sizeof(ImDrawIdx);
            
            if(VertexSize && IndexSize)
            {                
                if(ImGuiContext->VertexBufferSize < VertexSize)
                {
                    if(ImGuiContext->VertexBuffer)
                        vkDestroyBuffer(DevGraphics->Device, ImGuiContext->VertexBuffer, VK_NULL_HANDLE);
                    
                    if(ImGuiContext->VertexBufferMemory)
                        vkFreeMemory(DevGraphics->Device, ImGuiContext->VertexBufferMemory, VK_NULL_HANDLE);
                    
                    VkBufferCreateInfo BufferInfo = GetBufferInfo(VertexSize, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT);
                    VULKAN_CHECK_AND_HANDLE(vkCreateBuffer(DevGraphics->Device, &BufferInfo, VK_NULL_HANDLE, &ImGuiContext->VertexBuffer), 
                                            "Failed to create the imgui vertex buffer.");
                    
                    ImGuiContext->VertexBufferMemory = AllocateAndBindMemory(ImGuiContext->VertexBuffer, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);
                    BOOL_CHECK_AND_HANDLE(ImGuiContext->VertexBufferMemory, "Failed to allocate the ImGui vertex buffer memory.");
                    
                    ImGuiContext->VertexBufferSize = VertexSize;
                }
                
                if(ImGuiContext->IndexBufferSize < IndexSize)
                {
                    if(ImGuiContext->IndexBuffer)
                        vkDestroyBuffer(DevGraphics->Device, ImGuiContext->IndexBuffer, VK_NULL_HANDLE);
                    
                    if(ImGuiContext->IndexBufferMemory)
                        vkFreeMemory(DevGraphics->Device, ImGuiContext->IndexBufferMemory, VK_NULL_HANDLE);
                    
                    VkBufferCreateInfo BufferInfo = GetBufferInfo(IndexSize, VK_BUFFER_USAGE_INDEX_BUFFER_BIT);
                    VULKAN_CHECK_AND_HANDLE(vkCreateBuffer(DevGraphics->Device, &BufferInfo, VK_NULL_HANDLE, &ImGuiContext->IndexBuffer),
                                            "Failed to create the imgui index buffer.");
                    
                    ImGuiContext->IndexBufferMemory = AllocateAndBindMemory(ImGuiContext->IndexBuffer, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);                    
                    BOOL_CHECK_AND_HANDLE(ImGuiContext->IndexBufferMemory, "Failed to allocate the ImGui index buffer memory.");
                    
                    ImGuiContext->IndexBufferSize = IndexSize;
                }
                
                u8* ImGuiVertexBufferData;
                u8* ImGuiIndexBufferData;
                VULKAN_CHECK_AND_HANDLE(vkMapMemory(Graphics->Device, ImGuiContext->VertexBufferMemory, 0, ImGuiContext->VertexBufferSize, 0, (void**)(&ImGuiVertexBufferData)),
                                        "Failed to map the imgui vertex buffer memory to the cpu.");
                VULKAN_CHECK_AND_HANDLE(vkMapMemory(Graphics->Device, ImGuiContext->IndexBufferMemory, 0, ImGuiContext->IndexBufferSize, 0, (void**)(&ImGuiIndexBufferData)),
                                        "Failed to map the imgui index buffer memory to the cpu.");                
                
                VkDeviceSize VertexOffset = 0;
                VkDeviceSize IndexOffset = 0;
                for(i32 CmdListIndex = 0; CmdListIndex < ImGuiData->CmdListsCount; CmdListIndex++)
                {
                    ImDrawList* CmdList = ImGuiData->CmdLists[CmdListIndex];
                    
                    ptr CmdVertexSize = CmdList->VtxBuffer.Size * sizeof(ImDrawVert);
                    ptr CmdIndexSize = CmdList->IdxBuffer.Size * sizeof(ImDrawIdx);
                    
                    CopyMemory(ImGuiVertexBufferData+VertexOffset, CmdList->VtxBuffer.Data, CmdVertexSize);
                    CopyMemory(ImGuiIndexBufferData+IndexOffset, CmdList->IdxBuffer.Data, CmdIndexSize);
                    
                    VertexOffset += CmdVertexSize;
                    IndexOffset += CmdIndexSize;
                }
                
                vkUnmapMemory(Graphics->Device, ImGuiContext->VertexBufferMemory);
                vkUnmapMemory(Graphics->Device, ImGuiContext->IndexBufferMemory);
                
                ASSERT(VertexOffset == VertexSize);
                ASSERT(IndexOffset == IndexSize);
                
                vkCmdBindPipeline(CommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, ImGuiContext->Pipeline);
                vkCmdBindDescriptorSets(CommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, ImGuiContext->PipelineLayout, 0, 1, &ImGuiContext->DescriptorSet, 0, NULL);
                
                VkDeviceSize ImguiVertexOffset = 0;
                vkCmdBindVertexBuffers(CommandBuffer, 0, 1, &ImGuiContext->VertexBuffer, &ImguiVertexOffset);
                vkCmdBindIndexBuffer(CommandBuffer, ImGuiContext->IndexBuffer, 0, sizeof(ImDrawIdx) == 2 ? VK_INDEX_TYPE_UINT16 : VK_INDEX_TYPE_UINT16);
                
                v2f Scale = 2.0f/ImGuiData->DisplaySize;
                v2f Translate = -1.0f - (v2f(ImGuiData->DisplayPos)*Scale);
                vkCmdPushConstants(CommandBuffer, ImGuiContext->PipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(v2f), &Scale);
                vkCmdPushConstants(CommandBuffer, ImGuiContext->PipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, sizeof(v2f), sizeof(v2f), &Translate);
                
                i32 GlobalVertexOffset = 0;
                u32 GlobalIndexOffset = 0;
                for(i32 CmdListIndex = 0; CmdListIndex < ImGuiData->CmdListsCount; CmdListIndex++)
                {
                    ImDrawList* CmdList = ImGuiData->CmdLists[CmdListIndex];
                    for(i32 CmdIndex = 0; CmdIndex < CmdList->CmdBuffer.Size; CmdIndex++)
                    {
                        ImDrawCmd* Cmd =&CmdList->CmdBuffer[CmdIndex];
                        ASSERT(!Cmd->UserCallback);
                        
                        v4f ClipRect = V4((Cmd->ClipRect.x - ImGuiData->DisplayPos.x) * ImGuiData->FramebufferScale.x,                        
                                          (Cmd->ClipRect.y - ImGuiData->DisplayPos.y) * ImGuiData->FramebufferScale.y,
                                          (Cmd->ClipRect.z - ImGuiData->DisplayPos.x) * ImGuiData->FramebufferScale.x,
                                          (Cmd->ClipRect.w - ImGuiData->DisplayPos.y) * ImGuiData->FramebufferScale.y);
                        
                        if((ClipRect.x < WindowDim.x) && (ClipRect.y < WindowDim.y) && 
                           (ClipRect.z >= 0.0f) && (ClipRect.w >= 0.0f))
                        {
                            if(ClipRect.x < 0.0f) ClipRect.x = 0.0f;
                            if(ClipRect.y < 0.0f) ClipRect.y = 0.0f;
                            
                            VkRect2D ScissorRect;
                            ScissorRect.offset.x = (i32)(ClipRect.x);
                            ScissorRect.offset.y = (i32)(ClipRect.y);
                            ScissorRect.extent.width = (u32)(ClipRect.z - ClipRect.x);
                            ScissorRect.extent.height = (u32)(ClipRect.w - ClipRect.y);
                            vkCmdSetScissor(CommandBuffer, 0, 1, &ScissorRect);
                            vkCmdDrawIndexed(CommandBuffer, Cmd->ElemCount, 1, Cmd->IdxOffset + GlobalIndexOffset, Cmd->VtxOffset + GlobalVertexOffset, 0);
                        }                        
                    }
                    
                    GlobalVertexOffset += CmdList->VtxBuffer.Size;
                    GlobalIndexOffset += CmdList->IdxBuffer.Size;
                }
            }            
#endif            
        }
        vkCmdEndRenderPass(CommandBuffer);
        
    }
    VULKAN_CHECK_AND_HANDLE(vkEndCommandBuffer(CommandBuffer), "Failed to end the command buffer recording.");
    
    VkPipelineStageFlags WaitStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    
    VkSubmitInfo SubmitInfo[1] = {};
    SubmitInfo[0].sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    SubmitInfo[0].waitSemaphoreCount = 1;
    SubmitInfo[0].pWaitSemaphores = &Graphics->RenderLock;
    SubmitInfo[0].pWaitDstStageMask = &WaitStage;
    SubmitInfo[0].commandBufferCount = 1;
    SubmitInfo[0].pCommandBuffers = &CommandBuffer;
    SubmitInfo[0].signalSemaphoreCount = 1;
    SubmitInfo[0].pSignalSemaphores = &Graphics->PresentLock;    
    
    VULKAN_CHECK_AND_HANDLE(vkQueueSubmit(Graphics->GraphicsQueue, ARRAYCOUNT(SubmitInfo), SubmitInfo, VK_NULL_HANDLE),
                            "Failed to submit the command buffer to the graphics queue.");
    
    VkPresentInfoKHR PresentInfo = {};
    PresentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    PresentInfo.waitSemaphoreCount = 1;
    PresentInfo.pWaitSemaphores = &Graphics->PresentLock;
    PresentInfo.swapchainCount = 1;
    PresentInfo.pSwapchains = &RenderBuffer->Swapchain;
    PresentInfo.pImageIndices = &ImageIndex;
    
    VULKAN_CHECK_AND_HANDLE(vkQueuePresentKHR(Graphics->PresentQueue, &PresentInfo),
                            "Failed to submit the vulkan image for presentation.");
    
    return true;
    
    handle_error:
    return false;    
}

b32 VulkanInit(void* PlatformSurfaceInfo, assets* Assets)
{
    vulkan_graphics* Graphics = GetVulkanGraphics();
    Graphics->RenderGame = RenderGame;    
    Graphics->DEBUGDrawLine = DEBUGDrawLine;
    Graphics->DEBUGDrawPoint = DEBUGDrawPoint;
    Graphics->Storage = CreateArena(KILOBYTE(32));    
    
    LOAD_GLOBAL_FUNCTION(vkCreateInstance);
    LOAD_GLOBAL_FUNCTION(vkEnumerateInstanceExtensionProperties);
    LOAD_GLOBAL_FUNCTION(vkEnumerateInstanceLayerProperties);
    
    Graphics->Instance = CreateInstance();
    BOOL_CHECK_AND_HANDLE(Graphics->Instance, "Failed to create the vulkan instance.");
    
#ifdef OS_WINDOWS
    LOAD_INSTANCE_FUNCTION(vkCreateWin32SurfaceKHR);
#endif
    
    LOAD_INSTANCE_FUNCTION(vkGetPhysicalDeviceSurfaceSupportKHR);
    LOAD_INSTANCE_FUNCTION(vkGetPhysicalDeviceSurfaceFormatsKHR);
    LOAD_INSTANCE_FUNCTION(vkGetPhysicalDeviceSurfaceCapabilitiesKHR);
    LOAD_INSTANCE_FUNCTION(vkEnumeratePhysicalDevices);    
    LOAD_INSTANCE_FUNCTION(vkGetPhysicalDeviceProperties);
    LOAD_INSTANCE_FUNCTION(vkGetPhysicalDeviceQueueFamilyProperties);   
    LOAD_INSTANCE_FUNCTION(vkEnumerateDeviceExtensionProperties);
    LOAD_INSTANCE_FUNCTION(vkCreateDevice);
    LOAD_INSTANCE_FUNCTION(vkGetDeviceProcAddr);
    LOAD_INSTANCE_FUNCTION(vkGetPhysicalDeviceFormatProperties);
    LOAD_INSTANCE_FUNCTION(vkGetPhysicalDeviceMemoryProperties);
    LOAD_INSTANCE_FUNCTION(vkGetPhysicalDeviceFeatures);
    
    Graphics->Surface = CreateSurface(PlatformSurfaceInfo);
    BOOL_CHECK_AND_HANDLE(Graphics->Surface, "Failed to create the vulkan surface.");
    
    Graphics->GPUs = CreatePhysicalDevices();
    BOOL_CHECK_AND_HANDLE(Graphics->GPUs.Ptr, "Failed to enumerate the vulkan gpus.");
    
    Graphics->SelectedGPU = GetFirstCompatbileGPU(&Graphics->GPUs);
    BOOL_CHECK_AND_HANDLE(Graphics->SelectedGPU, "Failed to retrieve a compatible vulkan gpu.");
    
    Graphics->Device = CreateLogicalDevice(Graphics->SelectedGPU);
    BOOL_CHECK_AND_HANDLE(Graphics->Device, "Failed to create the vulkan logical device.");
    
    LOAD_DEVICE_FUNCTION(vkGetDeviceQueue);
    LOAD_DEVICE_FUNCTION(vkCreateCommandPool);
    LOAD_DEVICE_FUNCTION(vkResetCommandPool);
    LOAD_DEVICE_FUNCTION(vkAllocateCommandBuffers);
    LOAD_DEVICE_FUNCTION(vkCreateRenderPass);
    LOAD_DEVICE_FUNCTION(vkAcquireNextImageKHR);
    LOAD_DEVICE_FUNCTION(vkQueueWaitIdle);
    LOAD_DEVICE_FUNCTION(vkDeviceWaitIdle);
    LOAD_DEVICE_FUNCTION(vkCreateSemaphore);
    LOAD_DEVICE_FUNCTION(vkDestroySemaphore);
    LOAD_DEVICE_FUNCTION(vkCreateSwapchainKHR);
    LOAD_DEVICE_FUNCTION(vkDestroySwapchainKHR);
    LOAD_DEVICE_FUNCTION(vkGetSwapchainImagesKHR);
    LOAD_DEVICE_FUNCTION(vkDestroyImageView);
    LOAD_DEVICE_FUNCTION(vkDestroyFramebuffer);
    LOAD_DEVICE_FUNCTION(vkCreateImageView);
    LOAD_DEVICE_FUNCTION(vkCreateFramebuffer);
    LOAD_DEVICE_FUNCTION(vkBeginCommandBuffer);
    LOAD_DEVICE_FUNCTION(vkCmdBeginRenderPass);
    LOAD_DEVICE_FUNCTION(vkCmdEndRenderPass);
    LOAD_DEVICE_FUNCTION(vkEndCommandBuffer);
    LOAD_DEVICE_FUNCTION(vkQueueSubmit);
    LOAD_DEVICE_FUNCTION(vkQueuePresentKHR);
    LOAD_DEVICE_FUNCTION(vkCreateShaderModule);
    LOAD_DEVICE_FUNCTION(vkDestroyShaderModule);
    LOAD_DEVICE_FUNCTION(vkCreateGraphicsPipelines);
    LOAD_DEVICE_FUNCTION(vkCreatePipelineLayout);
    LOAD_DEVICE_FUNCTION(vkCmdSetViewport);
    LOAD_DEVICE_FUNCTION(vkCmdSetScissor);
    LOAD_DEVICE_FUNCTION(vkCmdBindPipeline);
    LOAD_DEVICE_FUNCTION(vkCmdDraw);
    LOAD_DEVICE_FUNCTION(vkCmdDrawIndexed);
    LOAD_DEVICE_FUNCTION(vkCreateImage);
    LOAD_DEVICE_FUNCTION(vkDestroyImageView);
    LOAD_DEVICE_FUNCTION(vkGetImageMemoryRequirements);
    LOAD_DEVICE_FUNCTION(vkGetBufferMemoryRequirements);
    LOAD_DEVICE_FUNCTION(vkAllocateMemory);
    LOAD_DEVICE_FUNCTION(vkFreeMemory);
    LOAD_DEVICE_FUNCTION(vkBindImageMemory);
    LOAD_DEVICE_FUNCTION(vkBindBufferMemory);    
    LOAD_DEVICE_FUNCTION(vkCreateDescriptorSetLayout);
    LOAD_DEVICE_FUNCTION(vkCmdPushConstants);
    LOAD_DEVICE_FUNCTION(vkCreateDescriptorPool);
    LOAD_DEVICE_FUNCTION(vkAllocateDescriptorSets);
    LOAD_DEVICE_FUNCTION(vkCmdBindDescriptorSets);
    LOAD_DEVICE_FUNCTION(vkUpdateDescriptorSets);
    LOAD_DEVICE_FUNCTION(vkCreateBuffer);
    LOAD_DEVICE_FUNCTION(vkMapMemory);
    LOAD_DEVICE_FUNCTION(vkUnmapMemory);
    LOAD_DEVICE_FUNCTION(vkCmdUpdateBuffer);
    LOAD_DEVICE_FUNCTION(vkDestroyBuffer);
    LOAD_DEVICE_FUNCTION(vkCmdBindVertexBuffers);
    LOAD_DEVICE_FUNCTION(vkCmdBindIndexBuffer);
    LOAD_DEVICE_FUNCTION(vkCreateSampler);        
    LOAD_DEVICE_FUNCTION(vkCmdPipelineBarrier);
    LOAD_DEVICE_FUNCTION(vkCmdCopyBufferToImage);
    
    if(Graphics->SelectedGPU->FoundDedicatedMemoryExtension)
    {
        LOAD_DEVICE_FUNCTION(vkGetImageMemoryRequirements2KHR);
        LOAD_DEVICE_FUNCTION(vkGetBufferMemoryRequirements2KHR);
    }
    
    vkGetDeviceQueue(Graphics->Device, Graphics->SelectedGPU->GraphicsFamilyIndex, 0, &Graphics->GraphicsQueue);
    Graphics->PresentQueue = Graphics->GraphicsQueue;    
    if(Graphics->SelectedGPU->GraphicsFamilyIndex != Graphics->SelectedGPU->PresentFamilyIndex)
        vkGetDeviceQueue(Graphics->Device, Graphics->SelectedGPU->PresentFamilyIndex, 0, &Graphics->PresentQueue);    
    
    VkCommandPoolCreateInfo CommandPoolInfo = {};
    CommandPoolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    CommandPoolInfo.queueFamilyIndex = Graphics->SelectedGPU->GraphicsFamilyIndex; 
    
    VULKAN_CHECK_AND_HANDLE(vkCreateCommandPool(Graphics->Device, &CommandPoolInfo, VK_NULL_HANDLE, &Graphics->CommandPool),
                            "Failed to create the command pool.");
    
    VkCommandBufferAllocateInfo CommandBufferInfo = {};
    CommandBufferInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    CommandBufferInfo.commandPool = Graphics->CommandPool;
    CommandBufferInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    CommandBufferInfo.commandBufferCount = 1;
    
    VULKAN_CHECK_AND_HANDLE(vkAllocateCommandBuffers(Graphics->Device, &CommandBufferInfo, &Graphics->CommandBuffer),
                            "Failed to allocate the command buffers.");        
    
    Graphics->UploadBuffer = CreateUploadBuffer(MEGABYTE(32));
    BOOL_CHECK_AND_HANDLE(Graphics->UploadBuffer.Buffer, "Failed to create the graphics upload buffer.");
    
    VkDeviceSize VertexBufferSize = GetVerticesSizeInBytes(&Assets->BoxGraphicsMesh);
    VkDeviceSize IndexBufferSize = GetIndicesSizeInBytes(&Assets->BoxGraphicsMesh);
    VkBufferCreateInfo VertexBufferInfo = GetBufferInfo(VertexBufferSize, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT|VK_BUFFER_USAGE_TRANSFER_DST_BIT);
    VkBufferCreateInfo IndexBufferInfo = GetBufferInfo(IndexBufferSize, VK_BUFFER_USAGE_INDEX_BUFFER_BIT|VK_BUFFER_USAGE_TRANSFER_DST_BIT);
    
    VULKAN_CHECK_AND_HANDLE(vkCreateBuffer(Graphics->Device, &VertexBufferInfo, VK_NULL_HANDLE, &Graphics->VertexBuffer),
                            "Failed to create the graphics vertex buffer.");
    
    VULKAN_CHECK_AND_HANDLE(vkCreateBuffer(Graphics->Device, &IndexBufferInfo, VK_NULL_HANDLE, &Graphics->IndexBuffer),
                            "Failed to create the graphics index buffer.");
    
    Graphics->VertexBufferMemory = AllocateAndBindMemory(Graphics->VertexBuffer, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);
    BOOL_CHECK_AND_HANDLE(Graphics->VertexBufferMemory, "Failed to allocate and bind the vertex buffer memory.");
    Graphics->IndexBufferMemory = AllocateAndBindMemory(Graphics->IndexBuffer, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);
    BOOL_CHECK_AND_HANDLE(Graphics->IndexBufferMemory, "Failed to allocate and bind the index buffer memory.");
    
    VkAttachmentDescription AttachmentDescriptions[2] = {};    
    AttachmentDescriptions[0].format = Graphics->SelectedGPU->SurfaceFormat.format;
    AttachmentDescriptions[0].samples = VK_SAMPLE_COUNT_1_BIT;
    AttachmentDescriptions[0].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    AttachmentDescriptions[0].storeOp = VK_ATTACHMENT_STORE_OP_STORE;        
    AttachmentDescriptions[0].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    AttachmentDescriptions[0].finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;    
    
    AttachmentDescriptions[1].format = Graphics->SelectedGPU->DepthFormat;
    AttachmentDescriptions[1].samples = VK_SAMPLE_COUNT_1_BIT;
    AttachmentDescriptions[1].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    AttachmentDescriptions[1].storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    AttachmentDescriptions[1].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    AttachmentDescriptions[1].finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
    
    VkAttachmentReference ColorAttachmentRef = {};
    ColorAttachmentRef.attachment = 0;
    ColorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    
    VkAttachmentReference DepthAttachmentRef = {};
    DepthAttachmentRef.attachment = 1;
    DepthAttachmentRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
    
    VkSubpassDescription SubpassDescriptions[1] = {};
    SubpassDescriptions[0].pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    SubpassDescriptions[0].colorAttachmentCount = 1;
    SubpassDescriptions[0].pColorAttachments = &ColorAttachmentRef;
    SubpassDescriptions[0].pDepthStencilAttachment = &DepthAttachmentRef;
    
    VkRenderPassCreateInfo RenderPassInfo = {};
    RenderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    RenderPassInfo.attachmentCount = ARRAYCOUNT(AttachmentDescriptions);
    RenderPassInfo.pAttachments = AttachmentDescriptions;
    RenderPassInfo.subpassCount = ARRAYCOUNT(SubpassDescriptions);
    RenderPassInfo.pSubpasses = SubpassDescriptions;
    
    VULKAN_CHECK_AND_HANDLE(vkCreateRenderPass(Graphics->Device, &RenderPassInfo, VK_NULL_HANDLE, &Graphics->RenderPass),
                            "Failed to create the render pass.");
    
    Graphics->RenderLock = CreateSemaphore();
    Graphics->PresentLock = CreateSemaphore();
    
    BOOL_CHECK_AND_HANDLE(Graphics->RenderLock, "Failed to create the render lock.");
    BOOL_CHECK_AND_HANDLE(Graphics->PresentLock, "Failed to create the present lock.");
    
    VkDescriptorPoolSize PoolSizes[2] = {};
    PoolSizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    PoolSizes[0].descriptorCount = 3;
    
    PoolSizes[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    PoolSizes[1].descriptorCount = 1;
    
    VkDescriptorPoolCreateInfo DescriptorPoolInfo = {};
    DescriptorPoolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    DescriptorPoolInfo.maxSets = 4;
    DescriptorPoolInfo.poolSizeCount = ARRAYCOUNT(PoolSizes);
    DescriptorPoolInfo.pPoolSizes = PoolSizes;
    
    VULKAN_CHECK_AND_HANDLE(vkCreateDescriptorPool(Graphics->Device, &DescriptorPoolInfo, VK_NULL_HANDLE, &Graphics->DescriptorPool),
                            "Failed to create the descriptor pool.");
        
    VkShaderModule OpaqueVertex   = CreateShader("shaders/vulkan/opaque_shading_vertex.spv");
    VkShaderModule OpaqueFragment = CreateShader("shaders/vulkan/opaque_shading_fragment.spv");
    
    BOOL_CHECK_AND_HANDLE(OpaqueVertex, "Failed to create the opaque vertex shader.");
    BOOL_CHECK_AND_HANDLE(OpaqueFragment, "Failed to create the opaque fragment shader.");
    
    VkDescriptorSetLayoutBinding DescriptorBinding = {};
    DescriptorBinding.binding = 0;
    DescriptorBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    DescriptorBinding.descriptorCount = 1;
    DescriptorBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
    
    VkDescriptorSetLayoutCreateInfo DescriptorSetLayoutInfo = {};
    DescriptorSetLayoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    DescriptorSetLayoutInfo.bindingCount = 1;
    DescriptorSetLayoutInfo.pBindings = &DescriptorBinding;
    
    VULKAN_CHECK_AND_HANDLE(vkCreateDescriptorSetLayout(Graphics->Device, &DescriptorSetLayoutInfo, VK_NULL_HANDLE, &Graphics->DescriptorSetLayout),
                            "Failed to create the descriptor set layout.");
    
    VkPushConstantRange PushConstantRanges[2] = {};
    PushConstantRanges[0].stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
    PushConstantRanges[0].offset = 0;
    PushConstantRanges[0].size = sizeof(m4);
    
    PushConstantRanges[1].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
    PushConstantRanges[1].offset = sizeof(m4);
    PushConstantRanges[1].size = sizeof(c4);
    
    VkPipelineLayoutCreateInfo PipelineLayoutInfo = {};
    PipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;    
    PipelineLayoutInfo.setLayoutCount = 1;
    PipelineLayoutInfo.pSetLayouts = &Graphics->DescriptorSetLayout;
    PipelineLayoutInfo.pushConstantRangeCount = ARRAYCOUNT(PushConstantRanges);
    PipelineLayoutInfo.pPushConstantRanges = PushConstantRanges;
    
    VULKAN_CHECK_AND_HANDLE(vkCreatePipelineLayout(Graphics->Device, &PipelineLayoutInfo, VK_NULL_HANDLE, &Graphics->PipelineLayout),
                            "Failed to create the pipeline layout.");
    
    VkDescriptorSetAllocateInfo DescriptorSetInfo = {};
    DescriptorSetInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    DescriptorSetInfo.descriptorPool = Graphics->DescriptorPool;
    DescriptorSetInfo.descriptorSetCount = 1;
    DescriptorSetInfo.pSetLayouts = &Graphics->DescriptorSetLayout;
    
    VULKAN_CHECK_AND_HANDLE(vkAllocateDescriptorSets(Graphics->Device, &DescriptorSetInfo, &Graphics->DescriptorSet),
                            "Failed to allocate the descriptor set.");
    
    VkPipelineShaderStageCreateInfo ShaderStages[2] = {};
    ShaderStages[0].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    ShaderStages[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
    ShaderStages[0].module = OpaqueVertex;
    ShaderStages[0].pName = "main";
    
    ShaderStages[1].sType  = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    ShaderStages[1].stage  = VK_SHADER_STAGE_FRAGMENT_BIT;
    ShaderStages[1].module = OpaqueFragment;
    ShaderStages[1].pName  = "main";    
        
    VkVertexInputBindingDescription InputBindingDescription = {};
    InputBindingDescription.binding = 0;
    InputBindingDescription.stride = sizeof(graphics_vertex);
    InputBindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;    
    
    VkVertexInputAttributeDescription VertexAttributeDescription[2] = {};
    VertexAttributeDescription[0].location = 0;
    VertexAttributeDescription[0].binding = InputBindingDescription.binding;
    VertexAttributeDescription[0].format = VK_FORMAT_R32G32B32A32_SFLOAT;
    VertexAttributeDescription[0].offset = 0;        
    
    VertexAttributeDescription[1].location = 1;
    VertexAttributeDescription[1].binding = InputBindingDescription.binding;
    VertexAttributeDescription[1].format = VK_FORMAT_R32G32B32A32_SFLOAT;
    VertexAttributeDescription[1].offset = sizeof(v4f);
    
    VkPipelineVertexInputStateCreateInfo VertexInputState = {};
    VertexInputState.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;    
    VertexInputState.vertexBindingDescriptionCount = 1;
    VertexInputState.pVertexBindingDescriptions = &InputBindingDescription;
    VertexInputState.vertexAttributeDescriptionCount = ARRAYCOUNT(VertexAttributeDescription);
    VertexInputState.pVertexAttributeDescriptions = VertexAttributeDescription;    
    
    VkGraphicsPipelineCreateInfo GraphicsPipelineInfo = {};
    GraphicsPipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    GraphicsPipelineInfo.stageCount = ARRAYCOUNT(ShaderStages);
    GraphicsPipelineInfo.pStages = ShaderStages;
    GraphicsPipelineInfo.pVertexInputState = &VertexInputState;
    GraphicsPipelineInfo.pInputAssemblyState = TriangleListInputAssemblyState();
    GraphicsPipelineInfo.pViewportState = SingleViewportState();
    GraphicsPipelineInfo.pRasterizationState = DefaultCullBackRasterizationState();
    GraphicsPipelineInfo.pMultisampleState = DefaultMultisampleState();
    GraphicsPipelineInfo.pColorBlendState = DefaultColorBlendState();
    GraphicsPipelineInfo.pDepthStencilState = DepthOnState();
    GraphicsPipelineInfo.pDynamicState = ViewportScissorDynamicState();
    GraphicsPipelineInfo.layout = Graphics->PipelineLayout;
    GraphicsPipelineInfo.renderPass = Graphics->RenderPass;
    GraphicsPipelineInfo.subpass = 0;    
    
    VULKAN_CHECK_AND_HANDLE(vkCreateGraphicsPipelines(Graphics->Device, VK_NULL_HANDLE, 1, &GraphicsPipelineInfo, VK_NULL_HANDLE, &Graphics->Pipeline),
                            "Failed to create the graphics pipeline.");
    
    vkDestroyShaderModule(Graphics->Device, OpaqueVertex, VK_NULL_HANDLE);
    vkDestroyShaderModule(Graphics->Device, OpaqueFragment, VK_NULL_HANDLE);        
    
    VkBufferCreateInfo CameraBufferInfo = GetBufferInfo(sizeof(m4)*2, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT);
    VULKAN_CHECK_AND_HANDLE(vkCreateBuffer(Graphics->Device, &CameraBufferInfo, VK_NULL_HANDLE, &Graphics->CameraBuffer),
                            "Failed to create the camera buffer.");
    
    VkMemoryRequirements MemoryRequirements;
    vkGetBufferMemoryRequirements(Graphics->Device, Graphics->CameraBuffer, &MemoryRequirements);
    i32 MemoryTypeIndex = FindMemoryTypeIndex(&Graphics->SelectedGPU->MemoryProperties, &MemoryRequirements, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);
    if(MemoryTypeIndex == -1)
        WRITE_AND_HANDLE_ERROR("Failed to find a valid memory type for the camera buffer.");
    
    VkMemoryAllocateInfo AllocateInfo = {};
    AllocateInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    AllocateInfo.memoryTypeIndex = MemoryTypeIndex;
    AllocateInfo.allocationSize = MemoryRequirements.size;
    VULKAN_CHECK_AND_HANDLE(vkAllocateMemory(Graphics->Device, &AllocateInfo, VK_NULL_HANDLE, &Graphics->CameraBufferMemory),
                            "Failed to allocate the camera buffer memory.");
    
    VULKAN_CHECK_AND_HANDLE(vkBindBufferMemory(Graphics->Device, Graphics->CameraBuffer, Graphics->CameraBufferMemory, 0),
                            "Failed to bind the camera buffer to the camera memory.");
    
    VULKAN_CHECK_AND_HANDLE(vkMapMemory(Graphics->Device, Graphics->CameraBufferMemory, 0, CameraBufferInfo.size, 0, (void**)&Graphics->CameraBufferData),
                            "Failed to map the camera buffer memory.");
    
    Graphics->CameraBufferData[0] = IdentityM4();
    Graphics->CameraBufferData[1] = IdentityM4();
    
    VkDescriptorBufferInfo DescriptorBufferInfo = {};
    DescriptorBufferInfo.buffer = Graphics->CameraBuffer;
    DescriptorBufferInfo.range = VK_WHOLE_SIZE;
    
    VkWriteDescriptorSet DescriptorWrites = {};
    DescriptorWrites.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    DescriptorWrites.dstSet = Graphics->DescriptorSet;
    DescriptorWrites.dstBinding = 0;
    DescriptorWrites.dstArrayElement = 0;
    DescriptorWrites.descriptorCount = 1;
    DescriptorWrites.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    DescriptorWrites.pBufferInfo = &DescriptorBufferInfo;
    
    vkUpdateDescriptorSets(Graphics->Device, 1, &DescriptorWrites, 0, VK_NULL_HANDLE);
    
    
    VULKAN_CHECK_AND_HANDLE(vkResetCommandPool(Graphics->Device, Graphics->CommandPool, 0), "Failed ot reset the command buffer.");
    VULKAN_CHECK_AND_HANDLE(vkBeginCommandBuffer(Graphics->CommandBuffer, OneTimeCommandBufferBeginInfo()), "Failed to begin recording the command buffer.");
    
    vkCmdUpdateBuffer(Graphics->CommandBuffer, Graphics->VertexBuffer, 0, GetVerticesSizeInBytes(&Assets->BoxGraphicsMesh), GetVertices(&Assets->BoxGraphicsMesh));
    vkCmdUpdateBuffer(Graphics->CommandBuffer, Graphics->IndexBuffer, 0, GetIndicesSizeInBytes(&Assets->BoxGraphicsMesh), GetIndices(&Assets->BoxGraphicsMesh));
    
#if DEVELOPER_BUILD
    developer_vulkan_graphics* DevGraphics = (developer_vulkan_graphics*)Graphics;
    debug_primitive_context* PrimitiveContext = &DevGraphics->PrimitiveContext;
    
    VkDescriptorSetLayoutBinding DebugDescriptorBinding = {};
    DebugDescriptorBinding.binding = 0;
    DebugDescriptorBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    DebugDescriptorBinding.descriptorCount = 1;
    DebugDescriptorBinding.stageFlags = VK_SHADER_STAGE_GEOMETRY_BIT;
    
    VkDescriptorSetLayoutCreateInfo DebugDescriptorSetLayoutInfo = {};
    DebugDescriptorSetLayoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    DebugDescriptorSetLayoutInfo.bindingCount = 1;
    DebugDescriptorSetLayoutInfo.pBindings = &DebugDescriptorBinding;
    
    VULKAN_CHECK_AND_HANDLE(vkCreateDescriptorSetLayout(DevGraphics->Device, &DebugDescriptorSetLayoutInfo, VK_NULL_HANDLE, &PrimitiveContext->DescriptorSetLayout),
                            "Failed to create the debug point descriptor set layout.");
    
    VkDescriptorSetAllocateInfo DebugDescriptorSetInfo = {};
    DebugDescriptorSetInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    DebugDescriptorSetInfo.descriptorPool = DevGraphics->DescriptorPool;
    DebugDescriptorSetInfo.descriptorSetCount = 1;
    DebugDescriptorSetInfo.pSetLayouts = &PrimitiveContext->DescriptorSetLayout;
    
    VULKAN_CHECK_AND_HANDLE(vkAllocateDescriptorSets(DevGraphics->Device, &DebugDescriptorSetInfo, &PrimitiveContext->DescriptorSet),
                            "Failed to allocate the debug point descriptor set.");
    
    VkShaderModule DebugPointVertex   = CreateShader("shaders/vulkan/debug_point_vertex.spv");
    VkShaderModule DebugPointGeometry = CreateShader("shaders/vulkan/debug_point_geometry.spv");
    VkShaderModule DebugPointFragment = CreateShader("shaders/vulkan/debug_point_fragment.spv");
    
    BOOL_CHECK_AND_HANDLE(DebugPointVertex,   "Failed to create the debug point vertex shader.");
    BOOL_CHECK_AND_HANDLE(DebugPointGeometry, "Failed to create the debug point geometry shader.");
    BOOL_CHECK_AND_HANDLE(DebugPointFragment, "Failed to create the debug point fragment shader.");
    
    VkPushConstantRange PointPushConstantRanges[2] = {};
    PointPushConstantRanges[0].stageFlags = VK_SHADER_STAGE_GEOMETRY_BIT;
    PointPushConstantRanges[0].offset = 0;
    PointPushConstantRanges[0].size = sizeof(v4f);
    
    PointPushConstantRanges[1].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
    PointPushConstantRanges[1].offset = sizeof(v4f);    
    PointPushConstantRanges[1].size = sizeof(c4);
    
    VkPipelineLayoutCreateInfo PointPipelineLayoutInfo = {};
    PointPipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    PointPipelineLayoutInfo.setLayoutCount = 1;
    PointPipelineLayoutInfo.pSetLayouts = &PrimitiveContext->DescriptorSetLayout;
    PointPipelineLayoutInfo.pushConstantRangeCount = ARRAYCOUNT(PointPushConstantRanges);
    PointPipelineLayoutInfo.pPushConstantRanges = PointPushConstantRanges;
    
    VULKAN_CHECK_AND_HANDLE(vkCreatePipelineLayout(DevGraphics->Device, &PointPipelineLayoutInfo, VK_NULL_HANDLE, &PrimitiveContext->PointPipelineLayout),
                            "Failed to create the debug point pipeline layout.");
    
    VkPipelineShaderStageCreateInfo PointShaderStages[3] = {};
    PointShaderStages[0].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    PointShaderStages[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
    PointShaderStages[0].module = DebugPointVertex;
    PointShaderStages[0].pName = "main";
    
    PointShaderStages[1].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    PointShaderStages[1].stage = VK_SHADER_STAGE_GEOMETRY_BIT;
    PointShaderStages[1].module = DebugPointGeometry;
    PointShaderStages[1].pName = "main";
    
    PointShaderStages[2].sType  = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    PointShaderStages[2].stage  = VK_SHADER_STAGE_FRAGMENT_BIT;
    PointShaderStages[2].module = DebugPointFragment;
    PointShaderStages[2].pName  = "main";    
    
    VkGraphicsPipelineCreateInfo PointGraphicsPipelineInfo = {};
    PointGraphicsPipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    PointGraphicsPipelineInfo.stageCount = ARRAYCOUNT(PointShaderStages);
    PointGraphicsPipelineInfo.pStages = PointShaderStages;
    PointGraphicsPipelineInfo.pVertexInputState = EmptyVertexInputState();
    PointGraphicsPipelineInfo.pInputAssemblyState = PointListInputAssemblyState();
    PointGraphicsPipelineInfo.pViewportState = SingleViewportState();
    PointGraphicsPipelineInfo.pRasterizationState = DefaultCullBackRasterizationState();
    PointGraphicsPipelineInfo.pMultisampleState = DefaultMultisampleState();
    PointGraphicsPipelineInfo.pColorBlendState = DefaultColorBlendState();    
    PointGraphicsPipelineInfo.pDynamicState = ViewportScissorDynamicState();
    PointGraphicsPipelineInfo.pDepthStencilState = DepthOffState();
    PointGraphicsPipelineInfo.layout = PrimitiveContext->PointPipelineLayout;
    PointGraphicsPipelineInfo.renderPass = Graphics->RenderPass;
    PointGraphicsPipelineInfo.subpass = 0;    
    
    VULKAN_CHECK_AND_HANDLE(vkCreateGraphicsPipelines(Graphics->Device, VK_NULL_HANDLE, 1, &PointGraphicsPipelineInfo, VK_NULL_HANDLE, &PrimitiveContext->PointPipeline),
                            "Failed to create the debug point graphics pipeline.");
    
    vkDestroyShaderModule(DevGraphics->Device, DebugPointVertex, VK_NULL_HANDLE);
    vkDestroyShaderModule(DevGraphics->Device, DebugPointGeometry, VK_NULL_HANDLE);        
    vkDestroyShaderModule(DevGraphics->Device, DebugPointFragment, VK_NULL_HANDLE);
    
    VkShaderModule DebugLineVertex   = CreateShader("shaders/vulkan/debug_line_vertex.spv");
    VkShaderModule DebugLineGeometry = CreateShader("shaders/vulkan/debug_line_geometry.spv");
    VkShaderModule DebugLineFragment = CreateShader("shaders/vulkan/debug_line_fragment.spv");
    
    BOOL_CHECK_AND_HANDLE(DebugLineVertex,   "Failed to create the debug line vertex shader.");
    BOOL_CHECK_AND_HANDLE(DebugLineGeometry, "Failed to create the debug line geometry shader.");
    BOOL_CHECK_AND_HANDLE(DebugLineFragment, "Failed to create the debug line fragment shader.");
    
    VkPushConstantRange LinePushConstantRanges[2] = {};
    LinePushConstantRanges[0].stageFlags = VK_SHADER_STAGE_GEOMETRY_BIT;
    LinePushConstantRanges[0].offset = 0;
    LinePushConstantRanges[0].size = sizeof(v4f)*2;
    
    LinePushConstantRanges[1].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
    LinePushConstantRanges[1].offset = sizeof(v4f)*2;
    LinePushConstantRanges[1].size = sizeof(c4);
    
    VkPipelineLayoutCreateInfo LinePipelineLayoutInfo = {};
    LinePipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    LinePipelineLayoutInfo.setLayoutCount = 1;
    LinePipelineLayoutInfo.pSetLayouts = &PrimitiveContext->DescriptorSetLayout;
    LinePipelineLayoutInfo.pushConstantRangeCount = ARRAYCOUNT(LinePushConstantRanges);
    LinePipelineLayoutInfo.pPushConstantRanges = LinePushConstantRanges;
    
    VULKAN_CHECK_AND_HANDLE(vkCreatePipelineLayout(DevGraphics->Device, &LinePipelineLayoutInfo, VK_NULL_HANDLE, &PrimitiveContext->LinePipelineLayout),
                            "Failed to create the debug line pipeline layout.");
    
    VkPipelineShaderStageCreateInfo LineShaderStages[3] = {};
    LineShaderStages[0].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    LineShaderStages[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
    LineShaderStages[0].module = DebugLineVertex;
    LineShaderStages[0].pName = "main";
    
    LineShaderStages[1].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    LineShaderStages[1].stage = VK_SHADER_STAGE_GEOMETRY_BIT;
    LineShaderStages[1].module = DebugLineGeometry;
    LineShaderStages[1].pName = "main";
    
    LineShaderStages[2].sType  = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    LineShaderStages[2].stage  = VK_SHADER_STAGE_FRAGMENT_BIT;
    LineShaderStages[2].module = DebugLineFragment;
    LineShaderStages[2].pName  = "main";    
    
    VkGraphicsPipelineCreateInfo LineGraphicsPipelineInfo = {};
    LineGraphicsPipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    LineGraphicsPipelineInfo.stageCount = ARRAYCOUNT(LineShaderStages);
    LineGraphicsPipelineInfo.pStages = LineShaderStages;
    LineGraphicsPipelineInfo.pVertexInputState = EmptyVertexInputState();
    LineGraphicsPipelineInfo.pInputAssemblyState = PointListInputAssemblyState();
    LineGraphicsPipelineInfo.pViewportState = SingleViewportState();
    LineGraphicsPipelineInfo.pRasterizationState = DefaultCullBackRasterizationState();
    LineGraphicsPipelineInfo.pMultisampleState = DefaultMultisampleState();
    LineGraphicsPipelineInfo.pColorBlendState = DefaultColorBlendState();    
    LineGraphicsPipelineInfo.pDynamicState = ViewportScissorDynamicState();
    LineGraphicsPipelineInfo.pDepthStencilState = DepthOffState();
    LineGraphicsPipelineInfo.layout = PrimitiveContext->LinePipelineLayout;
    LineGraphicsPipelineInfo.renderPass = Graphics->RenderPass;
    LineGraphicsPipelineInfo.subpass = 0;    
    
    VULKAN_CHECK_AND_HANDLE(vkCreateGraphicsPipelines(Graphics->Device, VK_NULL_HANDLE, 1, &LineGraphicsPipelineInfo, VK_NULL_HANDLE, &PrimitiveContext->LinePipeline),
                            "Failed to create the debug point graphics pipeline.");    
    
    vkDestroyShaderModule(DevGraphics->Device, DebugLineVertex, VK_NULL_HANDLE);
    vkDestroyShaderModule(DevGraphics->Device, DebugLineGeometry, VK_NULL_HANDLE);
    vkDestroyShaderModule(DevGraphics->Device, DebugLineFragment, VK_NULL_HANDLE);
    
    debug_volume_context* VolumeContext = &DevGraphics->VolumeContext;    
    VkDescriptorSetLayoutBinding DebugVolumeDescriptorBinding = {};
    DebugVolumeDescriptorBinding.binding = 0;
    DebugVolumeDescriptorBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    DebugVolumeDescriptorBinding.descriptorCount = 1;
    DebugVolumeDescriptorBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
    
    VkDescriptorSetLayoutCreateInfo DebugVolumeDescriptorSetLayoutInfo = {};
    DebugVolumeDescriptorSetLayoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    DebugVolumeDescriptorSetLayoutInfo.bindingCount = 1;
    DebugVolumeDescriptorSetLayoutInfo.pBindings = &DebugVolumeDescriptorBinding;
    
    VULKAN_CHECK_AND_HANDLE(vkCreateDescriptorSetLayout(Graphics->Device, &DebugVolumeDescriptorSetLayoutInfo, VK_NULL_HANDLE, &VolumeContext->DescriptorSetLayout),
                            "Failed to create the debug volumes descriptor set layout.");    
    
    VkPushConstantRange DebugVolumesPushConstantRanges[2] = {};
    DebugVolumesPushConstantRanges[0].stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
    DebugVolumesPushConstantRanges[0].offset = 0;
    DebugVolumesPushConstantRanges[0].size = sizeof(m4);
    
    DebugVolumesPushConstantRanges[1].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
    DebugVolumesPushConstantRanges[1].offset = sizeof(m4);
    DebugVolumesPushConstantRanges[1].size = sizeof(c4);
    
    VkPipelineLayoutCreateInfo DebugVolumePipelineLayoutInfo = {};
    DebugVolumePipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    DebugVolumePipelineLayoutInfo.setLayoutCount = 1;
    DebugVolumePipelineLayoutInfo.pSetLayouts = &VolumeContext->DescriptorSetLayout;
    DebugVolumePipelineLayoutInfo.pushConstantRangeCount = ARRAYCOUNT(DebugVolumesPushConstantRanges);
    DebugVolumePipelineLayoutInfo.pPushConstantRanges = DebugVolumesPushConstantRanges;
    
    VULKAN_CHECK_AND_HANDLE(vkCreatePipelineLayout(Graphics->Device, &DebugVolumePipelineLayoutInfo, VK_NULL_HANDLE, &VolumeContext->PipelineLayout),
                            "Failed to create the debug volume pipeline layout.");
    
    VkDescriptorSetAllocateInfo DebugVolumeDescriptorSetInfo = {};
    DebugVolumeDescriptorSetInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    DebugVolumeDescriptorSetInfo.descriptorPool = Graphics->DescriptorPool;
    DebugVolumeDescriptorSetInfo.descriptorSetCount = 1;
    DebugVolumeDescriptorSetInfo.pSetLayouts = &VolumeContext->DescriptorSetLayout;
    VULKAN_CHECK_AND_HANDLE(vkAllocateDescriptorSets(Graphics->Device, &DebugVolumeDescriptorSetInfo, &VolumeContext->DescriptorSet),
                            "Failed to allocate the debug volume descriptor set.");
    
    VkShaderModule DebugVolumesVertex   = CreateShader("shaders/vulkan/debug_volumes_vertex.spv");
    VkShaderModule DebugVolumesFragment = CreateShader("shaders/vulkan/debug_volumes_fragment.spv");
    
    BOOL_CHECK_AND_HANDLE(DebugVolumesVertex, "Failed to create the debug volume vertex shader.");
    BOOL_CHECK_AND_HANDLE(DebugVolumesFragment, "Failed to create the debug fragment vertex shader.");
    
    VkPipelineShaderStageCreateInfo VolumeShaderStages[2] = {};
    VolumeShaderStages[0].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    VolumeShaderStages[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
    VolumeShaderStages[0].module = DebugVolumesVertex;
    VolumeShaderStages[0].pName = "main";
    
    VolumeShaderStages[1].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    VolumeShaderStages[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    VolumeShaderStages[1].module = DebugVolumesFragment;
    VolumeShaderStages[1].pName = "main";
    
    VkVertexInputBindingDescription DebugVolumesInputBindingDescription = {};
    DebugVolumesInputBindingDescription.binding = 0;
    DebugVolumesInputBindingDescription.stride = sizeof(v3f);
    DebugVolumesInputBindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
    
    VkVertexInputAttributeDescription DebugVolumesVertexAttributeDescription = {};
    DebugVolumesVertexAttributeDescription.location = 0;
    DebugVolumesVertexAttributeDescription.binding = 0;
    DebugVolumesVertexAttributeDescription.format = VK_FORMAT_R32G32B32_SFLOAT;
    DebugVolumesVertexAttributeDescription.offset = 0;
    
    VkPipelineVertexInputStateCreateInfo DebugVolumesVertexInputState = {};
    DebugVolumesVertexInputState.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;    
    DebugVolumesVertexInputState.vertexBindingDescriptionCount = 1;
    DebugVolumesVertexInputState.pVertexBindingDescriptions = &DebugVolumesInputBindingDescription;
    DebugVolumesVertexInputState.vertexAttributeDescriptionCount = 1;
    DebugVolumesVertexInputState.pVertexAttributeDescriptions = &DebugVolumesVertexAttributeDescription;
    
    VkGraphicsPipelineCreateInfo DebugVolumesGraphicsPipelineInfo = {};
    DebugVolumesGraphicsPipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    DebugVolumesGraphicsPipelineInfo.stageCount = ARRAYCOUNT(VolumeShaderStages);
    DebugVolumesGraphicsPipelineInfo.pStages = VolumeShaderStages;
    DebugVolumesGraphicsPipelineInfo.pVertexInputState = &DebugVolumesVertexInputState;
    DebugVolumesGraphicsPipelineInfo.pInputAssemblyState = LineListInputAssemblyState();
    DebugVolumesGraphicsPipelineInfo.pViewportState = SingleViewportState();
    DebugVolumesGraphicsPipelineInfo.pRasterizationState = DefaultCullBackRasterizationState();
    DebugVolumesGraphicsPipelineInfo.pMultisampleState = DefaultMultisampleState();
    DebugVolumesGraphicsPipelineInfo.pColorBlendState = DefaultColorBlendState();
    DebugVolumesGraphicsPipelineInfo.pDepthStencilState = DepthOffState();
    DebugVolumesGraphicsPipelineInfo.pDynamicState = ViewportScissorDynamicState();
    DebugVolumesGraphicsPipelineInfo.layout = VolumeContext->PipelineLayout;
    DebugVolumesGraphicsPipelineInfo.renderPass = Graphics->RenderPass;
    DebugVolumesGraphicsPipelineInfo.subpass = 0;
    
    VULKAN_CHECK_AND_HANDLE(vkCreateGraphicsPipelines(Graphics->Device, VK_NULL_HANDLE, 1, &DebugVolumesGraphicsPipelineInfo, VK_NULL_HANDLE, &VolumeContext->Pipeline),
                            "Failed to create the debug volume pipeline.");
    
    vkDestroyShaderModule(DevGraphics->Device, DebugVolumesVertex, VK_NULL_HANDLE);
    vkDestroyShaderModule(DevGraphics->Device, DebugVolumesFragment, VK_NULL_HANDLE);
    
    VolumeContext->CapsuleMesh = DEBUGCreateCapsuleMesh(60);
    debug_capsule_mesh* CapsuleMesh = &VolumeContext->CapsuleMesh;
    
    VolumeContext->BoxMesh = DEBUGCreateBoxMesh();
    
    VkDeviceSize DEBUGCapsuleCapVertexSize = CapsuleMesh->Cap.Vertices.Count*sizeof(v3f);
    VkDeviceSize DEBUGCapsuleBodyVertexSize = CapsuleMesh->Body.Vertices.Count*sizeof(v3f);
    VkDeviceSize DEBUGBoxVertexSize = VolumeContext->BoxMesh.Vertices.Count*sizeof(v3f);
    VkDeviceSize DEBUGCapsuleCapIndicesSize = CapsuleMesh->Cap.Indices.Count*sizeof(u16);
    VkDeviceSize DEBUGCapsuleBodyIndicesSize = CapsuleMesh->Body.Indices.Count*sizeof(u16);
    VkDeviceSize DEBUGBoxIndicesSize = VolumeContext->BoxMesh.Indices.Count*sizeof(u16);
    
    VkDeviceSize DEBUGVertexBufferSize = DEBUGCapsuleCapVertexSize+DEBUGCapsuleBodyVertexSize+DEBUGBoxVertexSize;
    VkDeviceSize DEBUGIndexBufferSize = DEBUGCapsuleCapIndicesSize+DEBUGCapsuleBodyIndicesSize+DEBUGBoxIndicesSize;
    
    VkBufferCreateInfo DEBUGVertexBufferCreateInfo = GetBufferInfo(DEBUGVertexBufferSize, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT|VK_BUFFER_USAGE_TRANSFER_DST_BIT);    
    VULKAN_CHECK_AND_HANDLE(vkCreateBuffer(Graphics->Device, &DEBUGVertexBufferCreateInfo, VK_NULL_HANDLE, &VolumeContext->VertexBuffer),
                            "Failed to create the debug vertex buffer.");
    
    VkBufferCreateInfo DEBUGIndexBufferCreateInfo = GetBufferInfo(DEBUGIndexBufferSize, VK_BUFFER_USAGE_INDEX_BUFFER_BIT|VK_BUFFER_USAGE_TRANSFER_DST_BIT);
    VULKAN_CHECK_AND_HANDLE(vkCreateBuffer(Graphics->Device, &DEBUGIndexBufferCreateInfo, VK_NULL_HANDLE, &VolumeContext->IndexBuffer),
                            "Failed to create the debug index buffer.");
    
    VkMemoryRequirements DEBUGVertexBufferMemoryRequirements = {};
    VkMemoryRequirements DEBUGIndexBufferMemoryRequirements = {};
    
    vkGetBufferMemoryRequirements(Graphics->Device, VolumeContext->VertexBuffer, &DEBUGVertexBufferMemoryRequirements);
    vkGetBufferMemoryRequirements(Graphics->Device, VolumeContext->IndexBuffer, &DEBUGIndexBufferMemoryRequirements);    
    
    i32 DEBUGMemoryTypeIndex = FindMemoryTypeIndex(&Graphics->SelectedGPU->MemoryProperties, &DEBUGVertexBufferMemoryRequirements, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    if(DEBUGMemoryTypeIndex == -1)
        WRITE_AND_HANDLE_ERROR("Failed to find a valid memory type for the debug vertex buffer.");
    
    if(FindMemoryTypeIndex(&Graphics->SelectedGPU->MemoryProperties, &DEBUGIndexBufferMemoryRequirements, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT) != DEBUGMemoryTypeIndex)    
        WRITE_AND_HANDLE_ERROR("Memory type index for the debugindex buffer does not equal the debug vertex buffer memory index.");    
    
    VkDeviceSize DEBUGIndexMemoryOffset = ALIGN(DEBUGVertexBufferMemoryRequirements.size, (i32)DEBUGIndexBufferMemoryRequirements.alignment);    
    VkDeviceSize DEBUGMemorySize = DEBUGIndexMemoryOffset+DEBUGIndexBufferMemoryRequirements.size;
    VkMemoryAllocateInfo DEBUGAllocateInfo = {};
    DEBUGAllocateInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    DEBUGAllocateInfo.memoryTypeIndex = DEBUGMemoryTypeIndex;
    DEBUGAllocateInfo.allocationSize = DEBUGMemorySize;
    VULKAN_CHECK_AND_HANDLE(vkAllocateMemory(Graphics->Device, &DEBUGAllocateInfo, VK_NULL_HANDLE, &VolumeContext->Memory),
                            "Failed to allocate the debug memory.");
    VULKAN_CHECK_AND_HANDLE(vkBindBufferMemory(Graphics->Device, VolumeContext->VertexBuffer, VolumeContext->Memory, 0),
                            "Failed to bind the debug vertex buffer to the debug memory.");
    VULKAN_CHECK_AND_HANDLE(vkBindBufferMemory(Graphics->Device, VolumeContext->IndexBuffer, VolumeContext->Memory, DEBUGIndexMemoryOffset),
                            "Failed to bind the debug index buffer to the debug memory.");
    
    debug_imgui_context* ImGuiContext = &DevGraphics->ImGuiContext;
    
    ImGuiIO& IO = ImGui::GetIO();
    IO.BackendRendererName = "world_game_vulkan_graphics";
    IO.BackendFlags |= ImGuiBackendFlags_RendererHasVtxOffset;
    
    VkSamplerCreateInfo ImGuiSamplerInfo = {};
    ImGuiSamplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    ImGuiSamplerInfo.magFilter = VK_FILTER_LINEAR;
    ImGuiSamplerInfo.minFilter = VK_FILTER_LINEAR;
    ImGuiSamplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
    ImGuiSamplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    ImGuiSamplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    
    //Explain imgui_impl_vulkan.cpp ?!
    ImGuiSamplerInfo.minLod = -1000;
    ImGuiSamplerInfo.maxLod =  1000;    
    
    ImGuiSamplerInfo.maxAnisotropy = 1.0f;
    VULKAN_CHECK_AND_HANDLE(vkCreateSampler(Graphics->Device, &ImGuiSamplerInfo, VK_NULL_HANDLE, &ImGuiContext->FontSampler),
                            "Failed to create the imgui sampler.");
    
    VkDescriptorSetLayoutBinding ImGuiLayoutBinding = {};
    ImGuiLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    ImGuiLayoutBinding.descriptorCount = 1;
    ImGuiLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
    ImGuiLayoutBinding.pImmutableSamplers = &ImGuiContext->FontSampler;
    
    VkDescriptorSetLayoutCreateInfo ImGuiDescriptorSetLayoutInfo = {};
    ImGuiDescriptorSetLayoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    ImGuiDescriptorSetLayoutInfo.pBindings = &ImGuiLayoutBinding;
    ImGuiDescriptorSetLayoutInfo.bindingCount = 1;
    VULKAN_CHECK_AND_HANDLE(vkCreateDescriptorSetLayout(Graphics->Device, &ImGuiDescriptorSetLayoutInfo, VK_NULL_HANDLE, &ImGuiContext->DescriptorSetLayout),
                            "Failed to create the imgui descriptor set layout.");
    
    VkDescriptorSetAllocateInfo ImGuiDescriptorSetInfo = {};
    ImGuiDescriptorSetInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    ImGuiDescriptorSetInfo.descriptorPool = Graphics->DescriptorPool;
    ImGuiDescriptorSetInfo.descriptorSetCount = 1;
    ImGuiDescriptorSetInfo.pSetLayouts = &ImGuiContext->DescriptorSetLayout;
    VULKAN_CHECK_AND_HANDLE(vkAllocateDescriptorSets(Graphics->Device, &ImGuiDescriptorSetInfo, &ImGuiContext->DescriptorSet),
                            "Failed to allocate the imgui descriptor set.");
    
    VkPushConstantRange ImGuiPushConstantRanges[1] = {};
    ImGuiPushConstantRanges[0].stageFlags = VK_SHADER_STAGE_VERTEX_BIT;    
    ImGuiPushConstantRanges[0].size = sizeof(v2f)*2;
    
    VkPipelineLayoutCreateInfo ImGuiPipelineLayoutInfo = {};
    ImGuiPipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    ImGuiPipelineLayoutInfo.setLayoutCount = 1;
    ImGuiPipelineLayoutInfo.pSetLayouts = &ImGuiContext->DescriptorSetLayout;
    ImGuiPipelineLayoutInfo.pushConstantRangeCount = ARRAYCOUNT(ImGuiPushConstantRanges);
    ImGuiPipelineLayoutInfo.pPushConstantRanges = ImGuiPushConstantRanges;
    VULKAN_CHECK_AND_HANDLE(vkCreatePipelineLayout(Graphics->Device, &ImGuiPipelineLayoutInfo, VK_NULL_HANDLE, &ImGuiContext->PipelineLayout),
                            "Failed to create the imgui pipeline layout.");    
    
    VkShaderModule ImGuiVertex   = CreateShader("shaders/vulkan/imgui_vertex.spv");
    VkShaderModule ImGuiFragment = CreateShader("shaders/vulkan/imgui_fragment.spv");
    
    BOOL_CHECK_AND_HANDLE(ImGuiVertex, "Failed to create the imgui vertex shader.");
    BOOL_CHECK_AND_HANDLE(ImGuiFragment, "Failed to create the imgui fragment shader.");
    
    VkPipelineShaderStageCreateInfo ImGuiShaderStages[2] = {};
    ImGuiShaderStages[0].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    ImGuiShaderStages[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
    ImGuiShaderStages[0].module = ImGuiVertex;
    ImGuiShaderStages[0].pName = "main";
    
    ImGuiShaderStages[1].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    ImGuiShaderStages[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    ImGuiShaderStages[1].module = ImGuiFragment;
    ImGuiShaderStages[1].pName = "main";
    
    VkVertexInputBindingDescription ImGuiVertexBindings[1] = {};
    ImGuiVertexBindings[0].stride = sizeof(ImDrawVert);
    ImGuiVertexBindings[0].inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
    ImGuiVertexBindings[0].binding = 0;
    
    VkVertexInputAttributeDescription ImGuiVertexAttributes[3] = {};
    ImGuiVertexAttributes[0].location = 0;
    ImGuiVertexAttributes[0].binding = ImGuiVertexBindings[0].binding;
    ImGuiVertexAttributes[0].format = VK_FORMAT_R32G32_SFLOAT;
    ImGuiVertexAttributes[0].offset = IM_OFFSETOF(ImDrawVert, pos);
    
    ImGuiVertexAttributes[1].location = 1;
    ImGuiVertexAttributes[1].binding = ImGuiVertexBindings[0].binding;
    ImGuiVertexAttributes[1].format = VK_FORMAT_R32G32_SFLOAT;
    ImGuiVertexAttributes[1].offset = IM_OFFSETOF(ImDrawVert, uv);
    
    ImGuiVertexAttributes[2].location = 2;
    ImGuiVertexAttributes[2].binding = ImGuiVertexBindings[0].binding;
    ImGuiVertexAttributes[2].format = VK_FORMAT_R8G8B8A8_UNORM;
    ImGuiVertexAttributes[2].offset = IM_OFFSETOF(ImDrawVert, col);
    
    VkPipelineVertexInputStateCreateInfo ImGuiVertexInputState = {};
    ImGuiVertexInputState.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    ImGuiVertexInputState.vertexBindingDescriptionCount = ARRAYCOUNT(ImGuiVertexBindings);
    ImGuiVertexInputState.pVertexBindingDescriptions = ImGuiVertexBindings;
    ImGuiVertexInputState.vertexAttributeDescriptionCount = ARRAYCOUNT(ImGuiVertexAttributes);
    ImGuiVertexInputState.pVertexAttributeDescriptions = ImGuiVertexAttributes;    
    
    VkPipelineColorBlendAttachmentState ImGuiBlendAttachment = {};
    ImGuiBlendAttachment.blendEnable = VK_TRUE;
    ImGuiBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
    ImGuiBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
    ImGuiBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
    ImGuiBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
    ImGuiBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
    ImGuiBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;
    ImGuiBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    
    VkPipelineColorBlendStateCreateInfo ImGuiBlendState = {};
    ImGuiBlendState.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    ImGuiBlendState.attachmentCount = 1;
    ImGuiBlendState.pAttachments = &ImGuiBlendAttachment;
    
    VkGraphicsPipelineCreateInfo ImGuiPipelineInfo = {};
    ImGuiPipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    ImGuiPipelineInfo.stageCount = ARRAYCOUNT(ImGuiShaderStages);
    ImGuiPipelineInfo.pStages = ImGuiShaderStages;
    ImGuiPipelineInfo.pVertexInputState = &ImGuiVertexInputState;
    ImGuiPipelineInfo.pInputAssemblyState = TriangleListInputAssemblyState();
    ImGuiPipelineInfo.pViewportState = SingleViewportState();
    ImGuiPipelineInfo.pRasterizationState = DefaultCullNoneRasterizationState();
    ImGuiPipelineInfo.pMultisampleState = DefaultMultisampleState();
    ImGuiPipelineInfo.pDepthStencilState = DepthOffState();
    ImGuiPipelineInfo.pColorBlendState = &ImGuiBlendState;
    ImGuiPipelineInfo.pDynamicState = ViewportScissorDynamicState();
    ImGuiPipelineInfo.layout = ImGuiContext->PipelineLayout;
    ImGuiPipelineInfo.renderPass = Graphics->RenderPass;
    
    VULKAN_CHECK_AND_HANDLE(vkCreateGraphicsPipelines(Graphics->Device, VK_NULL_HANDLE, 1, &ImGuiPipelineInfo, VK_NULL_HANDLE, &ImGuiContext->Pipeline),
                            "Failed to create the imgui pipeline.");
    
    vkDestroyShaderModule(Graphics->Device, ImGuiVertex, VK_NULL_HANDLE);
    vkDestroyShaderModule(Graphics->Device, ImGuiFragment, VK_NULL_HANDLE);
    
    u8* ImGuiFontData;
    i32 ImGuiFontWidth, ImGuiFontHeight;    
    IO.Fonts->GetTexDataAsRGBA32(&ImGuiFontData, &ImGuiFontWidth, &ImGuiFontHeight);
    ptr ImGuiFontDataSize = ImGuiFontWidth*ImGuiFontHeight*4*sizeof(u8);
    
    VkImageCreateInfo ImGuiFontInfo = {};
    ImGuiFontInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    ImGuiFontInfo.imageType = VK_IMAGE_TYPE_2D;
    ImGuiFontInfo.format = VK_FORMAT_R8G8B8A8_UNORM;
    ImGuiFontInfo.extent.width = ImGuiFontWidth;
    ImGuiFontInfo.extent.height = ImGuiFontHeight;
    ImGuiFontInfo.extent.depth = 1;
    ImGuiFontInfo.mipLevels = 1;
    ImGuiFontInfo.arrayLayers = 1;
    ImGuiFontInfo.samples = VK_SAMPLE_COUNT_1_BIT;
    ImGuiFontInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
    ImGuiFontInfo.usage = VK_IMAGE_USAGE_SAMPLED_BIT|VK_IMAGE_USAGE_TRANSFER_DST_BIT;
    ImGuiFontInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    ImGuiFontInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    VULKAN_CHECK_AND_HANDLE(vkCreateImage(Graphics->Device, &ImGuiFontInfo, VK_NULL_HANDLE, &ImGuiContext->FontImage),
                            "Failed to create the imgui font image.");
    
    ImGuiContext->FontImageMemory = AllocateAndBindMemory(ImGuiContext->FontImage, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);        
    BOOL_CHECK_AND_HANDLE(ImGuiContext->FontImageMemory, "Failed to allocate the ImGui Font image memory.");
    
    VkImageViewCreateInfo FontImageViewInfo = {};
    FontImageViewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    FontImageViewInfo.image = ImGuiContext->FontImage;
    FontImageViewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    FontImageViewInfo.format = VK_FORMAT_R8G8B8A8_UNORM;
    FontImageViewInfo.subresourceRange = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1};
    VULKAN_CHECK_AND_HANDLE(vkCreateImageView(Graphics->Device, &FontImageViewInfo, VK_NULL_HANDLE, &ImGuiContext->FontImageView),
                            "Failed to create the ImGui font image view.");
    
    
    {
        VkDeviceSize VertexOffset = 0;        
        
        vkCmdUpdateBuffer(Graphics->CommandBuffer, VolumeContext->VertexBuffer, VertexOffset, DEBUGCapsuleCapVertexSize, CapsuleMesh->Cap.Vertices.Ptr);
        VertexOffset += DEBUGCapsuleCapVertexSize;
        
        vkCmdUpdateBuffer(Graphics->CommandBuffer, VolumeContext->VertexBuffer, VertexOffset, DEBUGCapsuleBodyVertexSize, CapsuleMesh->Body.Vertices.Ptr);
        VertexOffset += DEBUGCapsuleBodyVertexSize;
        
        vkCmdUpdateBuffer(Graphics->CommandBuffer, VolumeContext->VertexBuffer, VertexOffset, DEBUGBoxVertexSize, VolumeContext->BoxMesh.Vertices.Ptr);
        VertexOffset += DEBUGBoxVertexSize;                    
    }
    
    {
        VkDeviceSize IndexOffset = 0;
        
        vkCmdUpdateBuffer(Graphics->CommandBuffer, VolumeContext->IndexBuffer, IndexOffset, DEBUGCapsuleCapIndicesSize, CapsuleMesh->Cap.Indices.Ptr);
        IndexOffset += DEBUGCapsuleCapIndicesSize;
        
        vkCmdUpdateBuffer(Graphics->CommandBuffer, VolumeContext->IndexBuffer, IndexOffset, DEBUGCapsuleBodyIndicesSize, CapsuleMesh->Body.Indices.Ptr);
        IndexOffset += DEBUGCapsuleBodyIndicesSize;
        
        vkCmdUpdateBuffer(Graphics->CommandBuffer, VolumeContext->IndexBuffer, IndexOffset, DEBUGBoxIndicesSize, VolumeContext->BoxMesh.Indices.Ptr);
        IndexOffset += DEBUGBoxIndicesSize;
    }
    
    VkImageMemoryBarrier CopyBarrier[1] = {};
    CopyBarrier[0].sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    CopyBarrier[0].dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    CopyBarrier[0].oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    CopyBarrier[0].newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    CopyBarrier[0].srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    CopyBarrier[0].dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    CopyBarrier[0].image = ImGuiContext->FontImage;
    CopyBarrier[0].subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    CopyBarrier[0].subresourceRange.levelCount = 1;
    CopyBarrier[0].subresourceRange.layerCount = 1;
    vkCmdPipelineBarrier(Graphics->CommandBuffer, VK_PIPELINE_STAGE_HOST_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, NULL, 0, NULL, 1, CopyBarrier);
    
    VkDeviceSize FontUploadOffset = PushWrite(&Graphics->UploadBuffer, ImGuiFontData, ImGuiFontDataSize);
    
    VkBufferImageCopy ImageRegion = {};
    ImageRegion.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    ImageRegion.imageSubresource.layerCount = 1;
    ImageRegion.bufferOffset = FontUploadOffset;
    ImageRegion.imageExtent.width = ImGuiFontWidth;
    ImageRegion.imageExtent.height = ImGuiFontHeight;
    ImageRegion.imageExtent.depth = 1;
    vkCmdCopyBufferToImage(Graphics->CommandBuffer, Graphics->UploadBuffer.Buffer, ImGuiContext->FontImage, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &ImageRegion);
    
    VkImageMemoryBarrier UseBarrier[1] = {};
    UseBarrier[0].sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    UseBarrier[0].srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    UseBarrier[0].dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
    UseBarrier[0].oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    UseBarrier[0].newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    UseBarrier[0].srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    UseBarrier[0].dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    UseBarrier[0].image = ImGuiContext->FontImage;
    UseBarrier[0].subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    UseBarrier[0].subresourceRange.levelCount = 1;
    UseBarrier[0].subresourceRange.layerCount = 1;
    vkCmdPipelineBarrier(Graphics->CommandBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, 0, NULL, 0, NULL, 1, UseBarrier);        
    
    VkDescriptorImageInfo DescriptorImageInfo = {};
    DescriptorImageInfo.sampler = ImGuiContext->FontSampler;
    DescriptorImageInfo.imageView = ImGuiContext->FontImageView;
    DescriptorImageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;        
    
    VkWriteDescriptorSet DebugDescriptorWrites[3] = {};    
    DebugDescriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    DebugDescriptorWrites[0].dstSet = PrimitiveContext->DescriptorSet;
    DebugDescriptorWrites[0].dstBinding = 0;
    DebugDescriptorWrites[0].dstArrayElement = 0;
    DebugDescriptorWrites[0].descriptorCount = 1;
    DebugDescriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    DebugDescriptorWrites[0].pBufferInfo = &DescriptorBufferInfo;
    
    DebugDescriptorWrites[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    DebugDescriptorWrites[1].dstSet = VolumeContext->DescriptorSet;
    DebugDescriptorWrites[1].dstBinding = 0;
    DebugDescriptorWrites[1].dstArrayElement = 0;
    DebugDescriptorWrites[1].descriptorCount = 1;
    DebugDescriptorWrites[1].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    DebugDescriptorWrites[1].pBufferInfo = &DescriptorBufferInfo;
    
    DebugDescriptorWrites[2].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    DebugDescriptorWrites[2].dstSet = ImGuiContext->DescriptorSet;
    DebugDescriptorWrites[2].dstBinding = 0;
    DebugDescriptorWrites[2].dstArrayElement = 0;
    DebugDescriptorWrites[2].descriptorCount = 1;
    DebugDescriptorWrites[2].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    DebugDescriptorWrites[2].pImageInfo = &DescriptorImageInfo;
    
    vkUpdateDescriptorSets(DevGraphics->Device, ARRAYCOUNT(DebugDescriptorWrites), DebugDescriptorWrites, 0, VK_NULL_HANDLE);    
    
#endif
    
    VULKAN_CHECK_AND_HANDLE(vkEndCommandBuffer(Graphics->CommandBuffer), "Failed to end the command buffer recording.");
    
    VkSubmitInfo SubmitInfo = {};
    SubmitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    SubmitInfo.commandBufferCount = 1;
    SubmitInfo.pCommandBuffers = &Graphics->CommandBuffer;
    
    VULKAN_CHECK_AND_HANDLE(vkQueueSubmit(Graphics->GraphicsQueue, 1, &SubmitInfo, VK_NULL_HANDLE), 
                            "Failed to submit the command buffer to the graphics queue.");
    
    return true;
    
    handle_error:
    return false;
}

void VulkanUninit()
{
}

#ifdef OS_WINDOWS
extern "C"
EXPORT WIN32_GRAPHICS_INIT(GraphicsInit)
{
    Global_Platform = Platform;
    InitMemory(Global_Platform->TempArena, Global_Platform->AllocateMemory, Global_Platform->FreeMemory);
    SetGlobalErrorStream(Global_Platform->ErrorStream);
    
#if DEVELOPER_BUILD
    ImGui::SetCurrentContext(Context);
#endif
    
    HMODULE VulkanLib = LoadLibrary("vulkan-1.dll");
    BOOL_CHECK_AND_HANDLE(VulkanLib, "Failed to load the vulkan library");    
    
    vkGetInstanceProcAddr  = (PFN_vkGetInstanceProcAddr)GetProcAddress(VulkanLib, "vkGetInstanceProcAddr");
    BOOL_CHECK_AND_HANDLE(vkGetInstanceProcAddr, "Failed to load the vulkan vkGetInstanceProcAddr function");
    
    BOOL_CHECK_AND_HANDLE(VulkanInit(Window, Assets), "Failed to initialize the core vulkan graphics.");    
    return GetVulkanGraphics();
    
    handle_error:
    VulkanUninit();
    return NULL;
}
#endif