///                                                                           
/// Langulus::Module::Vulkan                                                  
/// Copyright(C) 2020 Dimo Markov <langulusteam@gmail.com>                    
///                                                                           
/// Distributed under GNU General Public License v3+                          
/// See LICENSE file, or https://www.gnu.org/licenses                         
///                                                                           
#include "../Vulkan.hpp"
#include <set>

/// Swapchain constructor                                                     
///   @param renderer - the renderer that owns the swapchain                  
VulkanSwapchain::VulkanSwapchain(VulkanRenderer& renderer) noexcept
   : mRenderer {renderer} {}

/// Create a surface inside a native window                                   
///   @param window - the window                                              
void VulkanSwapchain::CreateSurface(const A::Window* window) {
   // Create surface                                                    
   if (!CreateNativeVulkanSurfaceKHR(mRenderer.GetVulkanInstance(), window, mSurface))
      LANGULUS_THROW(Graphics, "Error creating window surface");
}

/// Create the swapchain                                                      
///   @param format - surface format                                          
///   @param families - set of queue families to use                          
void VulkanSwapchain::Create(const VkSurfaceFormatKHR& format, const QueueFamilies& families) {
   // Resolution                                                        
   const auto adapter = mRenderer.GetAdapter();
   const Real resx = mRenderer.mResolution[0];
   const Real resy = mRenderer.mResolution[1];
   const auto resxuint = static_cast<uint32_t>(resx);
   const auto resyuint = static_cast<uint32_t>(resy);
   if (resxuint == 0 || resyuint == 0) {
      Destroy();
      LANGULUS_THROW(Graphics, "Bad resolution");
   }

   // Create swap chain                                                 
   VkSurfaceCapabilitiesKHR surface_caps;
   memset(&surface_caps, 0, sizeof(VkSurfaceCapabilitiesKHR));
   std::vector<VkPresentModeKHR> surface_presentModes;
   if (vkGetPhysicalDeviceSurfaceCapabilitiesKHR(adapter, mSurface, &surface_caps)) {
      Destroy();
      LANGULUS_THROW(Graphics, "vkGetPhysicalDeviceSurfaceCapabilitiesKHR failed");
   }

   // Check supported present modes                                     
   uint32_t presentModeCount;
   if (vkGetPhysicalDeviceSurfacePresentModesKHR(adapter, mSurface, &presentModeCount, nullptr)) {
      Destroy();
      LANGULUS_THROW(Graphics, "vkGetPhysicalDeviceSurfacePresentModesKHR failed");
   }

   if (presentModeCount != 0) {
      surface_presentModes.resize(presentModeCount);
      if (vkGetPhysicalDeviceSurfacePresentModesKHR(adapter, mSurface, &presentModeCount, surface_presentModes.data())) {
         Destroy();
         LANGULUS_THROW(Graphics, "vkGetPhysicalDeviceSurfacePresentModesKHR failed");
      }
   }
   
   if (surface_presentModes.empty()) {
      Destroy();
      LANGULUS_THROW(Graphics, "Could not create swap chain");
   }

   // Choose present mode                                               
   auto surfacePresentMode = VkPresentModeKHR::VK_PRESENT_MODE_FIFO_KHR;
   for (const auto& availablePresentMode : surface_presentModes) {
      if (availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR)
         surfacePresentMode = availablePresentMode;
   }

   VkExtent2D extent {resxuint, resyuint};
   if (surface_caps.currentExtent.width != VK_INDEFINITELY)
      extent = surface_caps.currentExtent;
   else {
      extent.width = Math::Max(
         surface_caps.minImageExtent.width, 
         Math::Min(surface_caps.maxImageExtent.width, extent.width)
      );
      extent.height = Math::Max(
         surface_caps.minImageExtent.height, 
         Math::Min(surface_caps.maxImageExtent.height, extent.height)
      );
   }

   uint32_t imageCount = surface_caps.minImageCount + 1;
   if (surface_caps.maxImageCount > 0 && imageCount > surface_caps.maxImageCount)
      imageCount = surface_caps.maxImageCount;

   // Setup the swapchain                                               
   VkSwapchainCreateInfoKHR swapInfo {};
   swapInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
   swapInfo.surface = mSurface;
   swapInfo.minImageCount = imageCount;
   swapInfo.imageFormat = format.format;
   swapInfo.imageColorSpace = format.colorSpace;
   swapInfo.imageExtent = extent;
   swapInfo.imageArrayLayers = 1;   // 2 if stereoscopic                
   swapInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
   swapInfo.imageSharingMode = families.GetCount() == 1
      ? VK_SHARING_MODE_EXCLUSIVE 
      : VK_SHARING_MODE_CONCURRENT;
   swapInfo.queueFamilyIndexCount = static_cast<uint32_t>(families.GetCount());
   swapInfo.pQueueFamilyIndices = families.GetRaw();
   swapInfo.preTransform = surface_caps.currentTransform;
   swapInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
   swapInfo.presentMode = surfacePresentMode;
   swapInfo.clipped = VK_TRUE;
   swapInfo.oldSwapchain = VK_NULL_HANDLE;

   if (vkCreateSwapchainKHR(mRenderer.mDevice, &swapInfo, nullptr, &mSwapChain.Get())) {
      Destroy();
      LANGULUS_THROW(Graphics, "Can't create swap chain");
   }

   // Get images inside swapchain                                       
   if (vkGetSwapchainImagesKHR(mRenderer.mDevice, mSwapChain, &imageCount, nullptr)) {
      Destroy();
      LANGULUS_THROW(Graphics, "vkGetSwapchainImagesKHR fails");
   }

   mSwapChainImages.New(imageCount);
   if (vkGetSwapchainImagesKHR(mRenderer.mDevice, mSwapChain, &imageCount, mSwapChainImages.GetRaw())) {
      Destroy();
      LANGULUS_THROW(Graphics, "vkGetSwapchainImagesKHR fails");
   }

   // Create the image views for the swap chain. They will all be       
   // single layer, 2D images, with no mipmaps.                         
   bool reverseFormat;
   const auto viewf = VkFormatToDMeta(format.format, reverseFormat);
   TextureView colorview {
      extent.width, extent.height, 1, 1, viewf, reverseFormat
   };

   mFrame.New(mSwapChainImages.GetCount());

   for (uint32_t i = 0; i < mSwapChainImages.GetCount(); i++) {
      auto& image = mSwapChainImages[i];
      mRenderer.mVRAM.ImageTransfer(
         image,
         VK_IMAGE_LAYOUT_UNDEFINED, 
         VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
      );
      mRenderer.mVRAM.ImageTransfer(
         image, 
         VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, 
         VK_IMAGE_LAYOUT_PRESENT_SRC_KHR
      );

      mFrame[i] = mRenderer.mVRAM.CreateImageView(
         image, colorview, VK_IMAGE_ASPECT_COLOR_BIT
      );
   }
   
   // Create the depth buffer image and view                            
   TextureView depthview {
      extent.width, extent.height, 1, 1, MetaOf<Depth32>()
   };
   mDepthImage = mRenderer.mVRAM.CreateImage(
      depthview, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT
   );
   mDepthImageView = mRenderer.mVRAM.CreateImageView(mDepthImage);
   mRenderer.mVRAM.ImageTransfer(mDepthImage,
      VK_IMAGE_LAYOUT_UNDEFINED, 
      VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL
   );
      
   // Create framebuffers                                               
   mFramebuffer.resize(mFrame.GetCount());
   for (Offset i = 0; i < mFrame.GetCount(); ++i) {
      VkImageView attachments[] {mFrame[i], mDepthImageView};
      VkFramebufferCreateInfo framebufferInfo {};
      framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
      framebufferInfo.renderPass = mRenderer.mPass;
      framebufferInfo.attachmentCount = 2;
      framebufferInfo.pAttachments = attachments;
      framebufferInfo.width = resxuint;
      framebufferInfo.height = resyuint;
      framebufferInfo.layers = 1;

      if (vkCreateFramebuffer(mRenderer.mDevice, &framebufferInfo, nullptr, &mFramebuffer[i])) {
         Destroy();
         LANGULUS_THROW(Graphics, "Can't create framebuffer");
      }
   }

   // Create a command buffer for each framebuffer                      
   mCommandBuffer.resize(mFramebuffer.size());
   VkCommandBufferAllocateInfo allocInfo {};
   allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
   allocInfo.commandPool = mRenderer.mCommandPool;
   allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
   allocInfo.commandBufferCount = uint32_t(mCommandBuffer.size());

   if (vkAllocateCommandBuffers(mRenderer.mDevice, &allocInfo, mCommandBuffer.data())) {
      Destroy();
      LANGULUS_THROW(Graphics, "Can't create command buffers");
   }

   // Create command buffer fences                                      
   if (mNewBufferFence.IsEmpty()) {
      TAny<VkFenceCreateInfo> fenceInfo;
      fenceInfo.New(mFramebuffer.size());
      mNewBufferFence.Reserve(mFramebuffer.size());

      for (auto& it : fenceInfo) {
         it.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
         VkFence result;
         if (vkCreateFence(mRenderer.mDevice, &it, nullptr, &result)) {
            Destroy();
            LANGULUS_THROW(Graphics, "Can't create buffer fence");
         }

         mNewBufferFence << result;
      }
   }
   
   // Create sync primitives                                            
   VkSemaphoreCreateInfo fenceInfo {};
   fenceInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

   if (vkCreateSemaphore(mRenderer.mDevice, &fenceInfo, nullptr, &mNewFrameFence.Get())) {
      Destroy();
      LANGULUS_THROW(Graphics, "Can't create new frame semaphore");
   }

   VkSemaphoreCreateInfo semaphoreInfo {};
   semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

   if (vkCreateSemaphore(mRenderer.mDevice, &semaphoreInfo, nullptr, &mFrameFinished.Get())) {
      Destroy();
      LANGULUS_THROW(Graphics, "Can't create frame finished semaphore");
   }

   mCurrentFrame = 0;
}

