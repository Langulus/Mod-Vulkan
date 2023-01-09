///                                                                           
/// Langulus::Module::Vulkan                                                  
/// Copyright(C) 2020 Dimo Markov <langulusteam@gmail.com>                    
///                                                                           
/// Distributed under GNU General Public License v3+                          
/// See LICENSE file, or https://www.gnu.org/licenses                         
///                                                                           
#pragma once
#include "UBO.hpp"

///                                                                           
///   VULKAN PIPELINE                                                         
///                                                                           
class CVulkanPipeline 
   : public AUnitGraphics
   , public TProducedFrom<CVulkanRenderer> {
   using Bindings = std::vector<VkDescriptorSetLayoutBinding>;

   void CreateDescriptorLayoutAndSet(const Bindings&, UBOLayout*, VkDescriptorSet*);
   void CreateUniformBuffers();
   void CreateNewSamplerSet();
   void CreateNewGeometrySet();

   // Shaders                                                           
   TAny<CVulkanShader*> mStages;
   // The graphics pipeline                                             
   Own<VkPipeline> mPipeline;
   // The rendering pipeline layout                                     
   Own<VkPipelineLayout> mPipeLayout;

   // Pool for UBO sets                                                 
   Own<VkDescriptorPool> mUBOPool;

   // UBO set layouts                                                   
   Own<UBOLayout> mStaticUBOLayout;
   Own<UBOLayout> mDynamicUBOLayout;
   Own<UBOLayout> mSamplersUBOLayout;

   // Sets of uniform buffer objects                                    
   Own<VkDescriptorSet> mStaticUBOSet;
   Own<VkDescriptorSet> mDynamicUBOSet;

   // Uniform buffer objects for each RefreshRate                       
   DataUBO<false> mStaticUBO[RRate::StaticUniformCount];
   DataUBO<true> mDynamicUBO[RRate::DynamicUniformCount];
   std::vector<DataUBO<true>*> mRelevantDynamicDescriptors;

   // Sets and samplers for textures                                    
   TAny<SamplerUBO> mSamplerUBO;

   // Vertex input descriptor                                           
   VertexInput mInput {};
   // Vertex input assembly descriptor                                  
   VertexAssembly mAssembly {};
   // Topology                                                          
   Topology mPrimitive = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
   // Blending mode (participates in hash)                              
   BlendMode mBlendMode = BlendMode::Normal;
   // Toggle depth testing and writing                                  
   bool mDepth = true;

   // Subscribers                                                       
   std::vector<PipeSubscriber> mSubscribers;
   std::vector<const CVulkanGeometry*> mGeometries;

   Hash mHash;
   bool mGenerated = false;
   Ref<const AMaterial> mOriginalContent;

public:
   REFLECT(CVulkanPipeline);
   CVulkanPipeline(CVulkanRenderer*);
   CVulkanPipeline(CVulkanPipeline&&) noexcept = default;
   CVulkanPipeline& operator = (CVulkanPipeline&&) noexcept = default;
   ~CVulkanPipeline();

   NOD() const Hash& GetHash() const noexcept;
   bool operator == (const ME&) const noexcept;
   bool operator != (const ME&) const noexcept;

   PC_VERB(Create);

   bool PrepareFromConstruct(const Construct&);
   bool PrepareFromMaterial(const AMaterial*);
   bool PrepareFromCode(const GLSL&);

   void Initialize();
   void Uninitialize();
   NOD() pcptr RenderLevel(const pcptr&) const;
   void RenderSubscriber(const PipeSubscriber&) const;
   void ResetUniforms();

   /// Set any kind of uniform                                                
   ///   @tparam RATE - the rate of the trait to set                          
   ///   @tparam TRAIT - the trait to set                                     
   ///   @tparam DATA - the data to set                                       
   ///   @param value - the value to use                                      
   ///   @param index - the index of the uniform of this kind                 
   ///                  used as ID only when setting a texture                
   template<RRate::Enum RATE, RTTI::ReflectedTrait TRAIT, RTTI::ReflectedData DATA>
   void SetUniform(const DATA& value, pcptr index = 0) {
      if constexpr (Same<DATA, CVulkanTexture>) {
         static_assert(RATE == RRate::PerRenderable,
            "Setting a texture requires RRate::PerRenderable");
         // Set the sampler with the given index                        
         mSamplerUBO[mSubscribers.back().samplerSet].template Set<DATA>(value, index);
      }
      else if constexpr (Same<DATA, CVulkanGeometry>) {
         static_assert(RATE == RRate::PerRenderable,
            "Setting a geometry stream requires RRate::PerRenderable");
         // Set the geometry stream, make sure VRAM is initialized      
         value->Initialize();
         mGeometries[mSubscribers.back().geometrySet] = value;
      }
      else if constexpr (RRate(RATE).IsStaticUniform()) {
         // Set a static uniform                                        
         (index);
         constexpr auto rate = RRate(RATE).GetStaticUniformIndex();
         mStaticUBO[rate].template Set<TRAIT, DATA>(value);
      }
      else if constexpr (RRate(RATE).IsDynamicUniform()) {
         // Set a dynamic uniform                                       
         (index);
         constexpr auto rate = RRate(RATE).GetDynamicUniformIndex();
         mDynamicUBO[rate].template Set<TRAIT, DATA>(value);
      }
      else LANGULUS_ASSERT("Unsupported uniform rate");
   }

   /// Push the current samplers and dynamic uniforms, advancing indices      
   ///   @tparam RATE - the rate to push                                      
   ///   @tparam SUBSCRIBE - whether or not to subscribe for batched draw     
   ///   @return the byte offset of the pushed uniform block of dynamic RATE  
   ///   @return the above, but paired with the sampler set index in use, and 
   ///           the geometry set index in use, if   RATE is PerRenderable    
   template<RRate::Enum RATE, bool SUBSCRIBE = true>
   NOD() auto PushUniforms() {
      if constexpr (RRate(RATE).IsStaticUniform()) {
         // Pushing static uniforms does nothing                        
         SAFETY(pcLogSelfError << "Trying to push a static uniform block "
            "- although not fatal, there's no point in doing that");
      }
      else if constexpr (RRate(RATE).IsDynamicUniform()) {
         // Push a dynamic rate                                         
         constexpr auto rate = RRate(RATE).GetDynamicUniformIndex();

         mDynamicUBO[rate].Push();

         if constexpr (RATE == RRate::PerRenderable) {
            // When pushing PerRenderable state, create new sampler set 
            // and a new geometry set for next SetUniform calls         
            CreateNewSamplerSet();
            CreateNewGeometrySet();
         }

         if constexpr (RATE == RRate::PerInstance) {
            // Push a new subscriber only on new instance               
            PipeSubscriber newSubscriber = mSubscribers.back();
            pcptr i = 0;
            for (auto ubo : mRelevantDynamicDescriptors) {
               newSubscriber.offsets[i] = ubo->GetOffset();
               ++i;
            }

            if constexpr (SUBSCRIBE)
               mSubscribers.push_back(newSubscriber);
            else {
               std::swap(mSubscribers.back(), newSubscriber);
               return newSubscriber;
            }
         }
      }
      else LANGULUS_ASSERT("Unsupported uniform rate to push");
   }

   ///                                                                        
   ///   @return                                                              
   template<RRate::Enum RATE>
   NOD() auto GetRelevantDynamicUBOIndexOfRate() const noexcept {
      constexpr auto r = RRate(RRate::PerLevel).GetDynamicUniformIndex();
      pcptr rtoi = 0;
      for (pcptr s = 0; s < r; ++s) {
         // Unused dynamic UBOs do not participate in offsets!          
         // Find what's the relevant index for 'r'                      
         if (mDynamicUBO[s].IsValid())
            ++rtoi;
      }
      return rtoi;
   }

   void UpdateUniformBuffers() const;
};
