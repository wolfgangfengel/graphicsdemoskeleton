/* Native Vulkan 1.0, Win32/x86, custom entry point and no C runtime.
 * One submitted frame at a time. Presentation has its own semaphore per image.
 * Static create-info scratch data avoids repeated zero-initialization code;
 * these small single-threaded hosts do not make concurrent API helper calls.
 */
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#define VK_USE_PLATFORM_WIN32_KHR
#include "../../Tools/Vulkan/include/vulkan/vulkan.h"
#ifdef DEMO_USE_SMOLV
#include "Smolv.h"
#endif

static VkInstance vkInstance;
static VkPhysicalDevice vkPhysical;
static VkDevice vkDevice;
static VkSurfaceKHR vkSurface;
static VkQueue vkQueue;
static UINT vkFamily, vkImageCount, vkImageIndex;
static VkSwapchainKHR vkSwapchain;
static VkImage vkImages[8];
static VkSemaphore vkAcquired, vkPresented[8];
static VkFence vkFence;
static VkCommandPool vkCommandPool;
static VkCommandBuffer vkCommands;
static VkFormat vkFormat;
static VkExtent2D vkExtent;
static VkPhysicalDeviceMemoryProperties vkMemoryProperties;
static HWND vkWindow;
static UINT vkMouseX;
static VkImageSubresourceRange vkColorRange={VK_IMAGE_ASPECT_COLOR_BIT,0,1,0,1};

static void vkCheck(VkResult result) { if(result<0) ExitProcess((UINT)result); }
static void vkRequire(int condition) { if(!condition) ExitProcess(1); }
#ifdef _DEBUG
static UINT vkErrors, vkWarnings;
static VkDebugUtilsMessengerEXT vkMessenger;
static PFN_vkDestroyDebugUtilsMessengerEXT vkDestroyMessenger;
static VkBool32 VKAPI_CALL vkMessage(VkDebugUtilsMessageSeverityFlagBitsEXT severity,VkDebugUtilsMessageTypeFlagsEXT type,const VkDebugUtilsMessengerCallbackDataEXT *data,void *user)
{
    (void)type; (void)user;
    if(severity&VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT) ++vkErrors;
    if(severity&VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT) ++vkWarnings;
    OutputDebugStringA(data->pMessage);
#ifdef VULKAN_TEST_MESSAGE
    VULKAN_TEST_MESSAGE(data->pMessage);
#endif
    return VK_FALSE;
}
static int vkEqual(const char *a,const char *b) { while(*a && *a==*b) {++a;++b;} return *a==*b; }
#endif

static void *vkArray(UINT count,UINT size) { void *p=HeapAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,count*size); vkRequire(p!=0); return p; }
static void vkFree(void *p) { HeapFree(GetProcessHeap(),0,p); }

