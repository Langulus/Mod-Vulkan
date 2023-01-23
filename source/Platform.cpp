///                                                                           
/// Langulus::Module::Vulkan                                                  
/// Copyright(C) 2020 Dimo Markov <langulusteam@gmail.com>                    
///                                                                           
/// Distributed under GNU General Public License v3+                          
/// See LICENSE file, or https://www.gnu.org/licenses                         
///                                                                           
#include "Common.hpp"

/// Make sure that vulkan headers are included properly,                      
/// by defining the same symbols for each include of this file                
#if LANGULUS_OS(WINDOWS)
   #define VK_USE_PLATFORM_WIN32_KHR
#elif LANGULUS_OS(LINUX)
   #define VK_USE_PLATFORM_XLIB_KHR
#else 
   #error "Specify your OS extensions for Vulkan"
#endif

#include <vulkan/vulkan.h>

#if LANGULUS_OS(WINDOWS)
   bool CreateNativeVulkanSurfaceKHR(const VkInstance& instance, const void* window, VkSurfaceKHR& surface) {
      VkWin32SurfaceCreateInfoKHR createInfo;
      memset(&createInfo, 0, sizeof(VkWin32SurfaceCreateInfoKHR));
      createInfo.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
      createInfo.hwnd = reinterpret_cast<HWND>(const_cast<void*>(window));
      createInfo.hinstance = GetModuleHandle(nullptr);
      auto CreateWin32SurfaceKHR = (PFN_vkCreateWin32SurfaceKHR)
         vkGetInstanceProcAddr(instance, "vkCreateWin32SurfaceKHR");

      if (!CreateWin32SurfaceKHR || CreateWin32SurfaceKHR(instance, &createInfo, nullptr, &surface) != VK_SUCCESS)
         return false;
      return true;
   }
#elif LANGULUS_OS(LINUX)
   bool CreateNativeVulkanSurfaceKHR(const VkInstance& instance, const AWindow* window, VkSurfaceKHR& surface) {
      VkXlibSurfaceCreateInfoKHR createInfo;
      memset(&createInfo, 0, sizeof(VkXlibSurfaceCreateInfoKHR));
      createInfo.sType = VK_STRUCTURE_TYPE_XLIB_SURFACE_CREATE_INFO_KHR;
      createInfo.window = reinterpret_cast<Window>(window);
      auto CreateXLIBSurfaceKHR = (PFN_vkCreateXlibSurfaceKHR)
         vkGetInstanceProcAddr(instance, "PFN_vkCreateXlibSurfaceKHR");

      if (!CreateXLIBSurfaceKHR || CreateXLIBSurfaceKHR(instance, &createInfo, nullptr, &surface) != VK_SUCCESS)
         return false;
      return true;
   }
#else 
   #error "You need to specify the pcCreateNativeVulkanSurfaceKHR function for your OS"
#endif

/// Get required extension layers                                             
///   @return a set of all the required extensions                            
TokenSet GetRequiredExtensions() {
   std::vector<const char*> extensions;
   #if LANGULUS_DEBUG()
      extensions.push_back(VK_EXT_DEBUG_REPORT_EXTENSION_NAME);
   #endif

   extensions.push_back(VK_KHR_SURFACE_EXTENSION_NAME);

   #if LANGULUS_OS(WINDOWS)
      extensions.push_back(VK_KHR_WIN32_SURFACE_EXTENSION_NAME);
   #endif
   return extensions;
}