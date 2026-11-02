///                                                                           
/// Langulus::Module::Vulkan                                                  
/// Copyright (c) 2020 Dimo Markov <team@langulus.com>                        
/// Part of the Langulus framework, see https://langulus.com                  
///                                                                           
/// SPDX-License-Identifier: GPL-3.0-or-later                                 
///                                                                           
#pragma once
#include "Export.hpp"


///                                                                           
///   Light source unit                                                       
struct VulkanLight : Things::Graphics, ProducedFrom<VulkanLayer> {
   using CTTI_Abstract  = No;
   using CTTI_Bases     = Things::Graphics;

   VulkanLight(VulkanLayer*, Neat const&);
};