/// Recreate the swapchain (usually on window resize)                         
///   @param families - a set of queue families                               
void VulkanSwapchain::Recreate(const QueueFamilies& families) {
   vkDeviceWaitIdle(mRenderer.mDevice);
   Destroy();
   Create(GetSurfaceFormat(), families);
}

/// Destroy the current swapchain                                             
void VulkanSwapchain::Destroy() {
   if (mFrameFinished) {
      vkDestroySemaphore(mRenderer.mDevice, mFrameFinished, nullptr);
      mFrameFinished.Reset();
   }

   if (mNewFrameFence) {
      vkDestroySemaphore(mRenderer.mDevice, mNewFrameFence, nullptr);
      mNewFrameFence.Reset();
   }

   // Destroy fences                                                    
   for (auto& it : mNewBufferFence)
      vkDestroyFence(mRenderer.mDevice, it, nullptr);
   mNewBufferFence.Clear();

   // Destroy the depth buffer images                                   
   vkDestroyImageView(mRenderer.mDevice, mDepthImageView, nullptr);
   mDepthImageView = 0;
   mRenderer.mVRAM.DestroyImage(mDepthImage);

   // Destroy framebuffers                                              
   for (auto &it : mFramebuffer)
      vkDestroyFramebuffer(mRenderer.mDevice, it, nullptr);
   mFramebuffer.clear();
   
   // Destroy command buffers                                           
   if (mCommandBuffer.size()) {
      vkFreeCommandBuffers(
         mRenderer.mDevice,
         mRenderer.mCommandPool,
         static_cast<uint32_t>(mCommandBuffer.size()),
         mCommandBuffer.data()
      );
      mCommandBuffer.clear();
   }
   
   // Destroy image views                                               
   for (auto &it : mFrame)
      vkDestroyImageView(mRenderer.mDevice, it, nullptr);
   mFrame.Clear();
   
   // Destroy swapchain                                                 
   vkDestroySwapchainKHR(mRenderer.mDevice, mSwapChain, nullptr);
   mSwapChain = 0;
   mSwapChainImages.Clear();
}

