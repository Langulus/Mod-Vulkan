///                                                                           
/// Langulus::Module::Vulkan                                                  
/// Copyright (c) 2020 Dimo Markov <team@langulus.com>                        
/// Part of the Langulus framework, see https://langulus.com                  
///                                                                           
/// Distributed under GNU General Public License v3+                          
/// See LICENSE file, or https://www.gnu.org/licenses                         
///                                                                           
#include "Vulkan.hpp"


/// Descriptor constructor                                                    
///   @param producer - the light producer                                    
///   @param descriptor - the light descriptor                                
VulkanLight::VulkanLight(VulkanLayer* producer, const Neat& descriptor)
   : Resolvable {MetaOf<VulkanLight>()}
   , ProducedFrom {producer, descriptor} {
   VERBOSE_VULKAN("Initializing...");
   Couple(descriptor);
   VERBOSE_VULKAN("Initialized");
}
