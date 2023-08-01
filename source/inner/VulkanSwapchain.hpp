///                                                                           
/// Langulus::Module::Vulkan                                                  
/// Copyright(C) 2020 Dimo Markov <langulusteam@gmail.com>                    
///                                                                           
/// Distributed under GNU General Public License v3+                          
/// See LICENSE file, or https://www.gnu.org/licenses                         
///                                                                           
#pragma once
#include "../Common.hpp"


///                                                                           
///   Vulkan swapchain                                                        
///                                                                           
struct VulkanSwapchain {
protected:
   friend struct VulkanRenderer;
   VulkanRenderer& mRenderer;
   Debug Self() const;

   // Rendering surface                                                 
   Own<VkSurfaceKHR> mSurface;
   // Swap chain                                                        
   Own<VkSwapchainKHR> mSwapChain;

   // Images for the swap chain framebuffers                            
   TAny<VulkanImage> mFrameImages;
   // Swap chain frame views                                            
   FrameViews mFrameViews;
   // Framebuffers                                                      
   FrameBuffers mFrameBuffers;
   // Fence for each framebuffer                                        
   TAny<VkFence> mNewBufferFence;

   // I think this is not used                                          
   Own<VkSemaphore> mNewFrameFence;
   // Frame finished                                                    
   Own<VkSemaphore> mFrameFinished;

   // Current frame index                                               
   uint32_t mCurrentFrame {};

   // Command buffers                                                   
   CmdBuffers mCommandBuffer;
   // Depth image                                                       
   VulkanImage mDepthImage;
   // Depth image view                                                  
   Own<VkImageView> mDepthImageView;

   // Always keep a reference to a screenshot, to avoid reallocation    
   Ref<A::Image> mScreenshot;

public:
   VulkanSwapchain() = delete;
   VulkanSwapchain(VulkanRenderer&) noexcept;
   ~VulkanSwapchain();

   void CreateSurface(const A::Window*);
   void Create(const VkSurfaceFormatKHR&, const QueueFamilies&);
   void Recreate(const QueueFamilies&);
   void Destroy();
   void DestroySurface();

   bool StartRendering();
   bool EndRendering();

   NOD() VkSurfaceFormatKHR GetSurfaceFormat() const noexcept;
   NOD() VkCommandBuffer GetRenderCB() const noexcept;
   NOD() VkFramebuffer GetFramebuffer() const noexcept;
   NOD() VkSurfaceKHR GetSurface() const noexcept;
   NOD() const VulkanImage& GetCurrentImage() const noexcept;
   NOD() Ref<A::Image> TakeScreenshot();
};
