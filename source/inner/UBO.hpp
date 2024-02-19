///                                                                           
/// Langulus::Module::Vulkan                                                  
/// Copyright(C) 2020 Dimo Markov <langulusteam@gmail.com>                    
///                                                                           
/// Distributed under GNU General Public License v3+                          
/// See LICENSE file, or https://www.gnu.org/licenses                         
///                                                                           
#pragma once 
#include "VulkanShader.hpp"


///                                                                           
/// A single uniform - maps a trait to a position inside shader binary        
///                                                                           
struct Uniform {
   Offset mPosition;
   Trait mTrait;

   Uniform(Offset offset, const Trait& data)
      : mPosition {offset}
      , mTrait {data} {}

   bool operator == (const Uniform&) const = default;
};

///                                                                           
///   General purpose uniform buffer object                                   
///                                                                           
struct UBO {
   VulkanRenderer* mRenderer {};
   Size mAllocated {};
   Size mStride {};
   Bytes mRAM;
   VulkanBuffer mBuffer;
   VkDescriptorBufferInfo mDescriptor {};
   TAny<Uniform> mUniforms;
   VkShaderStageFlags mStages {};

   ~UBO();

   void CalculateSizes();
   void Reallocate(Count);
   void Destroy();
   NOD() bool IsValid() const noexcept {
      return mStride > 0;
   }
};

using BufferUpdates = TAny<VkWriteDescriptorSet>;


///                                                                           
///   Uniform buffer object for data                                          
///                                                                           
///   Wraps either VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER or                       
/// VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC.                                
/// Main difference is, the latter is for rates above RefreshRate::PerPass    
///   These are used for layout(set = 0) and layout(set = 1), one DataUBO     
/// for each bindings corresponding to each RefreshRate                       
///                                                                           
template<bool DYNAMIC>
struct DataUBO : public UBO {
   // Number of allocated mStride-sized blocks                          
   Count mUsedCount {};

public:
   DataUBO() = default;
   DataUBO(DataUBO&&) noexcept = default;
   DataUBO& operator = (DataUBO&&) noexcept = default;

   DataUBO(const DataUBO&) = delete;

   NOD() auto GetOffset() const noexcept {
      return static_cast<uint32_t>(mUsedCount * mStride);
   }

   void Create(VulkanRenderer*);
   void Update(uint32_t, const VkDescriptorSet&, BufferUpdates&) const;

   /// Set the value of a trait inside last active block                      
   /// Will set nothing, if trait is not part of the UBO                      
   ///   @tparam TRAIT - the trait to search for                              
   ///   @tparam DATA - the value to set                                      
   ///   @param value - the value to set                                      
   template<CT::Trait TRAIT, CT::Data DATA>
   bool Set(const DATA& value) {
      static_assert(CT::Dense<DATA>, "DATA must be dense");
      static_assert(CT::POD<DATA>, "DATA must be POD");

      for (const auto& it : mUniforms) {
         if (not it.mTrait.template IsTrait<TRAIT>())
            continue;

         const auto offset = mUsedCount * mStride + it.mPosition;
         ::std::memcpy(mRAM.GetRaw() + offset, &value, sizeof(DATA));
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
///   Uniform buffer object for samplers                                      
///                                                                           
///   Wraps either                                                            
/// VK_DESCRIPTOR_TYPE_SAMPLER or VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER   
/// These are updated only at Rate::Renderable, one such is used for          
/// layout(set = 2)                                                           
///                                                                           
struct SamplerUBO {
   VulkanRenderer* mRenderer {};
   VkDescriptorPool mPool;
   Own<VkDescriptorSet> mSamplersUBOSet;
   TAny<VkDescriptorImageInfo> mSamplers;
   TAny<Uniform> mUniforms;

   SamplerUBO() = default;
   SamplerUBO(const SamplerUBO&) noexcept = default;
   SamplerUBO(SamplerUBO&&) noexcept = default;
   SamplerUBO(Abandoned<SamplerUBO>&&) noexcept;
   ~SamplerUBO();

   bool operator == (const SamplerUBO&) const noexcept;

   void Create(VulkanRenderer*, VkDescriptorPool);
   void Update(BufferUpdates&) const;
   void Set(const VulkanTexture*, Offset = 0);
};
