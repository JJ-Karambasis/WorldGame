#include "vulkan_graphics.h"

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
           (Device->PresentFamilyIndex != -1))
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
        if(VKResult == VK_SUCCESS)
        {
            VkExtensionProperties* Extensions = PushArray(ExtensionCount, VkExtensionProperties, Clear, 0);
            VKResult = vkEnumerateDeviceExtensionProperties(GPU, VK_NULL_HANDLE, &ExtensionCount, Extensions);
            if(VKResult == VK_SUCCESS)
            {
                b32 FoundSwapchainExtension = false;
                for(u32 ExtensionIndex = 0; ExtensionIndex < ExtensionCount; ExtensionIndex++)
                {
                    if(StringEquals(Extensions[ExtensionIndex].extensionName, VK_KHR_SWAPCHAIN_EXTENSION_NAME))
                        FoundSwapchainExtension = true;
                }
                
                if(FoundSwapchainExtension)
                {                    
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
                    
                    if((Result.Ptr[GPUIndex].GraphicsFamilyIndex != -1) &&
                       (Result.Ptr[GPUIndex].PresentFamilyIndex != -1))
                    {
                        Passed = true;
                    }
                }                
            }
        }
        
        if(Passed)        
            CONSOLE_LOG("Device PASSED!\n");        
        else
            CONSOLE_LOG("Device FAILED!\n");
        
        CONSOLE_LOG("\n");
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

b32 VulkanInit(void* PlatformSurfaceInfo)
{
    vulkan_graphics* Graphics = GetVulkanGraphics();
    
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