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


//IMPORTANT(EVERYONE): If we need to support stencil buffers remove the D32_SFLOAT only format
//NOTE(EVERYONE): Since we don't care about stencil formats right now, lets just prioritize higher precision depth values for now 
//(with less memory usage)
global const VkFormat Global_DepthBufferFormats[] =
{
    VK_FORMAT_D32_SFLOAT, 
    VK_FORMAT_D32_SFLOAT_S8_UINT, 
    VK_FORMAT_D24_UNORM_S8_UINT,
    VK_FORMAT_D16_UNORM, 
    VK_FORMAT_D16_UNORM_S8_UINT
};

VULKAN_FUNCTION(vkGetInstanceProcAddr);

VULKAN_FUNCTION(vkCreateInstance);
VULKAN_FUNCTION(vkEnumerateInstanceExtensionProperties);
VULKAN_FUNCTION(vkEnumerateInstanceLayerProperties);

#ifdef OS_WINDOWS
#include "vulkan/vulkan_win32.h"
VULKAN_FUNCTION(vkCreateWin32SurfaceKHR);
#endif

VULKAN_FUNCTION(vkCreateDebugUtilsMessengerEXT);
VULKAN_FUNCTION(vkGetPhysicalDeviceSurfaceSupportKHR);
VULKAN_FUNCTION(vkGetPhysicalDeviceSurfaceFormatsKHR);
VULKAN_FUNCTION(vkGetPhysicalDeviceSurfaceCapabilitiesKHR);
VULKAN_FUNCTION(vkEnumeratePhysicalDevices);
VULKAN_FUNCTION(vkGetPhysicalDeviceProperties);
VULKAN_FUNCTION(vkGetPhysicalDeviceQueueFamilyProperties);
VULKAN_FUNCTION(vkEnumerateDeviceExtensionProperties);
VULKAN_FUNCTION(vkCreateDevice);
VULKAN_FUNCTION(vkGetDeviceProcAddr);
VULKAN_FUNCTION(vkGetPhysicalDeviceFormatProperties);
VULKAN_FUNCTION(vkGetPhysicalDeviceMemoryProperties);

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
VULKAN_FUNCTION(vkCreateShaderModule);
VULKAN_FUNCTION(vkDestroyShaderModule);
VULKAN_FUNCTION(vkCreateGraphicsPipelines);
VULKAN_FUNCTION(vkCreatePipelineLayout);
VULKAN_FUNCTION(vkCmdSetViewport);
VULKAN_FUNCTION(vkCmdSetScissor);
VULKAN_FUNCTION(vkCmdBindPipeline);
VULKAN_FUNCTION(vkCmdDraw);
VULKAN_FUNCTION(vkCmdDrawIndexed);
VULKAN_FUNCTION(vkCreateImage);
VULKAN_FUNCTION(vkDestroyImage);
VULKAN_FUNCTION(vkGetImageMemoryRequirements);
VULKAN_FUNCTION(vkGetBufferMemoryRequirements);
VULKAN_FUNCTION(vkGetImageMemoryRequirements2KHR);
VULKAN_FUNCTION(vkGetBufferMemoryRequirements2KHR);
VULKAN_FUNCTION(vkAllocateMemory);
VULKAN_FUNCTION(vkFreeMemory);
VULKAN_FUNCTION(vkBindImageMemory);
VULKAN_FUNCTION(vkBindBufferMemory);
VULKAN_FUNCTION(vkCreateDescriptorSetLayout);
VULKAN_FUNCTION(vkCmdPushConstants);
VULKAN_FUNCTION(vkCreateDescriptorPool);
VULKAN_FUNCTION(vkAllocateDescriptorSets);
VULKAN_FUNCTION(vkCmdBindDescriptorSets);
VULKAN_FUNCTION(vkUpdateDescriptorSets);
VULKAN_FUNCTION(vkCreateBuffer);
VULKAN_FUNCTION(vkMapMemory);

struct extensions_array
{
    u32 Count;
    char** Ptr;
};

struct memory_info
{
    VkMemoryRequirements Requirements;
    b32 DedicatedAllocation;
};

struct physical_device
{
    b32 FoundDedicatedMemoryExtension;
    i32 GraphicsFamilyIndex;    
    i32 PresentFamilyIndex;
    VkSurfaceFormatKHR SurfaceFormat;
    VkFormat DepthFormat;    
    u32 ColorImageCount;
    VkPhysicalDeviceMemoryProperties MemoryProperties;
    VkPhysicalDevice Handle;    
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
    VkDeviceMemory DepthMemory;
    VkImage DepthImage;
    VkImageView DepthImageView;        
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
    VkDescriptorPool DescriptorPool;
    VkDescriptorSetLayout DescriptorSetLayout;
    VkDescriptorSet DescriptorSet;
    VkPipelineLayout PipelineLayout;
    VkPipeline Pipeline;    
    VkDeviceMemory CameraBufferMemory;
    VkBuffer CameraBuffer;
    m4* CameraBufferData;
    render_buffer RenderBuffer;  
};

global vulkan_graphics __Vulkan_Graphics__;

#if DEVELOPER_BUILD
struct developer_vulkan_graphics : public vulkan_graphics
{
    VkDebugUtilsMessengerEXT Messenger;
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