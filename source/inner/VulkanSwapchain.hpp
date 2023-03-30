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
   VulkanRenderer& mRenderer;
   Debug Self() const;

   // Rendering surface                                                 
   Own<VkSurfaceKHR> mSurface;
   // Swap chain                                                        
   Own<VkSwapchainKHR> mSwapChain;
   ::std::vector<VkImage> mSwapChainImages;
   // Swap chain frames                                                 
   Frames mFrame;
   // Framebuffers                                                      
   FrameBuffers mFramebuffer;

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

public:
   VulkanSwapchain() = delete;
   VulkanSwapchain(VulkanRenderer&);
   ~VulkanSwapchain();

   void CreateSurface(const A::Window*);
   void Create(const VkSurfaceFormatKHR&, const TAny<uint32_t>& families);
   void Recreate(const TAny<uint32_t>& families);
   void Destroy();
   void DestroySurface();

   bool StartRendering();
   bool EndRendering();

   NOD() VkSurfaceFormatKHR GetSurfaceFormat() const noexcept;
   NOD() VkCommandBuffer GetRenderCB() const noexcept;
   NOD() VkSurfaceKHR GetSurface() const noexcept;
};