VulkanSwapchain::~VulkanSwapchain() {
   if (mSurface)
      vkDestroySurfaceKHR(mRenderer.GetVulkanInstance(), mSurface, nullptr);
}

/// Get backbuffer surface format                                             
///   @return the surface format of the swap chain                            
VkSurfaceFormatKHR VulkanSwapchain::GetSurfaceFormat() const noexcept {
   // Check supported image formats                                     
   const VkSurfaceFormatKHR error {
      VK_FORMAT_MAX_ENUM, VK_COLOR_SPACE_MAX_ENUM_KHR
   };

   const auto adapter = mRenderer.GetAdapter();
   std::vector<VkSurfaceFormatKHR> surface_formats;
   uint32_t formatCount;
   if (vkGetPhysicalDeviceSurfaceFormatsKHR(adapter, mSurface, &formatCount, nullptr))
      LANGULUS_THROW(Graphics, "vkGetPhysicalDeviceSurfaceFormatsKHR failed");

   if (formatCount != 0) {
      surface_formats.resize(formatCount);
      if (vkGetPhysicalDeviceSurfaceFormatsKHR(adapter, mSurface, &formatCount, surface_formats.data()))
         LANGULUS_THROW(Graphics, "vkGetPhysicalDeviceSurfaceFormatsKHR failed");
   }

   if (surface_formats.empty()) {
      Logger::Error(Self(), "Could not create swap chain");
      return error;
   }

   // Choose image format                                               
   VkSurfaceFormatKHR surfaceFormat {
      VK_FORMAT_MAX_ENUM, VK_COLOR_SPACE_MAX_ENUM_KHR
   };

   if (surface_formats.size() == 1 && surface_formats[0].format == VK_FORMAT_UNDEFINED) {
      surfaceFormat = {
         VK_FORMAT_B8G8R8A8_UNORM,
         VK_COLOR_SPACE_SRGB_NONLINEAR_KHR
      };
   }
   else for (const auto& availableFormat : surface_formats) {
      if (availableFormat.format == VK_FORMAT_B8G8R8A8_UNORM) {
         surfaceFormat = availableFormat;
         break;
      }
   }

   if (surfaceFormat.colorSpace == VK_COLOR_SPACE_MAX_ENUM_KHR)
      LANGULUS_THROW(Graphics, "Incompatible surface format and color space for swap chain");

   return surfaceFormat;
}

