///                                                                           
/// Langulus::Module::Vulkan                                                  
/// Copyright(C) 2020 Dimo Markov <langulusteam@gmail.com>                    
///                                                                           
/// Distributed under GNU General Public License v3+                          
/// See LICENSE file, or https://www.gnu.org/licenses                         
///                                                                           
#include "Vulkan.hpp"


/// Descriptor constructor                                                    
///   @param producer - the light producer                                    
///   @param descriptor - the light descriptor                                
VulkanLight::VulkanLight(VulkanLayer* producer, const Neat& descriptor)
   : Graphics {MetaOf<VulkanLight>()}
   , ProducedFrom {producer, descriptor} {
   VERBOSE_VULKAN("Initializing...");
   Couple(descriptor);
   VERBOSE_VULKAN("Initialized");
}