static void vkInitialize(void)
{
    static const char *instanceExtensions[]={"VK_KHR_surface","VK_KHR_win32_surface","VK_EXT_debug_utils"};
    static const char *deviceExtension="VK_KHR_swapchain";
    static const float priority=1;
    static VkApplicationInfo app={VK_STRUCTURE_TYPE_APPLICATION_INFO};
    static VkInstanceCreateInfo instance={VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO};
    static VkWin32SurfaceCreateInfoKHR surface={VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR};
    static VkDeviceQueueCreateInfo queue={VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO};
    static VkDeviceCreateInfo device={VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO};
    static VkCommandPoolCreateInfo pool={VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO};
    static VkCommandBufferAllocateInfo commands={VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO};
    static VkFenceCreateInfo fence={VK_STRUCTURE_TYPE_FENCE_CREATE_INFO};
    static VkSemaphoreCreateInfo semaphore={VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO};
    VkPhysicalDevice *physical;
    UINT count,n;
    app.apiVersion=VK_API_VERSION_1_0;
    instance.pApplicationInfo=&app;
    instance.enabledExtensionCount=2; instance.ppEnabledExtensionNames=instanceExtensions;
#ifdef _DEBUG
    {
        static const char *layer="VK_LAYER_KHRONOS_validation";
        VkLayerProperties *layers;
        vkCheck(vkEnumerateInstanceLayerProperties(&count,0)); layers=vkArray(count,sizeof(*layers));
        vkCheck(vkEnumerateInstanceLayerProperties(&count,layers));
        for(n=0;n<count;++n) if(vkEqual(layers[n].layerName,layer)) { instance.enabledLayerCount=1; instance.ppEnabledLayerNames=&layer; instance.enabledExtensionCount=3; }
        vkFree(layers);
    }
#endif
    vkCheck(vkCreateInstance(&instance,0,&vkInstance));
#ifdef _DEBUG
    if(instance.enabledLayerCount) {
        PFN_vkGetInstanceProcAddr get=vkGetInstanceProcAddr;
        PFN_vkCreateDebugUtilsMessengerEXT create=(PFN_vkCreateDebugUtilsMessengerEXT)get(vkInstance,"vkCreateDebugUtilsMessengerEXT");
        static VkDebugUtilsMessengerCreateInfoEXT info={VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT};
        info.messageSeverity=VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT|VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
        info.messageType=VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT|VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT|VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
        info.pfnUserCallback=vkMessage;
        vkDestroyMessenger=(PFN_vkDestroyDebugUtilsMessengerEXT)get(vkInstance,"vkDestroyDebugUtilsMessengerEXT");
        vkCheck(create(vkInstance,&info,0,&vkMessenger));
    }
#endif
    surface.hinstance=GetModuleHandleW(0); surface.hwnd=vkWindow;
    vkCheck(vkCreateWin32SurfaceKHR(vkInstance,&surface,0,&vkSurface));
    vkCheck(vkEnumeratePhysicalDevices(vkInstance,&count,0)); physical=vkArray(count,sizeof(*physical));
    vkCheck(vkEnumeratePhysicalDevices(vkInstance,&count,physical));
    for(n=0;n<count && !vkPhysical;++n) {
        UINT f,families;
        VkQueueFamilyProperties *properties;
        vkGetPhysicalDeviceQueueFamilyProperties(physical[n],&families,0); properties=vkArray(families,sizeof(*properties));
        vkGetPhysicalDeviceQueueFamilyProperties(physical[n],&families,properties);
        for(f=0;f<families;++f) {
            VkBool32 present;
            vkCheck(vkGetPhysicalDeviceSurfaceSupportKHR(physical[n],f,vkSurface,&present));
            if(present && (properties[f].queueFlags&(VK_QUEUE_GRAPHICS_BIT|VK_QUEUE_COMPUTE_BIT))==(VK_QUEUE_GRAPHICS_BIT|VK_QUEUE_COMPUTE_BIT)) { vkPhysical=physical[n]; vkFamily=f; break; }
        }
        vkFree(properties);
    }
    vkFree(physical); vkRequire(vkPhysical!=0);
    vkGetPhysicalDeviceMemoryProperties(vkPhysical,&vkMemoryProperties);
    queue.queueFamilyIndex=vkFamily; queue.queueCount=1; queue.pQueuePriorities=&priority;
    device.queueCreateInfoCount=1; device.pQueueCreateInfos=&queue;
    device.enabledExtensionCount=1; device.ppEnabledExtensionNames=&deviceExtension;
    vkCheck(vkCreateDevice(vkPhysical,&device,0,&vkDevice)); vkGetDeviceQueue(vkDevice,vkFamily,0,&vkQueue);
    pool.queueFamilyIndex=vkFamily;
    vkCheck(vkCreateCommandPool(vkDevice,&pool,0,&vkCommandPool));
    commands.commandPool=vkCommandPool; commands.commandBufferCount=1;
    vkCheck(vkAllocateCommandBuffers(vkDevice,&commands,&vkCommands));
    fence.flags=VK_FENCE_CREATE_SIGNALED_BIT;
    vkCheck(vkCreateFence(vkDevice,&fence,0,&vkFence));
    vkCheck(vkCreateSemaphore(vkDevice,&semaphore,0,&vkAcquired));
}

