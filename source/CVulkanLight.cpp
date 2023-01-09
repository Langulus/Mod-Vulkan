///                                                                           
/// Langulus::Module::Vulkan                                                  
/// Copyright(C) 2020 Dimo Markov <langulusteam@gmail.com>                    
///                                                                           
/// Distributed under GNU General Public License v3+                          
/// See LICENSE file, or https://www.gnu.org/licenses                         
///                                                                           
#include "CVulkanRenderer.hpp"

/// Create light                                                              
///   @param producer - the producer of the light                             
CVulkanLight::CVulkanLight(CVulkanLayer* producer)
   : ALight{ MetaData::Of<CVulkanLight>() }
   , TProducedFrom{ producer } {
   ClassValidate();
}

/// Draw the light when using deferred rendering                              
void CVulkanLight::Draw() {
   TODO();
}
