#include "vulkan_graphics.h"

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

VkShaderModule CreateShader(char* Path)
{
    file_results ShaderFile = Global_Platform->ReadEntireFile(Path);
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
    
    memory_info MemoryInfo = GetImageMemoryInfo(DepthImage);        
    i32 MemoryTypeIndex = FindMemoryTypeIndex(&GPU->MemoryProperties, &MemoryInfo.Requirements, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    if(MemoryTypeIndex == -1)
        WRITE_AND_HANDLE_ERROR("Failed to find a valid memory type for the depth image.");    
    
    VkMemoryAllocateInfo AllocateInfo = {};
    AllocateInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    AllocateInfo.memoryTypeIndex = MemoryTypeIndex;
    AllocateInfo.allocationSize = MemoryInfo.Requirements.size;
    
    VkMemoryDedicatedAllocateInfo DedicatedAllocateInfo = {};
    if(MemoryInfo.DedicatedAllocation)
    {        
        DedicatedAllocateInfo.sType = VK_STRUCTURE_TYPE_MEMORY_DEDICATED_ALLOCATE_INFO;
        DedicatedAllocateInfo.image = DepthImage;
        AllocateInfo.pNext = &DedicatedAllocateInfo;
    }
    
    VkDeviceMemory DepthMemory;
    VULKAN_CHECK_AND_HANDLE(vkAllocateMemory(Graphics->Device, &AllocateInfo, VK_NULL_HANDLE, &DepthMemory),
                            "Failed to allocate the depth buffer memory.");
    
    VULKAN_CHECK_AND_HANDLE(vkBindImageMemory(Graphics->Device, DepthImage, DepthMemory, 0),
                            "Failed to bind the depth image to the depth memory.");
    
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
    
    VkDeviceCreateInfo DeviceInfo = {};
    DeviceInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    DeviceInfo.queueCreateInfoCount = DeviceQueueInfoCount;
    DeviceInfo.pQueueCreateInfos = DeviceQueueInfo;
    DeviceInfo.enabledExtensionCount = Extensions.Count;
    DeviceInfo.ppEnabledExtensionNames = Extensions.Ptr;
    //DeviceInfo.pEnabledFeatures = ;
    
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
        if((Device->GraphicsFamilyIndex != -1) &&
           (Device->PresentFamilyIndex != -1) &&
           (Device->SurfaceFormat.format != VK_FORMAT_UNDEFINED) &&
           (Device->ColorImageCount > 1))
        {
            return Device;
        }
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
    
    VULKAN_CHECK_AND_HANDLE(vkQueueWaitIdle(Graphics->GraphicsQueue), "Failed to wait for the graphics queue.");
    VULKAN_CHECK_AND_HANDLE(vkResetCommandPool(Graphics->Device, Graphics->CommandPool, 0), "Failed to reset the command pool.");
    
    VkCommandBuffer CommandBuffer = Graphics->CommandBuffer;
    
    VkCommandBufferBeginInfo CommandBufferBeginInfo = {};
    CommandBufferBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    CommandBufferBeginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    VULKAN_CHECK_AND_HANDLE(vkBeginCommandBuffer(CommandBuffer, &CommandBufferBeginInfo),
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
        
        vkCmdBeginRenderPass(CommandBuffer, &RenderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);
        {
            vkCmdBindPipeline(CommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, Graphics->Pipeline);
             
            vkCmdSetViewport(CommandBuffer, 0, 1, &VKViewport);
            vkCmdSetScissor(CommandBuffer, 0, 1, &RenderPassBeginInfo.renderArea);                    
            
            vkCmdDraw(CommandBuffer, 3, 1, 0, 0);
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

b32 VulkanInit(void* PlatformSurfaceInfo)
{
    vulkan_graphics* Graphics = GetVulkanGraphics();
    Graphics->RenderGame = RenderGame;    
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
    
    VkShaderModule TestBoxVertex   = CreateShader("shaders/vulkan/TestBoxVertex.spv");
    VkShaderModule TestBoxFragment = CreateShader("shaders/vulkan/TestBoxFragment.spv");
    
    BOOL_CHECK_AND_HANDLE(TestBoxVertex, "Failed to create the test box vertex shader.");
    BOOL_CHECK_AND_HANDLE(TestBoxFragment, "Failed to create the test box fragment shader.");
    
    VkPipelineLayoutCreateInfo PipelineLayoutInfo = {};
    PipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;    
    
    VULKAN_CHECK_AND_HANDLE(vkCreatePipelineLayout(Graphics->Device, &PipelineLayoutInfo, VK_NULL_HANDLE, &Graphics->PipelineLayout),
                            "Failed to create the pipeline layout.");
    
    VkPipelineShaderStageCreateInfo ShaderStages[2] = {};
    ShaderStages[0].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    ShaderStages[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
    ShaderStages[0].module = TestBoxVertex;
    ShaderStages[0].pName = "main";
    
    ShaderStages[1].sType  = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    ShaderStages[1].stage  = VK_SHADER_STAGE_FRAGMENT_BIT;
    ShaderStages[1].module = TestBoxFragment;
    ShaderStages[1].pName  = "main";    
    
    VkPipelineVertexInputStateCreateInfo VertexInputState = {};
    VertexInputState.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    
    VkPipelineInputAssemblyStateCreateInfo InputAssemblyState = {};
    InputAssemblyState.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    InputAssemblyState.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    
    VkPipelineViewportStateCreateInfo ViewportState = {};
    ViewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    ViewportState.viewportCount = 1;
    ViewportState.scissorCount = 1;
    
    VkPipelineRasterizationStateCreateInfo RasterizationState = {};
    RasterizationState.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    RasterizationState.polygonMode = VK_POLYGON_MODE_FILL;
    RasterizationState.cullMode = VK_CULL_MODE_BACK_BIT;
    RasterizationState.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
    RasterizationState.lineWidth = 1.0f;
    
    VkPipelineMultisampleStateCreateInfo MultisampleState = {};
    MultisampleState.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    MultisampleState.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
    
    VkPipelineColorBlendAttachmentState ColorBlendAttachment = {};
    ColorBlendAttachment.blendEnable = VK_FALSE;
    ColorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT|VK_COLOR_COMPONENT_G_BIT|VK_COLOR_COMPONENT_B_BIT|VK_COLOR_COMPONENT_A_BIT;
    
    VkPipelineColorBlendStateCreateInfo ColorBlendState = {};
    ColorBlendState.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    ColorBlendState.attachmentCount = 1;
    ColorBlendState.pAttachments = &ColorBlendAttachment;    
    
    VkPipelineDepthStencilStateCreateInfo DepthStencilState = {};
    DepthStencilState.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    DepthStencilState.depthTestEnable = VK_TRUE;
    DepthStencilState.depthWriteEnable = VK_TRUE;
    DepthStencilState.depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL;    
    
    VkDynamicState DynamicStates[2] = {VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR};    
    
    VkPipelineDynamicStateCreateInfo DynamicState = {};
    DynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    DynamicState.dynamicStateCount = ARRAYCOUNT(DynamicStates);
    DynamicState.pDynamicStates = DynamicStates;
    
    VkGraphicsPipelineCreateInfo GraphicsPipelineInfo = {};
    GraphicsPipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    GraphicsPipelineInfo.stageCount = ARRAYCOUNT(ShaderStages);
    GraphicsPipelineInfo.pStages = ShaderStages;
    GraphicsPipelineInfo.pVertexInputState = &VertexInputState;
    GraphicsPipelineInfo.pInputAssemblyState = &InputAssemblyState;
    GraphicsPipelineInfo.pViewportState = &ViewportState;
    GraphicsPipelineInfo.pRasterizationState = &RasterizationState;
    GraphicsPipelineInfo.pMultisampleState = &MultisampleState;
    GraphicsPipelineInfo.pColorBlendState = &ColorBlendState;
    GraphicsPipelineInfo.pDepthStencilState = &DepthStencilState;
    GraphicsPipelineInfo.pDynamicState = &DynamicState;
    GraphicsPipelineInfo.layout = Graphics->PipelineLayout;
    GraphicsPipelineInfo.renderPass = Graphics->RenderPass;
    GraphicsPipelineInfo.subpass = 0;    
    
    VULKAN_CHECK_AND_HANDLE(vkCreateGraphicsPipelines(Graphics->Device, VK_NULL_HANDLE, 1, &GraphicsPipelineInfo, VK_NULL_HANDLE, &Graphics->Pipeline),
                            "Failed to create the graphics pipeline.");
    
    vkDestroyShaderModule(Graphics->Device, TestBoxVertex, VK_NULL_HANDLE);
    vkDestroyShaderModule(Graphics->Device, TestBoxFragment, VK_NULL_HANDLE);
    
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
    
    HMODULE VulkanLib = LoadLibrary("vulkan-1.dll");
    BOOL_CHECK_AND_HANDLE(VulkanLib, "Failed to load the vulkan library");    
    
    vkGetInstanceProcAddr  = (PFN_vkGetInstanceProcAddr)GetProcAddress(VulkanLib, "vkGetInstanceProcAddr");
    BOOL_CHECK_AND_HANDLE(vkGetInstanceProcAddr, "Failed to load the vulkan vkGetInstanceProcAddr function");
    
    BOOL_CHECK_AND_HANDLE(VulkanInit(Window), "Failed to initialize the core vulkan graphics.");    
    return GetVulkanGraphics();
    
    handle_error:
    VulkanUninit();
    return NULL;
}
#endif