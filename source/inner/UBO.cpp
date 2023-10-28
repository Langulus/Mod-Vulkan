///                                                                           
/// Langulus::Module::Vulkan                                                  
/// Copyright(C) 2020 Dimo Markov <langulusteam@gmail.com>                    
///                                                                           
/// Distributed under GNU General Public License v3+                          
/// See LICENSE file, or https://www.gnu.org/licenses                         
///                                                                           
#include "UBO.hpp"
#include "../Vulkan.hpp"

/// Free uniform buffer                                                       
UBO::~UBO() {
   Destroy();
}

/// Free uniform buffer                                                       
void UBO::Destroy() {
   if (not mBuffer.IsValid())
      return;

   mRenderer->mVRAM.DestroyBuffer(mBuffer);
   mRAM.Reset();
}

/// Calculate aligned range, as well as individual uniform byte offsets       
void UBO::CalculateSizes() {
   // Calculate required UBO buffer sizes for the whole pipeline        
   Offset range {};
   for (auto& it : mUniforms) {
      auto concrete = it.mTrait.GetType()->GetMostConcrete();

      LANGULUS_ASSERT(concrete->mIsAbstract, Graphics,
         "Abstract uniform trait couldn't be concertized");
      LANGULUS_ASSERT(concrete->mIsPOD, Graphics,
         "Uniform trait is not POD");

      it.mTrait = Trait::FromMeta(it.mTrait.GetTrait(), concrete);

      // Info about base alignment in Vulkan Spec                       
      //  15.6.4. Offset and Stride Assignment - Alignment Requirements 
      Size baseAlignment;
      if (  it.mTrait.CastsTo<A::Number>(1)
         or it.mTrait.CastsTo<A::Number>(2)
         or it.mTrait.CastsTo<A::Number>(4)
      ) {
         // 1. A scalar has a base alignment equal to its scalar        
         // alignment. A scalar of size N has a scalar alignment of N   
         // 2. A two-component vector has a base alignment equal to     
         // twice its scalar alignment                                  
         baseAlignment = it.mTrait.GetStride();
      }
      else if (it.mTrait.CastsTo<A::Number>(3)) {
         // A three- or four-component vector has a base alignment      
         // equal to four times its scalar alignment                    
         baseAlignment = 4 * it.mTrait.GetMember(nullptr, 0u).GetStride();
      }
      else {
         // A structure has a base alignment equal to the largest base  
         // alignment of any of its members. Which coincides with the   
         // alignof() operator in C++11 and later (reflected)           
         baseAlignment = it.mTrait.GetType()->mAlignment;
      }

      LANGULUS_ASSERT(baseAlignment != 0, Graphics,
         "Bad uniform alignment");

      it.mPosition = Align(range, baseAlignment);
      range = it.mPosition + it.mTrait.GetStride();
   }

   if (range) {
      mStride = Align(range, mRenderer->GetOuterUBOAlignment());
      mDescriptor.range = mStride;
   }
}

/// Reallocate a dynamic uniform buffer object                                
///   @param elements - the number of buffer elements to allocate             
void UBO::Reallocate(Count elements) {
   if (not IsValid() or mAllocated >= elements) {
      // Once allocated enough size, don't do it again                  
      return;
   }

   if (mBuffer.IsValid()) {
      // No way to resize VRAM in place, so free the previous buffer    
      mRenderer->mVRAM.DestroyBuffer(mBuffer);
   }

   // Create the buffer in VRAM                                         
   mAllocated = elements;
   const auto byteSize = mStride * mAllocated;
   mBuffer = mRenderer->mVRAM.CreateBuffer(
      nullptr, VkDeviceSize {byteSize},
      VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, 
      VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT
   );
   mDescriptor.buffer = mBuffer.GetBuffer();

   // Resize the RAM data, retaining contained data                     
   mRAM.Reserve(byteSize);
}

/// Initialize a dynamic uniform buffer object                                
///   @param renderer - the renderer                                          
template<>
void DataUBO<true>::Create(VulkanRenderer* renderer) {
   mRenderer = renderer;
   CalculateSizes();
   Reallocate(1);
}

/// Initialize a static uniform buffer object                                 
///   @param renderer - the renderer                                          
template<>
void DataUBO<false>::Create(VulkanRenderer* renderer) {
   mRenderer = renderer;
   CalculateSizes();
   Reallocate(1);

   // Set predefined data if available                                  
   for (auto& it : mUniforms) {
      if (not it.mTrait)
         continue;

      ::std::memcpy(
         mRAM.GetRaw() + it.mPosition, 
         it.mTrait.GetRaw(), 
         it.mTrait.GetStride()
      );
   }
}