static void vkDeleteSwapchain(void)
{
    UINT n;
    for(n=0;n<vkImageCount;++n) vkDestroySemaphore(vkDevice,vkPresented[n],0);
    if(vkSwapchain) vkDestroySwapchainKHR(vkDevice,vkSwapchain,0);
    vkSwapchain=0; vkImageCount=0;
}
static int vkMakeSwapchain(void)
{
    VkSurfaceCapabilitiesKHR caps;
    VkSurfaceFormatKHR *formats;
    static VkSwapchainCreateInfoKHR info={VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR};
    static VkSemaphoreCreateInfo semaphore={VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO};
    UINT count,n;
    vkCheck(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(vkPhysical,vkSurface,&caps));
    vkExtent=caps.currentExtent;
    if(vkExtent.width==0xffffffffu) { vkExtent.width=WINWIDTH; vkExtent.height=WINHEIGHT; }
    if(!vkExtent.width || !vkExtent.height) return 0;
    vkCheck(vkGetPhysicalDeviceSurfaceFormatsKHR(vkPhysical,vkSurface,&count,0));
    formats=vkArray(count,sizeof(*formats)); vkCheck(vkGetPhysicalDeviceSurfaceFormatsKHR(vkPhysical,vkSurface,&count,formats));
    vkFormat=VK_FORMAT_UNDEFINED;
    for(n=0;n<count;++n) if(formats[n].colorSpace==VK_COLOR_SPACE_SRGB_NONLINEAR_KHR && (formats[n].format==VK_FORMAT_R8G8B8A8_UNORM || formats[n].format==VK_FORMAT_B8G8R8A8_UNORM || formats[n].format==VK_FORMAT_UNDEFINED)) {
        vkFormat=formats[n].format==VK_FORMAT_UNDEFINED?VK_FORMAT_R8G8B8A8_UNORM:formats[n].format; break;
    }
    vkFree(formats); vkRequire(vkFormat!=VK_FORMAT_UNDEFINED);
#ifndef DEMO_SKELETON
    {
        VkFormatProperties properties;
        vkGetPhysicalDeviceFormatProperties(vkPhysical,vkFormat,&properties);
        vkRequire((properties.optimalTilingFeatures&VK_FORMAT_FEATURE_BLIT_DST_BIT)!=0);
    }
#endif
    info.surface=vkSurface; info.minImageCount=caps.minImageCount+1;
    if(caps.maxImageCount && info.minImageCount>caps.maxImageCount) info.minImageCount=caps.maxImageCount;
    info.imageFormat=vkFormat; info.imageColorSpace=VK_COLOR_SPACE_SRGB_NONLINEAR_KHR; info.imageExtent=vkExtent; info.imageArrayLayers=1;
    info.imageUsage=VK_IMAGE_USAGE_TRANSFER_DST_BIT|VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    vkRequire((caps.supportedUsageFlags&info.imageUsage)==info.imageUsage);
    info.preTransform=caps.currentTransform;
    info.compositeAlpha=(VkCompositeAlphaFlagBitsKHR)(caps.supportedCompositeAlpha&(~caps.supportedCompositeAlpha+1));
    info.presentMode=VK_PRESENT_MODE_FIFO_KHR; info.clipped=VK_TRUE;
#ifdef DEMO_FAST_PRESENT
    {
        VkPresentModeKHR *modes;
        vkCheck(vkGetPhysicalDeviceSurfacePresentModesKHR(vkPhysical,vkSurface,&count,0));
        modes=vkArray(count,sizeof(*modes));
        vkCheck(vkGetPhysicalDeviceSurfacePresentModesKHR(vkPhysical,vkSurface,&count,modes));
        // Mailbox allows fresh frames above the refresh rate without tearing.
        // Prefer it to immediate, and retain FIFO when neither is supported.
        for(n=0;n<count;++n) {
            if(modes[n]==VK_PRESENT_MODE_MAILBOX_KHR) {info.presentMode=modes[n];break;}
            if(modes[n]==VK_PRESENT_MODE_IMMEDIATE_KHR) info.presentMode=modes[n];
        }
        vkFree(modes);
    }
#endif
    vkCheck(vkCreateSwapchainKHR(vkDevice,&info,0,&vkSwapchain));
    vkCheck(vkGetSwapchainImagesKHR(vkDevice,vkSwapchain,&vkImageCount,0)); vkRequire(vkImageCount<=8);
    vkCheck(vkGetSwapchainImagesKHR(vkDevice,vkSwapchain,&vkImageCount,vkImages));
    for(n=0;n<vkImageCount;++n) vkCheck(vkCreateSemaphore(vkDevice,&semaphore,0,&vkPresented[n]));
    return 1;
}
static VkDeviceMemory vkAllocate(const VkMemoryRequirements *requirements,VkMemoryPropertyFlags flags)
{
    static VkMemoryAllocateInfo info={VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO};
    VkDeviceMemory memory;
    UINT n;
    for(n=0;n<vkMemoryProperties.memoryTypeCount;++n) if((requirements->memoryTypeBits&(1u<<n)) && (vkMemoryProperties.memoryTypes[n].propertyFlags&flags)==flags) break;
    vkRequire(n<vkMemoryProperties.memoryTypeCount);
    info.allocationSize=requirements->size; info.memoryTypeIndex=n;
    vkCheck(vkAllocateMemory(vkDevice,&info,0,&memory)); return memory;
}
typedef struct { VkBuffer handle; VkDeviceMemory memory; void *mapped; } VK_BUFFER;
typedef struct { VkImage handle; VkDeviceMemory memory; VkImageView view; } VK_IMAGE;
static VK_BUFFER vkBuffer(UINT size,VkBufferUsageFlags usage,int host)
{
    VK_BUFFER result={0};
    static VkBufferCreateInfo info={VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO};
    VkMemoryRequirements requirements;
    info.size=size; info.usage=usage;
    vkCheck(vkCreateBuffer(vkDevice,&info,0,&result.handle));
    vkGetBufferMemoryRequirements(vkDevice,result.handle,&requirements);
    result.memory=vkAllocate(&requirements,host?VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT|VK_MEMORY_PROPERTY_HOST_COHERENT_BIT:VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    vkCheck(vkBindBufferMemory(vkDevice,result.handle,result.memory,0));
    if(host) vkCheck(vkMapMemory(vkDevice,result.memory,0,VK_WHOLE_SIZE,0,&result.mapped));
    return result;
}
static VkImageView vkImageView(VkImage image,VkFormat format,VkImageAspectFlags aspect)
{
    static VkImageViewCreateInfo info={VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO};
    VkImageView view;
    info.image=image; info.viewType=VK_IMAGE_VIEW_TYPE_2D; info.format=format; info.subresourceRange=vkColorRange; info.subresourceRange.aspectMask=aspect;
    vkCheck(vkCreateImageView(vkDevice,&info,0,&view)); return view;
}
static VK_IMAGE vkImage(VkFormat format,VkImageUsageFlags usage,VkImageAspectFlags aspect)
{
    VK_IMAGE result={0};
    static VkImageCreateInfo info={VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO};
    VkMemoryRequirements requirements;
    VkFormatProperties properties;
    VkFormatFeatureFlags required=0;
    if(usage&VK_IMAGE_USAGE_STORAGE_BIT) required|=VK_FORMAT_FEATURE_STORAGE_IMAGE_BIT;
    if(usage&VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT) required|=VK_FORMAT_FEATURE_COLOR_ATTACHMENT_BIT;
    if(usage&VK_IMAGE_USAGE_TRANSFER_SRC_BIT) required|=VK_FORMAT_FEATURE_BLIT_SRC_BIT;
    vkGetPhysicalDeviceFormatProperties(vkPhysical,format,&properties);
    vkRequire((properties.optimalTilingFeatures&required)==required);
    info.imageType=VK_IMAGE_TYPE_2D; info.format=format; info.extent.width=WINWIDTH; info.extent.height=WINHEIGHT; info.extent.depth=1;
    info.mipLevels=info.arrayLayers=1; info.samples=VK_SAMPLE_COUNT_1_BIT; info.usage=usage;
    vkCheck(vkCreateImage(vkDevice,&info,0,&result.handle));
    vkGetImageMemoryRequirements(vkDevice,result.handle,&requirements);
    result.memory=vkAllocate(&requirements,VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    vkCheck(vkBindImageMemory(vkDevice,result.handle,result.memory,0));
    result.view=vkImageView(result.handle,format,aspect); return result;
}
static void vkDeleteBuffer(VK_BUFFER *buffer) { if(buffer->mapped) vkUnmapMemory(vkDevice,buffer->memory); vkDestroyBuffer(vkDevice,buffer->handle,0); vkFreeMemory(vkDevice,buffer->memory,0); }
static void vkDeleteImage(VK_IMAGE *image) { vkDestroyImageView(vkDevice,image->view,0); vkDestroyImage(vkDevice,image->handle,0); vkFreeMemory(vkDevice,image->memory,0); }
static VkShaderModule vkShader(const void *code,UINT bytes)
{
    static VkShaderModuleCreateInfo info={VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO}; VkShaderModule shader;
#ifdef DEMO_USE_SMOLV
    UINT size=smolSize(code,bytes);
    UINT *decoded;
    vkRequire(size!=0); decoded=vkArray(1,size);
    vkRequire(smolDecode(code,bytes,decoded,size));
    info.pCode=decoded; info.codeSize=size;
#else
    info.pCode=(const UINT *)code; info.codeSize=bytes;
#endif
    vkCheck(vkCreateShaderModule(vkDevice,&info,0,&shader));
#ifdef DEMO_USE_SMOLV
    vkFree(decoded);
#endif
    return shader;
}
static void vkBarrier(VkImage image,VkImageLayout before,VkImageLayout after,VkAccessFlags source,VkAccessFlags destination,VkPipelineStageFlags sourceStage,VkPipelineStageFlags destinationStage)
{
    static VkImageMemoryBarrier barrier={VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER};
    barrier.srcAccessMask=source; barrier.dstAccessMask=destination; barrier.oldLayout=before; barrier.newLayout=after;
    barrier.srcQueueFamilyIndex=barrier.dstQueueFamilyIndex=VK_QUEUE_FAMILY_IGNORED; barrier.image=image; barrier.subresourceRange=vkColorRange;
    vkCmdPipelineBarrier(vkCommands,sourceStage,destinationStage,0,0,0,0,0,1,&barrier);
}
