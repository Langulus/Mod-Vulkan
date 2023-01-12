///                                                                           
/// Langulus::Module::Vulkan                                                  
/// Copyright(C) 2020 Dimo Markov <langulusteam@gmail.com>                    
///                                                                           
/// Distributed under GNU General Public License v3+                          
/// See LICENSE file, or https://www.gnu.org/licenses                         
///                                                                           
#pragma once 
#include "Content/VulkanShader.hpp"


/// A single uniform - maps a trait to a position inside shader binary        
struct Uniform {
   Offset mPosition;
   Trait mTrait;
};

using BufferUpdates = std::vector<VkWriteDescriptorSet>;

/// Base for DataUBO                                                          
struct UBO {
   CVulkanRenderer* mRenderer = nullptr;
   Size mAllocated = 0;
   pcptr mStride = 0;
   Bytes mRAM;
   VRAMBuffer mBuffer;
   VkDescriptorBufferInfo mDescriptor = {};
   std::vector<Uniform> mUniforms;
   VkShaderStageFlags mStages = 0;

   ~UBO();

   void CalculateSizes();
   void Reallocate(pcptr);
   void Destroy();
   bool IsValid() const noexcept {
      return mStride > 0;
   }
};


///                                                                           
///   DATA UNIFORM BUFFER OBJECT                                              
///                                                                           
///   Wraps either VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER or                       
/// VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC.                                
/// Main difference is, the latter is for rates above RefreshRate::PerPass    
///                                                                           
///   These are used for layout(set = 0) and layout(set = 1), one DataUBO     
/// for each bindings corresponding to each RRate                             
///                                                                           
template<bool DYNAMIC>
struct DataUBO : public UBO {
   // Number of allocated mStride-sized blocks                          
   pcptr mUsedCount = 0;

public:
   DataUBO() = default;
   DataUBO(DataUBO&&) noexcept = default;
   DataUBO& operator = (DataUBO&&) noexcept = default;

   DataUBO(const DataUBO&) = delete;

   NOD() auto GetOffset() const noexcept {
      return static_cast<uint32_t>(mUsedCount * mStride);
   }

   void Create(CVulkanRenderer*);
   void Update(pcptr, const VkDescriptorSet&, BufferUpdates&) const;

   /// Set the value of a trait inside last active block                      
   /// Will set nothing, if trait is not part of the UBO                      
   ///   @tparam TRAIT - the trait to search for                              
   ///   @tparam DATA - the value to set                                      
   ///   @param value - the value to set                                      
   template<RTTI::ReflectedTrait TRAIT, RTTI::ReflectedData DATA>
   bool Set(const DATA& value) {
      static_assert(Dense<DATA>, "DATA must be dense");
      static_assert(pcIsPOD<DATA>, "DATA must be POD");
      static const auto uniform = TRAIT::Reflect();
      for (const auto& it : mUniforms) {
         if (it.mTrait.GetTraitID() != uniform->GetID())
            continue;

         pcCopyMemory(&value, mRAM.GetBytes() + mUsedCount * mStride + it.mPosition, sizeof(DATA));
         return true;
      }

      return false;
   }

   /// Allocate another block inside the dynamic UBO                          
   /// Any subsequent Set(s) will be set in the new block                     
   void Push() requires (DYNAMIC) {
      if (mStride)
         Reallocate(++mUsedCount);
   }
};


///                                                                           
///   SAMPLER UNIFORM BUFFER OBJECT                                           
///                                                                           
/// Wraps either VK_DESCRIPTOR_TYPE_SAMPLER or                                
/// VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER.                                
/// These are updated only at RRate::PerRenderable                            
///                                                                           
/// One such is used for layout(set = 2)                                      
///                                                                           
struct SamplerUBO {
   CVulkanRenderer* mRenderer = nullptr;
   VkDescriptorPool mPool;
   Own<VkDescriptorSet> mSamplersUBOSet;
   TAny<VkDescriptorImageInfo> mSamplers;
   TAny<Uniform> mUniforms;

   SamplerUBO() = default;
   SamplerUBO(const SamplerUBO&) noexcept = default;
   SamplerUBO(SamplerUBO&&) noexcept = default;
   ~SamplerUBO();

   /// Check if two sampler sets are functionally the same                    
   inline bool operator == (const SamplerUBO& rhs) const noexcept {
      return mSamplers == rhs.mSamplers && mUniforms == rhs.mUniforms;
   }

   void Create(CVulkanRenderer*, VkDescriptorPool pool);
   void Update(BufferUpdates&) const;

   /// Set a sampler                                                          
   ///   @param value - the value to set                                      
   ///   @param index - the index of the stride, ignored if buffer is static  
   template<RTTI::ReflectedData DATA>
   void Set(const DATA& value, pcptr index = 0) {
      if (mSamplers.size() <= index) {
         throw Except::Graphics(pcLogFuncError
            << "Bad texture index " << index
            << " (max " << mSamplers.size() << ")");
      }

      if constexpr (Same<DATA, CVulkanTexture>) {
         // Reference content in VRAM                                   
         value->Initialize();
         VkDescriptorImageInfo sampler;
         sampler.sampler = value->GetSampler();
         sampler.imageView = value->GetImageView();
         sampler.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
         mSamplers[index] = sampler;
      }
      else if constexpr (Same<DATA, ATexture>) {
         // Cache content in VRAM                                       
         auto cached = PrepareTexture(value);
         VkDescriptorImageInfo sampler;
         sampler.sampler = cached->GetSampler();
         sampler.imageView = cached->GetImageView();
         sampler.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
         mSamplers[index] = sampler;
      }
      else LANGULUS_ASSERT("Unsupported sampler type");
   }

private:
   CVulkanTexture* PrepareTexture(ATexture*) const;
};
