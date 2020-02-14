#ifndef VULKAN_GRAPHICS_H
#define VULKAN_GRAPHICS_H

#define VK_NO_PROTOTYPES
#include "world_game.h"
#include "vulkan/vulkan.h"
#define VULKAN_FUNCTION(function) global PFN_##function function

#define LOAD_GLOBAL_FUNCTION(function) \
function = (PFN_##function)vkGetInstanceProcAddr(NULL, #function); \
BOOL_CHECK_AND_HANDLE(function, "Failed to load the vulkan %.*s function", LiteralStringLength(#function), #function)

#define LOAD_INSTANCE_FUNCTION(function) \
function = (PFN_##function)vkGetInstanceProcAddr(GetVulkanGraphics()->Instance, #function); \
BOOL_CHECK_AND_HANDLE(function, "Failed to load the vulkan %.*s function", LiteralStringLength(#function), #function)

#define VULKAN_CHECK_AND_HANDLE(check, message) \
do \
{ \
if((check) != VK_SUCCESS) \
WRITE_AND_HANDLE_ERROR(message); \
} \
while(0)

VULKAN_FUNCTION(vkGetInstanceProcAddr);

VULKAN_FUNCTION(vkCreateInstance);
VULKAN_FUNCTION(vkEnumerateInstanceExtensionProperties);
VULKAN_FUNCTION(vkEnumerateInstanceLayerProperties);

#ifdef OS_WINDOWS
#include "vulkan/vulkan_win32.h"
VULKAN_FUNCTION(vkCreateWin32SurfaceKHR);
#endif

VULKAN_FUNCTION(vkGetPhysicalDeviceSurfaceSupportKHR);
VULKAN_FUNCTION(vkEnumeratePhysicalDevices);
VULKAN_FUNCTION(vkGetPhysicalDeviceProperties);
VULKAN_FUNCTION(vkGetPhysicalDeviceQueueFamilyProperties);
VULKAN_FUNCTION(vkEnumerateDeviceExtensionProperties);

struct physical_device
{
    i32 GraphicsFamilyIndex;    
    i32 PresentFamilyIndex;
    VkPhysicalDevice Handle;    
};

struct physical_device_array
{
    u32 Count;
    physical_device* Ptr;    
};

struct vulkan_graphics : public graphics
{
    VkInstance Instance;
    physical_device_array GPUs;
    VkSurfaceKHR Surface;
    VkPhysicalDevice GPU;
    VkDevice Device;
};

global vulkan_graphics __Vulkan_Graphics__;

#if DEVELOPER_BUILD
struct developer_vulkan_graphics : public vulkan_graphics
{
};

global developer_vulkan_graphics __Developer_Vulkan_Graphics__;
#endif

inline vulkan_graphics* GetVulkanGraphics()
{
#if DEVELOPER_BUILD
    return &__Developer_Vulkan_Graphics__;
#else
    return &__Vulkan_Graphics__;
#endif
}

#endif