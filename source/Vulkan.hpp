///                                                                           
/// Langulus::Module::Vulkan                                                  
/// Copyright (c) 2020 Dimo Markov <team@langulus.com>                        
/// Part of the Langulus framework, see https://langulus.com                  
///                                                                           
/// SPDX-License-Identifier: GPL-3.0-or-later                                 
///                                                                           
#pragma once
#include "VulkanRenderer.hpp"


///                                                                           
///   Vulkan graphics module                                                  
///                                                                           
/// Manages and produces renderers. By default, the module itself can convey  
/// GPU computations, if your hardware has the required capabilities          
///                                                                           
struct Vulkan final : A::GraphicsModule {
   LANGULUS(ABSTRACT) false;
   LANGULUS_BASES(A::GraphicsModule);
   LANGULUS_VERBS(Verbs::Create);

protected:
   friend struct VulkanRenderer;

   // The vulkan instance                                               
   Own<VkInstance> mInstance;
   // The physical device                                               
   Own<VkPhysicalDevice> mAdapter;
   // The logical device                                                
   Own<VkDevice> mDevice;
   // The computation queue, should work without renderers              
   Own<VkQueue> mComputer;
   // True if hardware has GPU compute                                  
   bool mSupportsComputation {};
   // True if hardware has dedicated transfer queue                     
   bool mSupportsTransfer {};
   // True if hardware has sparse binding support                       
   bool mSupportsSparseBinding {};
   // List of renderer components                                       
   TFactory<VulkanRenderer> mRenderers;

   #if LANGULUS(DEBUG)
      // Set of validation layers                                       
      TokenSet mValidationLayers;
      // Relays debug messages                                          
      VkDebugReportCallbackEXT mLogRelay;
   #endif

public:
   Vulkan(Runtime*, const Neat&);
   ~Vulkan();

   bool Update(Time);
   void Create(Verb&);

   unsigned RateDevice(const VkPhysicalDevice&) const;
   VkPhysicalDevice PickAdapter() const;

   #if LANGULUS(DEBUG)
      const TokenSet& GetValidationLayers() const noexcept;
      void CheckValidationLayerSupport(const TokenSet&) const;
   #endif
};
