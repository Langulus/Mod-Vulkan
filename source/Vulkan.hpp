///                                                                           
/// Langulus::Module::Vulkan                                                  
/// Copyright(C) 2020 Dimo Markov <langulusteam@gmail.com>                    
///                                                                           
/// Distributed under GNU General Public License v3+                          
/// See LICENSE file, or https://www.gnu.org/licenses                         
///                                                                           
#pragma once
#include "VulkanRenderer.hpp"


///                                                                           
///   Vulkan graphics module                                                  
///                                                                           
/// Manages and produces renderers. By default, the module itself can convey  
/// GPU computations, if your hardware has the required capabilities          
///                                                                           
struct Vulkan final : A::Graphics {
   LANGULUS(ABSTRACT) false;
   LANGULUS_BASES(A::Graphics);
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
   Vulkan(Runtime*, const Any&);
   ~Vulkan();

   void Update(Time);
   void Create(Verb&);

   unsigned RateDevice(const VkPhysicalDevice&) const;
   VkPhysicalDevice PickAdapter() const;

   #if LANGULUS(DEBUG)
      auto& GetValidationLayers() const noexcept;
      void CheckValidationLayerSupport(const TokenSet&) const;
   #endif
};
