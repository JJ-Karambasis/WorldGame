#include "vulkan_graphics.h"

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
        
        VkFramebufferCreateInfo FramebufferInfo = {};
        FramebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        FramebufferInfo.renderPass = Graphics->RenderPass;
        FramebufferInfo.attachmentCount = 1;
        FramebufferInfo.pAttachments = &ColorImageViews[ImageIndex];
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
    
    char* RequiredDeviceExtensions[] = 
    {
        VK_KHR_SWAPCHAIN_EXTENSION_NAME
    };
    
    VkDeviceCreateInfo DeviceInfo = {};
    DeviceInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    DeviceInfo.queueCreateInfoCount = DeviceQueueInfoCount;
    DeviceInfo.pQueueCreateInfos = DeviceQueueInfo;
    DeviceInfo.enabledExtensionCount = ARRAYCOUNT(RequiredDeviceExtensions);
    DeviceInfo.ppEnabledExtensionNames = RequiredDeviceExtensions;    
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
    b32 FoundDebugExtensions = false;
    
    u32 InstanceExtensionCount = 0;
    char** InstanceExtensionNames = PushArray(InstancePropertyCount, char*, Clear, 0);
    for(u32 InstancePropertyIndex = 0; InstancePropertyIndex < InstancePropertyCount; InstancePropertyIndex++)
    {
#ifdef OS_WINDOWS
        if(StringEquals(InstanceExtensions[InstancePropertyIndex].extensionName, VK_KHR_WIN32_SURFACE_EXTENSION_NAME))
        {
            InstanceExtensionNames[InstanceExtensionCount++] = InstanceExtensions[InstancePropertyIndex].extensionName;
            FoundPlatformSurfaceExtension = true;
        }
#endif
        
#if DEVELOPER_BUILD
        if(StringEquals(InstanceExtensions[InstancePropertyIndex].extensionName, VK_EXT_DEBUG_UTILS_EXTENSION_NAME))
        {
            InstanceExtensionNames[InstanceExtensionCount++] = InstanceExtensions[InstancePropertyIndex].extensionName;
            FoundDebugExtensions = true;
        }        
#endif
    }
    
    BOOL_CHECK_AND_HANDLE(FoundPlatformSurfaceExtension, "Failed to find the platform surface extension! Cannot initialize vulkan.");    
    
    //NOTE(EVERYONE): Automatically included if the platform dependent extension is found (e.g. windows is VK_KHR_win32_surface)
    InstanceExtensionNames[InstanceExtensionCount++] = "VK_KHR_surface";
    
    VkInstanceCreateInfo InstanceInfo = {};
    InstanceInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    InstanceInfo.pApplicationInfo = &ApplicationInfo;
    InstanceInfo.enabledExtensionCount = InstanceExtensionCount;
    InstanceInfo.ppEnabledExtensionNames = InstanceExtensionNames;
    
    VkInstance Instance;
    VULKAN_CHECK_AND_HANDLE(vkCreateInstance(&InstanceInfo, VK_NULL_HANDLE, &Instance),
                            "Failed to create the vulkan instance.");    
    
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
    
    VkAttachmentDescription AttachmentDescriptions[1] = {};    
    AttachmentDescriptions[0].format = Graphics->SelectedGPU->SurfaceFormat.format;
    AttachmentDescriptions[0].samples = VK_SAMPLE_COUNT_1_BIT;
    AttachmentDescriptions[0].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    AttachmentDescriptions[0].storeOp = VK_ATTACHMENT_STORE_OP_STORE;        
    AttachmentDescriptions[0].initialLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    AttachmentDescriptions[0].finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;    
    
    VkAttachmentReference ColorAttachmentRef = {};
    ColorAttachmentRef.attachment = 0;
    ColorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    
    VkSubpassDescription SubpassDescriptions[1] = {};
    SubpassDescriptions[0].pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    SubpassDescriptions[0].colorAttachmentCount = 1;
    SubpassDescriptions[0].pColorAttachments = &ColorAttachmentRef;
    
    VkRenderPassCreateInfo RenderPassInfo = {};
    RenderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    RenderPassInfo.attachmentCount = ARRAYCOUNT(AttachmentDescriptions);
    RenderPassInfo.pAttachments = AttachmentDescriptions;
    RenderPassInfo.subpassCount = ARRAYCOUNT(SubpassDescriptions);
    RenderPassInfo.pSubpasses = SubpassDescriptions;
    
    VULKAN_CHECK_AND_HANDLE(vkCreateRenderPass(Graphics->Device, &RenderPassInfo, VK_NULL_HANDLE, &Graphics->RenderPass),
                            "Failed to create the render pass.");
    
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
    
    vkGetInstanceProcAddr = (PFN_vkGetInstanceProcAddr)GetProcAddress(VulkanLib, "vkGetInstanceProcAddr");
    BOOL_CHECK_AND_HANDLE(vkGetInstanceProcAddr, "Failed to load the vulkan vkGetInstanceProcAddr function");
    
    BOOL_CHECK_AND_HANDLE(VulkanInit(Window), "Failed to initialize the core vulkan graphics.");    
    return GetVulkanGraphics();
    
    handle_error:
    VulkanUninit();
    return NULL;
}
#endif