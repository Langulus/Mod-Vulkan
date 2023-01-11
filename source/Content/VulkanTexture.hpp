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
class CVulkanTexture : public IContentVRAM {
   LANGULUS(PRODUCER) CVulkanRenderer;
   LANGULUS_VERBS(Verbs::Create);

private:
   // Original content texture view                                     
   PixelView mView;
   // Image                                                             
   VRAMImage mImage;
   // Image view                                                        
   Own<VkImageView> mImageView;
   // Image sampler                                                     
   Own<VkSampler> mSampler;

public:
   CVulkanTexture(CVulkanRenderer*);
   CVulkanTexture(CVulkanTexture&&) noexcept = default;
   CVulkanTexture& operator = (CVulkanTexture&&) noexcept = default;
   ~CVulkanTexture();

   void Initialize();
   void Uninitialize();

   void Create(Verb&);

   using IContentVRAM::operator ==;
};
