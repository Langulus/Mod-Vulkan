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
   if (!mBuffer.IsValid())
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
         || it.mTrait.CastsTo<A::Number>(2)
         || it.mTrait.CastsTo<A::Number>(4)
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
   if (!IsValid() || mAllocated >= elements) {
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
      nullptr, VkDeviceSize(byteSize),
      VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, 
      VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT
   );
   mDescriptor.buffer = mBuffer.mBuffer;

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
      if (it.mTrait.IsEmpty())
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
   if (!IsValid() || !mUsedCount)
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
   if (!IsValid())
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

/// Initialize a sampler uniform buffer object                                
///   @param renderer - the renderer                                          
///   @param pool - the pool used for UBOs                                    
void SamplerUBO::Create(VulkanRenderer* renderer, VkDescriptorPool pool) {
   mRenderer = renderer;
   mPool = pool;
   mSamplers.New(mUniforms.GetCount());

   for (Offset id = 0; id < mUniforms.GetCount(); ++id) {
      auto& it = mUniforms[id];
      if (it.mTrait.IsEmpty())
         continue;

      Set(it.mTrait.As<VulkanTexture*>(), id);
   }
}

/// Update the sampler buffer in VRAM                                         
///   @param output - [out] where updates are registered                      
void SamplerUBO::Update(BufferUpdates& output) const {
   for (Offset i = 0; i < mSamplers.GetCount(); ++i) {
      if (!mSamplers[i].sampler)
         continue;

      output.New();

      auto& write = output.Last();
      write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
      write.dstSet = mSamplersUBOSet;
      write.dstBinding = static_cast<uint32_t>(i);
      write.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
      write.descriptorCount = 1;
      write.pBufferInfo = nullptr;
      write.pImageInfo = &mSamplers[i];
      write.pTexelBufferView = nullptr;
   }
}

/// Free up a sampler set                                                     
SamplerUBO::~SamplerUBO() {
   if (mSamplersUBOSet) {
      vkFreeDescriptorSets(mRenderer->mDevice, mPool, 1, &mSamplersUBOSet.Get());
      mSamplersUBOSet.Reset();
   }
}

/// Cache and initialize the texture                                          
///   @param value - texture to cache and initialize                          
///   @return the VRAM texture                                                
VulkanTexture* SamplerUBO::PrepareTexture(Unit* texture) const {
   auto cached = mRenderer->Cache(texture);
   cached->Initialize();
   return cached;
}