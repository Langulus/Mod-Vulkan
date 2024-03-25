///                                                                           
/// Langulus::Module::Vulkan                                                  
/// Copyright (c) 2020 Dimo Markov <team@langulus.com>                        
/// Part of the Langulus framework, see https://langulus.com                  
///                                                                           
/// Distributed under GNU General Public License v3+                          
/// See LICENSE file, or https://www.gnu.org/licenses                         
///                                                                           
#pragma once
#include "Common.hpp"


///                                                                           
///   Light source unit                                                       
///                                                                           
struct VulkanLight : A::Graphics, ProducedFrom<VulkanLayer> {
   LANGULUS(ABSTRACT) false;
   LANGULUS_BASES(A::Graphics);

   VulkanLight(VulkanLayer*, const Neat&);
};
