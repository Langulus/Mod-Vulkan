///                                                                           
/// Langulus::Module::Vulkan                                                  
/// Copyright(C) 2020 Dimo Markov <langulusteam@gmail.com>                    
///                                                                           
/// Distributed under GNU General Public License v3+                          
/// See LICENSE file, or https://www.gnu.org/licenses                         
///                                                                           
#include "VulkanRenderer.hpp"


/// Create light                                                              
///   @param producer - the producer of the light                             
VulkanLight::VulkanLight(VulkanLayer* producer)
   : ALight {MetaData::Of<VulkanLight>()}
   , TProducedFrom {producer} { }