/// Update a dynamic uniform buffer in VRAM                                   
///   @param binding - binding index                                          
///   @param set - the set to update                                          
///   @param output - [out] where updates are registered                      
template<>
void DataUBO<true>::Update(uint32_t binding, const VkDescriptorSet& set, BufferUpdates& output) const {
   if (not IsValid() or not mUsedCount)
      return;

   output.New();

   auto& write = output.Last();
   write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
   write.dstSet = set;
   write.dstBinding = binding;
   write.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
   write.descriptorCount = 1;
   write.pBufferInfo = &mDescriptor;
   mBuffer.Upload(0, mUsedCount * mStride, mRAM.GetRaw());
}

/// Update a static uniform buffer in VRAM                                    
///   @param binding - binding index                                          
///   @param set - the set to update                                          
///   @param output - [out] where updates are registered                      
template<>
void DataUBO<false>::Update(uint32_t binding, const VkDescriptorSet& set, BufferUpdates& output) const {
   if (not IsValid())
      return;

   output.New();

   auto& write = output.Last();
   write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
   write.dstSet = set;
   write.dstBinding = binding;
   write.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
   write.descriptorCount = 1;
   write.pBufferInfo = &mDescriptor;
   mBuffer.Upload(0, mStride, mRAM.GetRaw());
}

/// Explicit abandon-construction                                             
///   @param other - the sampler UBO to abandon                               
SamplerUBO::SamplerUBO(Abandoned<SamplerUBO>&& other) noexcept
   : mRenderer {other->mRenderer}
   , mPool {other->mPool}
   , mSamplersUBOSet {other->mSamplersUBOSet}
   , mSamplers {Abandon(other->mSamplers)}
   , mUniforms {Abandon(other->mUniforms)} {
   other->mSamplersUBOSet = VkDescriptorSet {};
}

/// Free up a sampler set                                                     
SamplerUBO::~SamplerUBO() {
   if (mSamplersUBOSet) {
      vkFreeDescriptorSets(*mRenderer->mDevice, mPool, 1, &mSamplersUBOSet.Get());
      mSamplersUBOSet.Reset();
   }
}

/// Initialize a sampler uniform buffer object                                
///   @param renderer - the renderer                                          
///   @param pool - the pool used for UBOs                                    
void SamplerUBO::Create(VulkanRenderer* renderer, VkDescriptorPool pool) {
   mRenderer = renderer;
   mPool = pool;
   mSamplers.New(mUniforms.GetCount());

   for (Offset id = 0; id < mUniforms.GetCount(); ++id) {
      auto& it = mUniforms[id];
      if (not it.mTrait)
         continue;

      Set(it.mTrait.As<VulkanTexture*>(), id);
   }
}

/// Update the sampler buffer in VRAM                                         
///   @param output - [out] where updates are registered                      
void SamplerUBO::Update(BufferUpdates& output) const {
   for (Offset i = 0; i < mSamplers.GetCount(); ++i) {
      if (not mSamplers[i].sampler)
         continue;

      output.New();

      auto& write = output.Last();
      write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
      write.dstSet = *mSamplersUBOSet;
      write.dstBinding = static_cast<uint32_t>(i);
      write.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
      write.descriptorCount = 1;
      write.pBufferInfo = nullptr;
      write.pImageInfo = &mSamplers[i];
      write.pTexelBufferView = nullptr;
   }
}

/// Check if two sampler sets are functionally the same                       
bool SamplerUBO::operator == (const SamplerUBO& rhs) const noexcept {
   return mSamplers.Compare(rhs.mSamplers) and mUniforms == rhs.mUniforms;
}

/// Set a sampler                                                             
///   @param value - the value to set                                         
///   @param index - the index of the stride, ignored if buffer is static     
void SamplerUBO::Set(const VulkanTexture* texture, Offset index) {
   LANGULUS_ASSERT(mSamplers.GetCount() > index, Graphics,
      "Bad texture index");

   VkDescriptorImageInfo sampler;
   sampler.sampler = texture->GetSampler();
   sampler.imageView = texture->GetImageView();
   sampler.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
   mSamplers[index] = sampler;
}
