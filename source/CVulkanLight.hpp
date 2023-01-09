///                                                                           
/// Langulus::Module::Vulkan                                                  
/// Copyright(C) 2020 Dimo Markov <langulusteam@gmail.com>                    
///                                                                           
/// Distributed under GNU General Public License v3+                          
/// See LICENSE file, or https://www.gnu.org/licenses                         
///                                                                           
#pragma once
#include "IncludeVulkan.hpp"


///                                                                           
///   VULKAN LIGHT COMPONENT                                                  
///                                                                           
class CVulkanLight : public ALight, public TProducedFrom<CVulkanLayer> {
   REFLECT(CVulkanLight);
   CVulkanLight(CVulkanLayer*);

   void Draw();
};