/// Pick a back buffer and start writing the command buffer                   
///   @return true if something was rendered                                  
bool VulkanSwapchain::StartRendering() {
   // Set next frame from the swapchain                                 
   // This changes mCurrentFrame globally for this renderer             
   auto result = vkAcquireNextImageKHR(
      mRenderer.mDevice, mSwapChain, VK_INDEFINITELY,
      mFrameFinished, VK_NULL_HANDLE, &mCurrentFrame
   );

   // Check if resolution has changed                                   
   if (result == VK_ERROR_OUT_OF_DATE_KHR || (result && result != VK_SUBOPTIMAL_KHR)) {
      // Le strange error occurs                                        
      Logger::Error(Self(), "Vulkan failed to resize swapchain");
      return false;
   }

   // Turn the present source to color attachment                       
   mRenderer.mVRAM.ImageTransfer(
      mSwapChainImages[mCurrentFrame], 
      VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
      VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
   );

   // Begin writing to command buffer                                   
   VkCommandBufferBeginInfo beginInfo {};
   beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
   beginInfo.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;
   vkBeginCommandBuffer(mCommandBuffer[mCurrentFrame], &beginInfo);
   return true;
}

/// Submit command buffers and present                                        
///   @return true if something was rendered                                  
bool VulkanSwapchain::EndRendering() {
   // Command buffer ends                                               
   if (vkEndCommandBuffer(mCommandBuffer[mCurrentFrame])) {
      Logger::Error(Self(), "Can't end command buffer");
      return false;
   }

   // Submit command buffer to GPU                                      
   VkSemaphore waitSemaphores[] {mFrameFinished};
   VkSemaphore signalSemaphores[] {mNewFrameFence};
   VkPipelineStageFlags waitStages[] {
      VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT
   };

   VkSubmitInfo submitInfo {};
   submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
   submitInfo.waitSemaphoreCount = 1;
   submitInfo.pWaitSemaphores = waitSemaphores;
   submitInfo.pWaitDstStageMask = waitStages;
   submitInfo.commandBufferCount = 1;
   submitInfo.pCommandBuffers = &mCommandBuffer[mCurrentFrame];
   submitInfo.signalSemaphoreCount = 1;
   submitInfo.pSignalSemaphores = signalSemaphores;

   if (vkQueueSubmit(mRenderer.mRenderQueue, 1, &submitInfo, VK_NULL_HANDLE)) {
      Logger::Error(Self(), "Vulkan failed to submit render buffer");
      return false;
   }

   // Present and return                                                
   VkSwapchainKHR swapChains[] {mSwapChain};
   VkPresentInfoKHR presentInfo {};
   presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
   presentInfo.waitSemaphoreCount = 1;
   presentInfo.pWaitSemaphores = signalSemaphores;
   presentInfo.swapchainCount = 1;
   presentInfo.pSwapchains = swapChains;
   presentInfo.pImageIndices = &mCurrentFrame;

   mRenderer.mVRAM.ImageTransfer(
      mSwapChainImages[mCurrentFrame],
      VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, 
      VK_IMAGE_LAYOUT_PRESENT_SRC_KHR
   );

   if (vkQueuePresentKHR(mRenderer.mPresentQueue, &presentInfo)) {
      Logger::Error(Self(), "Vulkan failed to present - the frame will be lost");
   }

   return true;
}

///                                                                           
Debug VulkanSwapchain::Self() const {
   return mRenderer.Self();
}

/// Get the command buffer for the current frame                              
///   @return the command buffer                                              
VkCommandBuffer VulkanSwapchain::GetRenderCB() const noexcept {
   return mCommandBuffer[mCurrentFrame];
}

/// Get the frame buffer for the current frame                                
///   @return the frame buffer                                                
VkFramebuffer VulkanSwapchain::GetFramebuffer() const noexcept {
   return mFramebuffer[mCurrentFrame];
}

/// Get the native surface                                                    
///   @return the surface                                                     
VkSurfaceKHR VulkanSwapchain::GetSurface() const noexcept {
   return mSurface;
}