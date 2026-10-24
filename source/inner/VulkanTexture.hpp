///                                                                           
/// Langulus::Module::Vulkan                                                  
/// Copyright (c) 2020 Dimo Markov <team@langulus.com>                        
/// Part of the Langulus framework, see https://langulus.com                  
///                                                                           
/// SPDX-License-Identifier: GPL-3.0-or-later                                 
///                                                                           
#pragma once
#include "VulkanBuffer.hpp"


///                                                                           
///   VULKAN VRAM TEXTURE                                                     
///                                                                           
/// Handles hardware pixel/voxel buffers                                      
///                                                                           
struct VulkanTexture : Things::Graphics, ProducedFrom<VulkanRenderer> {
   using CTTI_Abstract = No;
   LANGULUS_BASES(Things::Graphics);

private:
   // Original content texture view                                     
   ImageView mView;
   // Image                                                             
   VulkanImage mImage;
   // Image view                                                        
   Own<VkImageView> mImageView;
   // Image sampler                                                     
   Own<VkSampler> mSampler;

   void Upload(Things::Image const&);

public:
   VulkanTexture(VulkanRenderer*, Describe);
   ~VulkanTexture();

   NOD() VkImageView GetImageView() const noexcept;
   NOD() VkSampler GetSampler() const noexcept;
};
