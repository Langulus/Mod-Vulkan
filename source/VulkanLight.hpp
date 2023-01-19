///                                                                           
/// Langulus::Module::Vulkan                                                  
/// Copyright(C) 2020 Dimo Markov <langulusteam@gmail.com>                    
///                                                                           
/// Distributed under GNU General Public License v3+                          
/// See LICENSE file, or https://www.gnu.org/licenses                         
///                                                                           
#pragma once
#include "Common.hpp"


///                                                                           
///   Light source unit                                                       
///                                                                           
class VulkanLight : public A::GraphicsUnit {
   LANGULUS(ABSTRACT) false;
   LANGULUS(PRODUCER) VulkanLayer;
   LANGULUS_BASES(A::GraphicsUnit);
private:
   VulkanLight(VulkanLayer*);
};
