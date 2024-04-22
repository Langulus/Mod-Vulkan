///                                                                           
/// Langulus::Module::Vulkan                                                  
/// Copyright (c) 2020 Dimo Markov <team@langulus.com>                        
/// Part of the Langulus framework, see https://langulus.com                  
///                                                                           
/// Distributed under GNU General Public License v3+                          
/// See LICENSE file, or https://www.gnu.org/licenses                         
///                                                                           
#include "../Vulkan.hpp"
#include <Flow/Time.hpp>
#include <set>


/// Swapchain constructor                                                     
///   @param renderer - the renderer that owns the swapchain                  
VulkanSwapchain::VulkanSwapchain(VulkanRenderer& renderer) noexcept
   : mRenderer {renderer} {}

/// Create the swapchain                                                      
///   @param format - surface format                                          
///   @param families - set of queue families to use                          
void VulkanSwapchain::Create(const VkSurfaceFormatKHR& format, const QueueFamilies& families) {
   // Resolution                                                        
   const auto adapter = mRenderer.GetAdapter();
   const Real resx = (*mRenderer.mResolution)[0];
   const Real resy = (*mRenderer.mResolution)[1];
   const auto resxuint = static_cast<uint32_t>(resx);
   const auto resyuint = static_cast<uint32_t>(resy);
   if (resxuint == 0 or resyuint == 0)
      LANGULUS_OOPS(Graphics, "Bad resolution");

   // Create swap chain                                                 
   VkSurfaceCapabilitiesKHR surface_caps;
   memset(&surface_caps, 0, sizeof(VkSurfaceCapabilitiesKHR));
   std::vector<VkPresentModeKHR> surface_presentModes;
   if (vkGetPhysicalDeviceSurfaceCapabilitiesKHR(adapter, mRenderer.GetSurface(), &surface_caps))
      LANGULUS_OOPS(Graphics, "vkGetPhysicalDeviceSurfaceCapabilitiesKHR failed");

   // Check supported present modes                                     
   uint32_t presentModeCount;
   if (vkGetPhysicalDeviceSurfacePresentModesKHR(adapter, mRenderer.GetSurface(), &presentModeCount, nullptr))
      LANGULUS_OOPS(Graphics, "vkGetPhysicalDeviceSurfacePresentModesKHR failed");

   if (presentModeCount != 0) {
      surface_presentModes.resize(presentModeCount);
      if (vkGetPhysicalDeviceSurfacePresentModesKHR(adapter, mRenderer.GetSurface(), &presentModeCount, surface_presentModes.data()))
         LANGULUS_OOPS(Graphics, "vkGetPhysicalDeviceSurfacePresentModesKHR failed");
   }
   
   if (surface_presentModes.empty())
      LANGULUS_OOPS(Graphics, "Could not create swap chain");

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
   if (surface_caps.maxImageCount > 0 and imageCount > surface_caps.maxImageCount)
      imageCount = surface_caps.maxImageCount;

   // Setup the swapchain                                               
   VkSwapchainCreateInfoKHR swapInfo {};
   swapInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
   swapInfo.surface = mRenderer.GetSurface();
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

   // Allow us to copy swapchain images when testing                    
   IF_LANGULUS_TESTING(swapInfo.imageUsage |= VK_IMAGE_USAGE_TRANSFER_SRC_BIT);

   if (vkCreateSwapchainKHR(mRenderer.mDevice, &swapInfo, nullptr, &mSwapChain.Get()))
      LANGULUS_OOPS(Graphics, "Can't create swap chain");

   // Get images from swapchain                                         
   if (vkGetSwapchainImagesKHR(mRenderer.mDevice, mSwapChain, &imageCount, nullptr))
      LANGULUS_OOPS(Graphics, "vkGetSwapchainImagesKHR fails");

   TMany<VkImage> swapChainImages;
   swapChainImages.New(imageCount);
   if (vkGetSwapchainImagesKHR(mRenderer.mDevice, mSwapChain, &imageCount, swapChainImages.GetRaw()))
      LANGULUS_OOPS(Graphics, "vkGetSwapchainImagesKHR fails");

   // Create the image views for the swap chain. They will all be       
   // single layer, 2D images, with no mipmaps                          
   const auto count = imageCount;
   bool reverseFormat;
   const auto viewf = VkFormatToDMeta(format.format, reverseFormat);
   ImageView colorview {
      extent.width, extent.height, 1, 1, viewf, reverseFormat
   };

   mFrameViews.New(count);
   mFrameImages.New(count);

   for (uint32_t i = 0; i < count; i++) {
      auto& image = swapChainImages[i];
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

      // Add the view                                                   
      mFrameViews[i] = mRenderer.mVRAM.CreateImageView(
         image, colorview, VK_IMAGE_ASPECT_COLOR_BIT
      );

      // Add the image                                                  
      mFrameImages[i] = VulkanImage::FromSwapchain(
         mRenderer.mDevice, image, colorview
      );
   }
   
   // Create the depth buffer image and view                            
   ImageView depthview {
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
   mFrameBuffers.New(count);
   for (Offset i = 0; i < count; ++i) {
      VkImageView attachments[] {mFrameViews[i], mDepthImageView};
      VkFramebufferCreateInfo framebufferInfo {};
      framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
      framebufferInfo.renderPass = mRenderer.mPass;
      framebufferInfo.attachmentCount = 2;
      framebufferInfo.pAttachments = attachments;
      framebufferInfo.width = resxuint;
      framebufferInfo.height = resyuint;
      framebufferInfo.layers = 1;

      if (vkCreateFramebuffer(mRenderer.mDevice, &framebufferInfo, nullptr, &mFrameBuffers[i]))
         LANGULUS_OOPS(Graphics, "Can't create framebuffer");
   }

   // Create a command buffer for each framebuffer                      
   mCommandBuffer.resize(count);
   VkCommandBufferAllocateInfo allocInfo {};
   allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
   allocInfo.commandPool = mRenderer.mCommandPool;
   allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
   allocInfo.commandBufferCount = count;

   if (vkAllocateCommandBuffers(mRenderer.mDevice, &allocInfo, mCommandBuffer.data()))
      LANGULUS_OOPS(Graphics, "Can't create command buffers");

   // Create command buffer fences                                      
   if (not mNewBufferFence) {
      TMany<VkFenceCreateInfo> fenceInfo;
      fenceInfo.New(count);
      mNewBufferFence.Reserve(count);

      for (auto& it : fenceInfo) {
         it.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
         VkFence result {};
         if (vkCreateFence(mRenderer.mDevice, &it, nullptr, &result))
            LANGULUS_OOPS(Graphics, "Can't create buffer fence");

         mNewBufferFence << result;
      }
   }
   
   // Create sync primitives                                            
   VkSemaphoreCreateInfo fenceInfo {};
   fenceInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

   if (vkCreateSemaphore(mRenderer.mDevice, &fenceInfo, nullptr, &mNewFrameFence.Get()))
      LANGULUS_OOPS(Graphics, "Can't create new frame semaphore");

   VkSemaphoreCreateInfo semaphoreInfo {};
   semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

   if (vkCreateSemaphore(mRenderer.mDevice, &semaphoreInfo, nullptr, &mFrameFinished.Get()))
      LANGULUS_OOPS(Graphics, "Can't create frame finished semaphore");

   mCurrentFrame = 0;
}

/// Recreate the swapchain (usually on window resize)                         
///   @param families - a set of queue families                               
void VulkanSwapchain::Recreate(const QueueFamilies& families) {
   Destroy();

   try { Create(GetSurfaceFormat(), families); }
   catch (...) {
      Destroy();
      throw;
   }
}

/// Destroy the current swapchain and release all resource, that would        
/// otherwise cause circular references                                       
void VulkanSwapchain::Destroy() {
   mScreenshot.Reset();

   vkDeviceWaitIdle(mRenderer.mDevice);

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
   mDepthImageView.Reset();
   mRenderer.mVRAM.DestroyImage(mDepthImage);

   // Destroy framebuffers                                              
   for (auto& it : mFrameBuffers)
      vkDestroyFramebuffer(mRenderer.mDevice, it, nullptr);
   mFrameBuffers.Clear();
   
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
   for (auto& it : mFrameViews)
      vkDestroyImageView(mRenderer.mDevice, it, nullptr);
   mFrameViews.Clear();
   
   // Destroy swapchain                                                 
   vkDestroySwapchainKHR(mRenderer.mDevice, mSwapChain, nullptr);
   mSwapChain.Reset();
   mFrameImages.Clear();
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
   if (vkGetPhysicalDeviceSurfaceFormatsKHR(adapter, mRenderer.GetSurface(), &formatCount, nullptr))
      LANGULUS_OOPS(Graphics, "vkGetPhysicalDeviceSurfaceFormatsKHR failed");

   if (formatCount != 0) {
      surface_formats.resize(formatCount);
      if (vkGetPhysicalDeviceSurfaceFormatsKHR(adapter, mRenderer.GetSurface(), &formatCount, surface_formats.data()))
         LANGULUS_OOPS(Graphics, "vkGetPhysicalDeviceSurfaceFormatsKHR failed");
   }

   if (surface_formats.empty()) {
      Logger::Error(Self(), "Could not create swap chain");
      return error;
   }

   // Choose image format                                               
   VkSurfaceFormatKHR surfaceFormat {
      VK_FORMAT_MAX_ENUM, VK_COLOR_SPACE_MAX_ENUM_KHR
   };

   if (surface_formats.size() == 1 and surface_formats[0].format == VK_FORMAT_UNDEFINED) {
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
      LANGULUS_OOPS(Graphics, "Incompatible surface format and color space for swap chain");

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
   if (result == VK_ERROR_OUT_OF_DATE_KHR
   or (result and result != VK_SUBOPTIMAL_KHR)) {
      // Le strange error occurs                                        
      Logger::Error(Self(), "Vulkan failed to resize swapchain");
      return false;
   }

   // Turn the present source to color attachment                       
   mRenderer.mVRAM.ImageTransfer(
      mFrameImages[mCurrentFrame],
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
   VkSemaphore waitSemaphores[]   {mFrameFinished};
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
      mFrameImages[mCurrentFrame],
      VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, 
      VK_IMAGE_LAYOUT_PRESENT_SRC_KHR
   );

   if (vkQueuePresentKHR(mRenderer.mPresentQueue, &presentInfo)) {
      Logger::Error(Self(), "Vulkan failed to present - the frame will be lost");
   }

   return true;
}

///                                                                           
Text VulkanSwapchain::Self() const {
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
   return mFrameBuffers[mCurrentFrame];
}

/// Get currently bound swapchain image                                       
///   @return the image                                                       
const VulkanImage& VulkanSwapchain::GetCurrentImage() const noexcept {
   return mFrameImages[mCurrentFrame];
}

/// Take a screenshot                                                         
Ref<A::Image> VulkanSwapchain::TakeScreenshot() {
   // The stager is used to copy from current back buffer               
   // (VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL) to a                   
   // VK_BUFFER_USAGE_TRANSFER_DST_BIT buffer, that is later copied to  
   // a RAM buffer                                                      
   const auto source = GetCurrentImage();
   const auto bytesize = source.GetView().GetBytesize();
   auto& vram = mRenderer.mVRAM;
   auto stager = vram.CreateBuffer(
      nullptr, bytesize,
      VK_BUFFER_USAGE_TRANSFER_DST_BIT,
      VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
   );

   // Transfer provided source to a VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL
   mRenderer.mVRAM.ImageTransfer(
      source,
      VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
      VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL
   );

   // Copy bytes                                                        
   auto& cmdbuffer = vram.mTransferBuffer;
   VkCommandBufferBeginInfo beginInfo {};
   beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
   beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
   vkBeginCommandBuffer(cmdbuffer, &beginInfo);

   VkBufferImageCopy region {};
   region.bufferOffset = 0;
   region.bufferRowLength = 0;
   region.bufferImageHeight = 0;
   region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
   region.imageSubresource.mipLevel = 0;
   region.imageSubresource.baseArrayLayer = 0;
   region.imageSubresource.layerCount = 1;
   region.imageOffset = {0, 0, 0};
   region.imageExtent = {
      source.GetView().mWidth,
      source.GetView().mHeight,
      source.GetView().mDepth
   };

   vkCmdCopyImageToBuffer(
      cmdbuffer, source.GetImage(),
      VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, 
      stager.GetBuffer(),
      1, &region
   );
   vkEndCommandBuffer(cmdbuffer);

   // Submit                                                            
   VkSubmitInfo submitInfo {};
   submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
   submitInfo.commandBufferCount = 1;
   submitInfo.pCommandBuffers = &cmdbuffer;
   vkQueueSubmit(vram.mTransferer, 1, &submitInfo, VK_NULL_HANDLE);
   vkQueueWaitIdle(vram.mTransferer);

   // Transfer source back to its original layout                       
   mRenderer.mVRAM.ImageTransfer(
      source.GetImage(),
      VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
      VK_IMAGE_LAYOUT_PRESENT_SRC_KHR
   );

   // Copy the data from the buffer to a RAM texture                    
   Byte* input;
   auto err = vkMapMemory(
      stager.GetDevice(), stager.GetMemory(),
      0, VK_WHOLE_SIZE, 0,
      reinterpret_cast<void**>(&input)
   );
   LANGULUS_ASSERT(err == VK_SUCCESS, Graphics,
      "Failed to map memory for temp image");

   auto memory = Bytes::From(input, bytesize);

   // Don't forget to clean up the staging buffer                       
   vram.DestroyBuffer(stager);

   if (not mScreenshot) {
      // Create an image asset if not created yet, otherwise reuse it   
      // The image includes current timestamp to make it unique         
      // We also predefine the renderer as parent, because we don't     
      // want the image to be added as a component to a Thing. Parent   
      // will otherwise be added implicitly, by producing the image in  
      // the context of Thing that owns this renderer.                  
      Verbs::Create creator {Construct::From<A::Image>(
         Traits::Parent {Ref {&mRenderer}},
         source.GetView(),
         SteadyClock::Now()
      )};
      mScreenshot = mRenderer.RunIn(creator)->As<A::Image*>();
   }

   mScreenshot->Upload(Abandon(memory));
   return mScreenshot;
}