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

#define LOAD_DEVICE_FUNCTION(function) \
function = (PFN_##function)vkGetDeviceProcAddr(GetVulkanGraphics()->Device, #function); \
BOOL_CHECK_AND_HANDLE(function, "Failed to load the vulkan %.*s function", LiteralStringLength(#function), #function)

#define VULKAN_CHECK_AND_HANDLE(check, message, ...) \
do \
{ \
if((check) != VK_SUCCESS) \
WRITE_AND_HANDLE_ERROR(message, __VA_ARGS__); \
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
VULKAN_FUNCTION(vkGetPhysicalDeviceSurfaceFormatsKHR);
VULKAN_FUNCTION(vkGetPhysicalDeviceSurfaceCapabilitiesKHR);
VULKAN_FUNCTION(vkEnumeratePhysicalDevices);
VULKAN_FUNCTION(vkGetPhysicalDeviceProperties);
VULKAN_FUNCTION(vkGetPhysicalDeviceQueueFamilyProperties);
VULKAN_FUNCTION(vkEnumerateDeviceExtensionProperties);
VULKAN_FUNCTION(vkCreateDevice);
VULKAN_FUNCTION(vkGetDeviceProcAddr);

VULKAN_FUNCTION(vkGetDeviceQueue);
VULKAN_FUNCTION(vkCreateCommandPool);
VULKAN_FUNCTION(vkResetCommandPool);
VULKAN_FUNCTION(vkAllocateCommandBuffers);
VULKAN_FUNCTION(vkCreateRenderPass);
VULKAN_FUNCTION(vkAcquireNextImageKHR);
VULKAN_FUNCTION(vkQueueWaitIdle);
VULKAN_FUNCTION(vkDeviceWaitIdle);
VULKAN_FUNCTION(vkCreateSemaphore);
VULKAN_FUNCTION(vkDestroySemaphore);
VULKAN_FUNCTION(vkCreateSwapchainKHR);
VULKAN_FUNCTION(vkDestroySwapchainKHR);
VULKAN_FUNCTION(vkGetSwapchainImagesKHR);
VULKAN_FUNCTION(vkDestroyImageView);
VULKAN_FUNCTION(vkDestroyFramebuffer);
VULKAN_FUNCTION(vkCreateImageView);
VULKAN_FUNCTION(vkCreateFramebuffer);
VULKAN_FUNCTION(vkBeginCommandBuffer);
VULKAN_FUNCTION(vkCmdBeginRenderPass);
VULKAN_FUNCTION(vkCmdEndRenderPass);
VULKAN_FUNCTION(vkEndCommandBuffer);
VULKAN_FUNCTION(vkQueueSubmit);
VULKAN_FUNCTION(vkQueuePresentKHR);

struct physical_device
{    
    i32 GraphicsFamilyIndex;    
    i32 PresentFamilyIndex;
    VkSurfaceFormatKHR SurfaceFormat;
    VkPhysicalDevice Handle;    
    u32 ColorImageCount;
};

struct physical_device_array
{
    u32 Count;
    physical_device* Ptr;    
};

struct render_buffer
{    
    v2i Dimensions;
    VkSwapchainKHR Swapchain;
    u32 ColorImageCount;
    VkImage ColorImages[3];
    VkImageView ColorImageViews[3];
    VkFramebuffer Framebuffers[3];
};

struct vulkan_graphics : public graphics
{
    VkInstance Instance;
    physical_device_array GPUs;
    VkSurfaceKHR Surface;
    physical_device* SelectedGPU;
    VkQueue GraphicsQueue;
    VkQueue PresentQueue;
    VkCommandPool CommandPool;
    VkCommandBuffer CommandBuffer;
    VkRenderPass RenderPass;    
    VkDevice Device;
    VkSemaphore RenderLock;
    VkSemaphore PresentLock;
    render_buffer RenderBuffer;    
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