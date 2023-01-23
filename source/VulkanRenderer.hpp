///                                                                           
/// Langulus::Module::Vulkan                                                  
/// Copyright(C) 2020 Dimo Markov <langulusteam@gmail.com>                    
///                                                                           
/// Distributed under GNU General Public License v3+                          
/// See LICENSE file, or https://www.gnu.org/licenses                         
///                                                                           
#pragma once
#include "VulkanPipeline.hpp"
#include "VulkanLayer.hpp"
#include "inner/VulkanMemory.hpp"
#include "inner/VulkanGeometry.hpp"
#include "inner/VulkanTexture.hpp"
#include "inner/VulkanShader.hpp"


///                                                                           
///   Vulkan renderer                                                         
///                                                                           
/// Binds with a window and renders to it. Manages framebuffers, VRAM         
/// contents, and layers                                                      
///                                                                           
struct VulkanRenderer : A::Graphics, ProducedFrom<Vulkan> {
   LANGULUS(ABSTRACT) false;
   LANGULUS_BASES(A::Graphics);
   LANGULUS_VERBS(Verbs::Create);

protected:
   friend struct UBO;
   friend struct SamplerUBO;
   friend struct VulkanPipeline;
   friend struct VulkanShader;
   friend struct VulkanGeometry;
   friend struct VulkanTexture;

   // The platform window, where the renderer is created                
   Ptr<const Unit> mWindow;

   // The logical device                                                
   Own<VkDevice> mDevice;
   // VRAM memory properties                                            
   VulkanMemory mVRAM;
   // Physical device properties                                        
   VkPhysicalDeviceProperties mPhysicalProperties {};
   // Physical device features                                          
   VkPhysicalDeviceFeatures mPhysicalFeatures {};

   // Rendering surface                                                 
   Own<VkSurfaceKHR> mSurface;
   // Swap chain                                                        
   Own<VkSwapchainKHR> mSwapChain;
   ::std::vector<VkImage> mSwapChainImages;
   // Swap chain frames                                                 
   Frames mFrame;
   // Framebuffers                                                      
   FrameBuffers mFramebuffer;
   // Command buffers                                                   
   CmdBuffers mCommandBuffer;
   // Depth image                                                       
   VulkanImage mDepthImage;
   // Depth image view                                                  
   Own<VkImageView> mDepthImageView;
   // The main rendering pass                                           
   TAny<VkAttachmentDescription> mPassAttachments;
   Own<VkRenderPass> mPass;

   // Fence for each framebuffer                                        
   TAny<VkFence> mNewBufferFence;
   // I think this is not used                                          
   Own<VkSemaphore> mNewFrameFence;
   // Frame finished                                                    
   Own<VkSemaphore> mFrameFinished;

   // Graphics family                                                   
   uint32_t mGraphicIndex {};
   // Presentation family                                               
   uint32_t mPresentIndex {};
   // Transfer family                                                   
   uint32_t mTransferIndex {};

   // For draw commands                                                 
   Own<VkCommandPool> mCommandPool;

   // For drawing                                                       
   Own<VkQueue> mRenderQueue;
   // For presenting                                                    
   Own<VkQueue> mPresentQueue;

   // Current frame index                                               
   uint32_t mCurrentFrame {};
   // Previous resolution (for detecting change)                        
   Vec2 mResolution;
   // Renderer initialized                                              
   bool mRendererInitialized {};

   // Layers                                                            
   TFactory<VulkanLayer> mLayers;
   // Pipelines                                                         
   TFactoryUnique<VulkanPipeline> mPipelines;
   // Shader library                                                    
   TFactoryUnique<VulkanShader> mShaders;
   // Geometry content mirror, keeps RAM/VRAM synchronized              
   TFactoryUnique<VulkanGeometry> mGeometries;
   // Texture content mirror, keeps RAM/VRAM synchronized               
   TFactoryUnique<VulkanTexture> mTextures;

public:
   VulkanRenderer(Vulkan*, const Any&);
   ~VulkanRenderer();

   void Create(Verb&);

   void Draw();
   void Initialize(Unit*);
   void Resize(const Vec2&);
   void Uninitialize();

   /// Get the current command buffer                                         
   ///   @return the command buffer                                           
   NOD() auto& GetRenderCB() const noexcept {
      return mCommandBuffer[mCurrentFrame];
   }

   /// Get hardware dependent UBO outer alignment for dynamic buffers         
   ///   @return the alignment                                                
   NOD() Size GetOuterUBOAlignment() const noexcept {
      return static_cast<Size>(
         mPhysicalProperties.limits.minUniformBufferOffsetAlignment
      );
   }

   NOD() VkSurfaceFormatKHR GetSurfaceFormat() const noexcept;

   /// Cache RAM content                                                      
   ///   @param content - the RAM content to cache                            
   ///   @return VRAM content, or nullptr if can't cache it                   
   /*template<CT::Content T>
   NOD() auto Cache(const T* content) requires CT::Dense<T> {
      if constexpr (CT::Texture<T>)
         return mTextures.CreateInner(content);
      else if constexpr (CT::Geometry<T>)
         return mGeometries.CreateInner(content);
      else
         LANGULUS_ERROR("Can't cache RAM content to VRAM");
   }

   /// Merge VRAM content                                                     
   ///   @param content - the RAM content to cache                            
   ///   @return VRAM content, or nullptr if can't cache it                   
   template<CT::Unit T>
   NOD() auto Cache(T&& content) requires CT::Dense<T> {
      if constexpr (CT::DerivedFrom<T, VulkanShader>)
         return mShaders.Merge(Forward<T>(content));
      else
         LANGULUS_ERROR("Can't merge content in VRAM");
   }*/

private:
   bool StartRendering();
   bool EndRendering();
   bool CreateSwapchain(const VkSurfaceFormatKHR&);
   bool RecreateSwapchain();
   void DestroySwapchain();
};
