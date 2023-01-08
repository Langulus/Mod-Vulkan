#include "IncludeVulkan.hpp"

#if LANGULUS_OS_IS(WINDOWS)
	bool pcCreateNativeVulkanSurfaceKHR(const VkInstance& instance, const AWindow* window, VkSurfaceKHR& surface) {
		// Create surface																
		VkWin32SurfaceCreateInfoKHR createInfo;
		memset(&createInfo, 0, sizeof(VkWin32SurfaceCreateInfoKHR));
		createInfo.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
		createInfo.hwnd = reinterpret_cast<HWND>(const_cast<void*>(window->GetNativeWindowHandle().Get()));
		createInfo.hinstance = GetModuleHandle(nullptr);
		auto CreateWin32SurfaceKHR = (PFN_vkCreateWin32SurfaceKHR)vkGetInstanceProcAddr(instance, "vkCreateWin32SurfaceKHR");
		if (!CreateWin32SurfaceKHR || CreateWin32SurfaceKHR(instance, &createInfo, nullptr, &surface) != VK_SUCCESS)
			return false;
		return true;
	}
#elif LANGULUS_OS_IS(LINUX)
	bool pcCreateNativeVulkanSurfaceKHR(const VkInstance& instance, const AWindow* window, VkSurfaceKHR& surface) {
		// Create surface																
		VkXlibSurfaceCreateInfoKHR createInfo;
		memset(&createInfo, 0, sizeof(VkXlibSurfaceCreateInfoKHR));
		createInfo.sType = VK_STRUCTURE_TYPE_XLIB_SURFACE_CREATE_INFO_KHR;
		createInfo.window = pcReinterpret<Window>(window->GetNativeWindowHandle().Get());
		auto CreateXLIBSurfaceKHR = (PFN_vkCreateXlibSurfaceKHR)vkGetInstanceProcAddr(instance, "PFN_vkCreateXlibSurfaceKHR");
		if (!CreateXLIBSurfaceKHR || CreateXLIBSurfaceKHR(instance, &createInfo, nullptr, &surface) != VK_SUCCESS)
			return false;
		return true;
	}
#else 
	#error "You need to specify the pcCreateNativeVulkanSurfaceKHR function for your OS"
#endif