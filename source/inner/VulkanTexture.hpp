///                                                                           
/// Langulus::Module::Vulkan                                                  
/// Copyright(C) 2020 Dimo Markov <langulusteam@gmail.com>                    
///                                                                           
/// Distributed under GNU General Public License v3+                          
/// See LICENSE file, or https://www.gnu.org/licenses                         
///                                                                           
#pragma once
#include "VulkanBuffer.hpp"


///                                                                           
///   VULKAN VRAM TEXTURE                                                     
///                                                                           
/// Handles hardware pixel/voxel buffers                                      
///                                                                           
struct VulkanTexture : A::Graphics, ProducedFrom<VulkanRenderer> {
   LANGULUS(ABSTRACT) false;
   LANGULUS_BASES(A::Graphics);

private:
   // Original content texture view                                     
   TextureView mView;
   // Image                                                             
   VulkanImage mImage;
   // Image view                                                        
   Own<VkImageView> mImageView;
   // Image sampler                                                     
   Own<VkSampler> mSampler;

   void InitializeFromPixels(const A::Texture&);

public:
   VulkanTexture(VulkanRenderer*, const Any&);
   ~VulkanTexture();

   NOD() VkImageView GetImageView() const noexcept;
   NOD() VkSampler GetSampler() const noexcept;
};
