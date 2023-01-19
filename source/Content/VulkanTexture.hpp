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
class VulkanTexture : public ContentVRAM {
   LANGULUS(PRODUCER) VulkanRenderer;
   LANGULUS_VERBS(Verbs::Create);

private:
   // Original content texture view                                     
   TextureView mView;
   // Image                                                             
   VulkanImage mImage;
   // Image view                                                        
   Own<VkImageView> mImageView;
   // Image sampler                                                     
   Own<VkSampler> mSampler;

public:
   VulkanTexture(VulkanRenderer*);
   VulkanTexture(VulkanTexture&&) noexcept = default;
   VulkanTexture& operator = (VulkanTexture&&) noexcept = default;
   ~VulkanTexture();

   void Initialize();
   void Uninitialize();

   void Create(Verb&);

   using ContentVRAM::operator ==;
};
