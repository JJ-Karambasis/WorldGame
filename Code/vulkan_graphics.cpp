#include "vulkan_graphics.h"

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
#if OS_WINDOWS
        if(StringEquals(InstanceExtensions[InstancePropertyIndex].extensionName, "VK_KHR_win32_surface"))
        {
            InstanceExtensionNames[InstanceExtensionCount++] = InstanceExtensions[InstancePropertyIndex].extensionName;
            FoundPlatformSurfaceExtension = true;
        }
#endif
        
#if DEVELOPER_BUILD
        if(StringEquals(InstanceExtensions[InstancePropertyIndex].extensionName, "VK_EXT_debug_utils"))
        {
            InstanceExtensionNames[InstanceExtensionCount++] = InstanceExtensions[InstancePropertyIndex].extensionName;
            FoundDebugExtensions = true;
        }        
#endif
    }
    
    BOOL_CHECK_AND_HANDLE(FoundPlatformSurfaceExtension, "Failed to find the platform surface extension! Cannot initialize vulkan.");    
    
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

b32 VulkanInit()
{
    vulkan_graphics* Graphics = GetVulkanGraphics();
    
    LOAD_GLOBAL_FUNCTION(vkCreateInstance);
    LOAD_GLOBAL_FUNCTION(vkEnumerateInstanceExtensionProperties);
    LOAD_GLOBAL_FUNCTION(vkEnumerateInstanceLayerProperties);
    
    Graphics->Instance = CreateInstance();
    BOOL_CHECK_AND_HANDLE(Graphics->Instance, "Failed to create the vulkan instance.");
    
    return true;
    
    handle_error:
    return false;
}

void VulkanUninit()
{
}

#if OS_WINDOWS
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
    
    BOOL_CHECK_AND_HANDLE(VulkanInit(), "Failed to initialize the core vulkan graphics.");    
    return GetVulkanGraphics();
    
    handle_error:
    VulkanUninit();
    return NULL;
}
#endif