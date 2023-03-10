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
#include "inner/VulkanSwapchain.hpp"


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
   friend struct VulkanCamera;
   friend struct VulkanSwapchain;
   friend struct VulkanLayer;

   // The platform window, where the renderer is created                
   Ptr<const A::Window> mWindow;

   // The logical device                                                
   Own<VkDevice> mDevice;
   // VRAM memory properties                                            
   VulkanMemory mVRAM;
   // Physical device properties                                        
   VkPhysicalDeviceProperties mPhysicalProperties {};
   // Physical device features                                          
   VkPhysicalDeviceFeatures mPhysicalFeatures {};

   // The swapchain interface                                           
   VulkanSwapchain mSwapchain;

   // The main rendering pass                                           
   TAny<VkAttachmentDescription> mPassAttachments;
   Own<VkRenderPass> mPass;

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

   // Previous resolution (for detecting change)                        
   Pinnable<Vec2> mResolution;

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
   void Resize(const Vec2&);

   NOD() VkInstance GetVulkanInstance() const noexcept;
   NOD() VkPhysicalDevice GetAdapter() const noexcept;
   NOD() Size GetOuterUBOAlignment() const noexcept;
   NOD() VkCommandBuffer GetRenderCB() const noexcept;
};
