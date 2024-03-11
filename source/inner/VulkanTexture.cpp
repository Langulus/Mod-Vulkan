///                                                                           
/// Langulus::Module::Vulkan                                                  
/// Copyright(C) 2020 Dimo Markov <langulusteam@gmail.com>                    
///                                                                           
/// Distributed under GNU General Public License v3+                          
/// See LICENSE file, or https://www.gnu.org/licenses                         
///                                                                           
#include "../Vulkan.hpp"

#define VERBOSE_VKTEXTURE(...) //Logger::Verbose(Self(), __VA_ARGS__)


/// VRAM texture constructor                                                  
///   @param producer - the texture producer                                  
///   @param descriptor - the texture descriptor                              
VulkanTexture::VulkanTexture(VulkanRenderer* producer, const Neat& descriptor)
   : A::Graphics {MetaOf<VulkanTexture>()}
   , ProducedFrom {producer, descriptor} {
   descriptor.ForEachDeep([&](const A::Image& content) {
      Upload(content);
   });
}

/// VRAM texture destructor                                                   
VulkanTexture::~VulkanTexture() {
   if (mSampler)
      vkDestroySampler(mProducer->mDevice, mSampler, nullptr);
   if (mImageView)
      vkDestroyImageView(mProducer->mDevice, mImageView, nullptr);
   if (mImage.GetImage())
      mProducer->mVRAM.DestroyImage(mImage);
}

