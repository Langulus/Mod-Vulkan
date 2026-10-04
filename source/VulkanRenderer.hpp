///                                                                           
/// Langulus::Module::Vulkan                                                  
/// Copyright (c) 2020 Dimo Markov <team@langulus.com>                        
/// Part of the Langulus framework, see https://langulus.com                  
///                                                                           
/// SPDX-License-Identifier: GPL-3.0-or-later                                 
///                                                                           
#pragma once
#include "VulkanPipeline.hpp"
#include "VulkanLayer.hpp"
#include "inner/VulkanMemory.hpp"
#include "inner/VulkanGeometry.hpp"
#include "inner/VulkanTexture.hpp"
#include "inner/VulkanShader.hpp"
#include "inner/VulkanSwapchain.hpp"
#include <Flow/Verbs/Create.hpp>
#include <Flow/Verbs/Interpret.hpp>
#include <Math/Gradient.hpp>
#include <Entity/Pin.hpp>


///                                                                           
///   Vulkan renderer                                                         
///                                                                           
/// Binds with a window and renders to it. Manages framebuffers, VRAM         
/// contents, and layers                                                      
///                                                                           
struct VulkanRenderer : Things::Renderer, ProducedFrom<Vulkan> {
   using CTTI_Abstract = No;
   using CTTI_Producer = Vulkan;
   LANGULUS_BASES(Things::Renderer);
   LANGULUS_VERBS(Verbs::Create, Verbs::Interpret);

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

protected:
   //                                                                   
   // Runtime updatable variables                                       
   //                                                                   
   
   // The platform window, where the renderer is created                
   Ref<const Things::Window> mWindow;
   // The time gradient, used for animations                            
   Ref<TGradient<Time>> mTime;
   // Mouse position, can be passed to shaders                          
   Ref<Grad2v2> mMousePosition;
   // Mouse scroll, can be passed to shaders                            
   Ref<Grad2v2> mMouseScroll;

   // Rendering surface                                                 
   Own<VkSurfaceKHR> mSurface;
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
   TMany<VkAttachmentDescription> mPassAttachments;
   Own<VkRenderPass> mPass;

   // Graphics family                                                   
   uint32_t mGraphicIndex {};
   // Presentation family                                               
   uint32_t mPresentIndex {};
   // Transfer family                                                   
   uint32_t mTransferIndex {};
   // The set of families                                               
   QueueFamilies mFamilies;

   // For draw commands                                                 
   Own<VkCommandPool> mCommandPool;

   // For drawing                                                       
   Own<VkQueue> mRenderQueue;
   // For presenting                                                    
   Own<VkQueue> mPresentQueue;

   // Previous resolution (for detecting change)                        
   Pin<Scale2> mResolution;

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
   VulkanRenderer(Vulkan*, Describe);
   ~VulkanRenderer();

   void Detach();
   void Refresh() override;

   void Create(Verb&);
   void Interpret(Verb&);

   void Draw();

   NOD() VkInstance GetVulkanInstance() const noexcept;
   NOD() VkPhysicalDevice GetAdapter() const noexcept;
   NOD() const Things::Window* GetWindow() const noexcept;
   NOD() Offset GetOuterUBOAlignment() const noexcept;
   NOD() VkCommandBuffer GetRenderCB() const noexcept;
   NOD() const Scale2& GetResolution() const noexcept;
   NOD() VkSurfaceKHR GetSurface() const noexcept;
};
