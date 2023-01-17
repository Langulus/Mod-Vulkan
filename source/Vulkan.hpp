///                                                                           
/// Langulus::Module::Vulkan                                                  
/// Copyright(C) 2020 Dimo Markov <langulusteam@gmail.com>                    
///                                                                           
/// Distributed under GNU General Public License v3+                          
/// See LICENSE file, or https://www.gnu.org/licenses                         
///                                                                           
#pragma once
#include "VulkanRenderer.hpp"

using TokenSet = ::std::vector<const char*>;

///                                                                           
///   Vulkan graphics module                                                  
///                                                                           
/// Manages and produces renderers. By default, the module itself can convey  
/// GPU computations, if hardware allows it                                   
///                                                                           
class Vulkan : public Module {
   LANGULUS(ABSTRACT) false;
   LANGULUS_BASES(Module);
   LANGULUS_VERBS(Verbs::Create);

private:
   // The vulkan instance                                               
   Own<VkInstance> mInstance;
   // The physical device                                               
   Own<VkPhysicalDevice> mAdapter;
   // The logical device                                                
   Own<VkDevice> mDevice;
   // The computation queue, should work without renderers              
   Own<VkQueue> mComputer;
   // True if hardware has GPU compute                                  
   bool mSupportsComputation = false;
   // True if hardware has dedicated transfer queue                     
   bool mSupportsTransfer = false;
   // True if hardware has sparse binding support                       
   bool mSupportsSparseBinding = false;
   // List of renderer components                                       
   TFactory<VulkanRenderer> mRenderers;


   #if LANGULUS_DEBUG()
      TokenSet mValidationLayers;
      VkDebugReportCallbackEXT mLogRelay;   // Relays debug messages    
   #endif

public:
   Vulkan(Runtime*, const Any&);
   ~Vulkan();

   void Update(Time) final;
   void Create(Verb&);

   // Helpers                                                           
   TokenSet GetRequiredExtensions() const;
   unsigned RateDevice(const VkPhysicalDevice&) const;
   VkPhysicalDevice PickAdapter() const;

   #if LANGULUS_DEBUG()
      auto& GetValidationLayers() const noexcept;
      void CheckValidationLayerSupport(const TokenSet&) const;
   #endif
};
