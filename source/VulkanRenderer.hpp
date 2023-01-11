///                                                                           
/// Langulus::Module::Vulkan                                                  
/// Copyright(C) 2020 Dimo Markov <langulusteam@gmail.com>                    
///                                                                           
/// Distributed under GNU General Public License v3+                          
/// See LICENSE file, or https://www.gnu.org/licenses                         
///                                                                           
#pragma once
#include "VRAM.hpp"
#include "VulkanPipeline.hpp"
#include "VulkanLayer.hpp"
#include "Content/VulkanGeometry.hpp"
#include "Content/VulkanTexture.hpp"
#include "Content/VulkanShader.hpp"

class MVulkan;

///                                                                           
///   THE VULKAN RENDERER COMPONENT                                           
///                                                                           
/// Binds with a window and renders to it                                     
/// Manages framebuffers, VRAM contents, and layers                           
///                                                                           
class CVulkanRenderer : public ARenderer, public TProducedFrom<MVulkan> {
private:
   Ptr<const AWindow> mWindow;

   // The logical device                                                
   Own<VkDevice> mDevice;
   // VRAM memory properties                                            
   VRAM mVRAM;
   // Physical device properties                                        
   VkPhysicalDeviceProperties mPhysicalProperties = {};
   // Physical device features                                          
   VkPhysicalDeviceFeatures mPhysicalFeatures = {};

   // Rendering surface                                                 
   Own<VkSurfaceKHR> mSurface;
   // Swap chain                                                        
   Own<VkSwapchainKHR> mSwapChain;
   std::vector<VkImage> mSwapChainImages;
   // Swap chain frames                                                 
   Frames mFrame;
   // Framebuffers                                                      
   FrameBuffers mFramebuffer;
   // Command buffers                                                   
   CmdBuffers mCommandBuffer;
   // Depth image                                                       
   VRAMImage mDepthImage;
   // Depth image view                                                  
   Own<VkImageView> mDepthImageView;
   // The main rendering pass                                           
   std::vector<VkAttachmentDescription> mPassAttachments;
   Own<VkRenderPass> mPass;

   // Fence for each framebuffer                                        
   std::vector<VkFence>   mNewBufferFence;
   // I think this is not used                                          
   Own<VkSemaphore> mNewFrameFence;
   // Frame finished                                                    
   Own<VkSemaphore> mFrameFinished;

   // Graphics family                                                   
   uint32_t mGraphicIndex = 0;
   // Presentation family                                               
   uint32_t mPresentIndex = 0;
   // Transfer family                                                   
   uint32_t mTransferIndex = 0;

   // For draw commands                                                 
   Own<VkCommandPool> mCommandPool;

   // For drawing                                                       
   Own<VkQueue> mRenderQueue;
   // For presenting                                                    
   Own<VkQueue> mPresentQueue;

   // Current frame index                                               
   uint32_t mCurrentFrame = 0;
   // Previous resolution (for detection)                               
   vec2 mResolution;
   // Renderer initialized                                              
   bool mRendererInitialized = false;

private:
   // Layers                                                            
   TFactory<CVulkanLayer> mLayers;
   // Pipelines                                                         
   TFactoryUnique<CVulkanPipeline> mPipelines;
   // Shader library                                                    
   TFactoryUnique<CVulkanShader> mShaders;
   // Geometry content mirror, keeps RAM/VRAM synchronized              
   TFactoryUnique<CVulkanGeometry> mGeometries;
   // Texture content mirror, keeps RAM/VRAM synchronized               
   TFactoryUnique<CVulkanTexture> mTextures;

public:
   REFLECT(CVulkanRenderer);
   CVulkanRenderer(MVulkan*);
   CVulkanRenderer(CVulkanRenderer&&) noexcept = default;
   CVulkanRenderer& operator = (CVulkanRenderer&&) noexcept = default;
   ~CVulkanRenderer();

   void Initialize(AWindow*) final;
   void Resize(const vec2&) final;
   void Update() final;
   void Uninitialize();

   PC_VERB(Create);
   PC_VERB(Interpret);

   /// Get the current command buffer                                         
   ///   @return the command buffer                                           
   NOD() inline auto& GetRenderCB() const noexcept {
      return mCommandBuffer[mCurrentFrame];
   }

   /// Get hardware dependent UBO outer alignment for dynamic buffers         
   ///   @return the alignment                                                
   NOD() inline pcptr GetOuterUBOAlignment() const noexcept {
      return static_cast<pcptr>(
         mPhysicalProperties.limits.minUniformBufferOffsetAlignment
      );
   }

   PC_SET(VRAM);
   PC_GET(Device);
   PC_GET(Resolution);
   PC_GET(Pass);
   PC_GET(PassAttachments);

   NOD() VkSurfaceFormatKHR GetSurfaceFormat() const noexcept;

   /// Cache RAM content                                                      
   ///   @param content - the RAM content to cache                            
   ///   @return VRAM content, or nullptr if can't cache it                   
   template<Unit T>
   NOD() auto Cache(const T* content) requires Dense<T> {
      if constexpr (pcHasBase<T, ATexture>)
         return mTextures.CreateInner(content);
      else if constexpr (pcHasBase<T, AGeometry>)
         return mGeometries.CreateInner(content);
      else LANGULUS_ASSERT("Can't cache RAM content in VRAM");
   }

   /// Merge VRAM content                                                     
   ///   @param content - the RAM content to cache                            
   ///   @return VRAM content, or nullptr if can't cache it                   
   template<Unit T>
   NOD() auto Cache(T&& content) requires Dense<T> {
      if constexpr (Same<T, CVulkanShader>)
         return mShaders.Merge(pcForward<T>(content));
      else LANGULUS_ASSERT("Can't merge content in VRAM");
   }

private:
   bool StartRendering();
   bool EndRendering();
   bool CreateSwapchain(const VkSurfaceFormatKHR&);
   bool RecreateSwapchain();
   void DestroySwapchain();
};