/// Initialize from the provided content                                      
///   @param content - the abstract texture content interface                 
void VulkanTexture::Upload(const A::Image& content) {
   // Check if any data was found                                       
   //const auto startTime = SteadyClock::Now();
   const auto pixels = content.GetDataList<Traits::Color>();
   LANGULUS_ASSERT(pixels && *pixels, Graphics,
      "Can't generate texture - no color data found");

   // Copy base view and create image                                   
   // Beware, the VRAM image may have a different internal format       
   auto& vram = mProducer->mVRAM;
   mView = content.GetView();
   mImage = vram.CreateImage(
      mView, VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT
   );

   // Copy raw data to VRAM stager                                      
   // The stager is created to contain the type in the VulkanImage,     
   // which may or may have not changed                                 
   const auto totalVramBytes = mImage.GetView().GetBytesize();
   auto stager = vram.CreateBuffer(
      mImage.GetView().mFormat, totalVramBytes,
      VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
      VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
   );

   if (mImage.GetView().mFormat == content.GetView().mFormat) {
      // Formats match, directly upload the dense memory                
      stager.Upload(0, totalVramBytes, pixels->GetRaw());
   }
   else {
      // Add an alpha channel                                           
      //TODO this may not be the only reason this happens!              
      Logger::Warning(
         "Performance warning: texture is being converted "
         "to a different internal memory format"
      );

      auto rawTo = stager.Lock(0, totalVramBytes);
      auto rawFrom = pixels->GetRaw();
      const auto strideTo = mImage.GetView().GetPixelBytesize();
      const auto strideFrom = content.GetView().GetPixelBytesize();
      for (uint32_t pixel = 0; pixel < mView.GetPixelCount(); ++pixel) {
         ::std::memcpy(rawTo, rawFrom, strideFrom);
         ::std::memset(rawTo + strideFrom, 255, strideTo - strideFrom);
         rawFrom += strideFrom;
         rawTo += strideTo;
      }
      stager.Unlock();
   }

   VkCommandBufferBeginInfo beginInfo {};
   auto& cmdbuffer = vram.mTransferBuffer;
   beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO; 
   beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
   /*VkBufferCopy copyRegion {};
   copyRegion.srcOffset = 0;
   copyRegion.dstOffset = 0;
   copyRegion.size = totalVramBytes;*/
   vkBeginCommandBuffer(cmdbuffer, &beginInfo);

   // Transition to VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL layout         
   VkImageMemoryBarrier barrier {};
   barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
   barrier.oldLayout = VK_IMAGE_LAYOUT_PREINITIALIZED;
   barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
   barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
   barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
   barrier.image = mImage.GetImage();
   barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
   barrier.subresourceRange.baseMipLevel = 0;
   barrier.subresourceRange.levelCount = 1;
   barrier.subresourceRange.baseArrayLayer = 0;
   barrier.subresourceRange.layerCount = 1;
   barrier.srcAccessMask = VK_ACCESS_HOST_WRITE_BIT;
   barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

   vkCmdPipelineBarrier(
      cmdbuffer,
      VK_PIPELINE_STAGE_HOST_BIT | VK_PIPELINE_STAGE_TRANSFER_BIT,
      VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
      0,
      0, nullptr,
      0, nullptr,
      1, &barrier
   );

   // Copy bytes                                                        
   VkBufferImageCopy region {};
   region.bufferOffset = 0;
   region.bufferRowLength = 0;
   region.bufferImageHeight = 0;
   region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
   region.imageSubresource.mipLevel = 0;
   region.imageSubresource.baseArrayLayer = 0;
   region.imageSubresource.layerCount = 1;
   region.imageOffset = {0, 0, 0};
   region.imageExtent = {mView.mWidth, mView.mHeight, mView.mDepth};

   vkCmdCopyBufferToImage(
      cmdbuffer,
      stager.GetBuffer(),
      mImage.GetImage(),
      VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
      1, &region
   );

   // Transition to VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL            
   barrier = {};
   barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
   barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
   barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
   barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
   barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
   barrier.image = mImage.GetImage();
   barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
   barrier.subresourceRange.baseMipLevel = 0;
   barrier.subresourceRange.levelCount = 1;
   barrier.subresourceRange.baseArrayLayer = 0;
   barrier.subresourceRange.layerCount = 1;
   barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
   barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

   vkCmdPipelineBarrier(
      cmdbuffer,
      VK_PIPELINE_STAGE_TRANSFER_BIT,
      VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
      0, 0, nullptr, 0, nullptr, 1, &barrier
   );

   // Submit                                                            
   vkEndCommandBuffer(cmdbuffer);
   VkSubmitInfo submitInfo {};
   submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
   submitInfo.commandBufferCount = 1;
   submitInfo.pCommandBuffers = &cmdbuffer;
   vkQueueSubmit(vram.mTransferer, 1, &submitInfo, VK_NULL_HANDLE);

   // Wait for completion                                               
   vkQueueWaitIdle(vram.mTransferer);

   // Don't forget to clean up the staging buffer                       
   vram.DestroyBuffer(stager);

   // Create image view                                                 
   mImageView = vram.CreateImageView(mImage);

   // Create samplers                                                   
   VkSamplerCreateInfo samplerInfo {};
   samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
   samplerInfo.magFilter = VK_FILTER_LINEAR;
   samplerInfo.minFilter = VK_FILTER_LINEAR;
   samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
   samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
   samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
   samplerInfo.anisotropyEnable = VK_FALSE;
   samplerInfo.maxAnisotropy = 16;
   samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
   samplerInfo.unnormalizedCoordinates = VK_FALSE;
   samplerInfo.compareEnable = VK_FALSE;
   samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;
   samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
   samplerInfo.mipLodBias = 0.0f;
   samplerInfo.minLod = 0.0f;
   samplerInfo.maxLod = 0.0f;

   if (vkCreateSampler(mProducer->mDevice, &samplerInfo, nullptr, &mSampler.Get()))
      LANGULUS_THROW(Graphics, "Can't create vulkan sampler");

   VERBOSE_VKTEXTURE(Logger::Green, "Data uploaded in VRAM for ",
      SteadyClock::now() - startTime);
}

/// Get the VkImageView                                                       
///   @return the image view                                                  
VkImageView VulkanTexture::GetImageView() const noexcept {
   return mImageView;
}

/// Get the VkSampler                                                         
///   @return the sampler                                                     
VkSampler VulkanTexture::GetSampler() const noexcept {
   return mSampler;
}
