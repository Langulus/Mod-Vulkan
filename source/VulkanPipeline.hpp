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
/// Pipeline subscriber                                                       
///                                                                           
struct PipeSubscriber {
   uint32_t offsets[Rate::DynamicUniformCount] {};
   uint32_t samplerSet {};
   uint32_t geometrySet {};
};


///                                                                           
///   Vulkan pipeline                                                         
///                                                                           
class VulkanPipeline : public A::GraphicsUnit {
   LANGULUS(ABSTRACT) false;
   LANGULUS(PRODUCER) VulkanRenderer;
   LANGULUS_BASES(A::GraphicsUnit);
   LANGULUS_VERBS(Verbs::Create);
private:
   using Bindings = TAny<VkDescriptorSetLayoutBinding>;

   void CreateDescriptorLayoutAndSet(const Bindings&, UBOLayout*, VkDescriptorSet*);
   void CreateUniformBuffers();
   void CreateNewSamplerSet();
   void CreateNewGeometrySet();

   // Shaders                                                           
   TAny<VulkanShader*> mStages;
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
   DataUBO<false> mStaticUBO[Rate::StaticUniformCount];
   DataUBO<true> mDynamicUBO[Rate::DynamicUniformCount];
   TAny<DataUBO<true>*> mRelevantDynamicDescriptors;

   // Sets and samplers for textures                                    
   TAny<SamplerUBO> mSamplerUBO;

   // Vertex input descriptor                                           
   VertexInput mInput {};
   // Vertex input assembly descriptor                                  
   VertexAssembly mAssembly {};
   // Topology                                                          
   Topology mPrimitive {VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST};
   // Blending mode (participates in hash)                              
   BlendMode mBlendMode {BlendMode::Alpha};
   // Toggle depth testing and writing                                  
   bool mDepth {true};

   // Subscribers                                                       
   TAny<PipeSubscriber> mSubscribers;
   TAny<const VulkanGeometry*> mGeometries;

   Hash mHash;
   bool mGenerated {false};
   Ptr<const Unit> mOriginalContent;

public:
   VulkanPipeline(VulkanRenderer*);
   VulkanPipeline(VulkanPipeline&&) noexcept = default;
   ~VulkanPipeline();

   VulkanPipeline& operator = (VulkanPipeline&&) noexcept = default;

   NOD() const Hash& GetHash() const noexcept;

   bool operator == (const VulkanPipeline&) const noexcept;

   void Create(Verb&);

   bool PrepareFromConstruct(const Construct&);
   bool PrepareFromMaterial(const Unit*);
   bool PrepareFromCode(const GLSL&);

   void Initialize();
   void Uninitialize();
   NOD() Count RenderLevel(const Offset&) const;
   void RenderSubscriber(const PipeSubscriber&) const;
   void ResetUniforms();

   /// Set any kind of uniform                                                
   ///   @tparam RATE - the rate of the trait to set                          
   ///   @tparam TRAIT - the trait to set                                     
   ///   @tparam DATA - the data to set                                       
   ///   @param value - the value to use                                      
   ///   @param index - the index of the uniform of this kind                 
   ///                  used as ID only when setting a texture                
   template<Rate::Enum RATE, CT::Trait TRAIT, CT::Data DATA>
   void SetUniform(const DATA& value, Offset index = 0) {
      constexpr Rate RATED = RATE;

      if constexpr (Same<DATA, VulkanTexture>) {
         static_assert(RATE == Rate::Renderable,
            "Setting a texture requires Rate::PerRenderable");
         // Set the sampler with the given index                        
         mSamplerUBO[mSubscribers.Last().samplerSet]
            .template Set<DATA>(value, index);
      }
      else if constexpr (Same<DATA, VulkanGeometry>) {
         static_assert(RATE == Rate::Renderable,
            "Setting a geometry stream requires Rate::PerRenderable");
         // Set the geometry stream, make sure VRAM is initialized      
         value->Initialize();
         mGeometries[mSubscribers.Last().geometrySet] = value;
      }
      else if constexpr (RATED.IsStaticUniform()) {
         // Set a static uniform                                        
         (index);
         constexpr auto rate = RATED.GetStaticUniformIndex();
         mStaticUBO[rate].template Set<TRAIT, DATA>(value);
      }
      else if constexpr (RATED.IsDynamicUniform()) {
         // Set a dynamic uniform                                       
         (index);
         constexpr auto rate = RATED.GetDynamicUniformIndex();
         mDynamicUBO[rate].template Set<TRAIT, DATA>(value);
      }
      else LANGULUS_ERROR("Unsupported uniform rate");
   }

   /// Push the current samplers and dynamic uniforms, advancing indices      
   ///   @tparam RATE - the rate to push                                      
   ///   @tparam SUBSCRIBE - whether or not to subscribe for batched draw     
   ///   @return the subscriber, if SUBSCRIBE is true, and RATE is dynamic    
   template<Rate::Enum RATE, bool SUBSCRIBE = true>
   NOD() auto PushUniforms() {
      constexpr Rate RATED = RATE;

      if constexpr (RATED.IsStaticUniform()) {
         // Pushing static uniforms does nothing                        
         SAFETY(Logger::Warning(
            "Trying to push a static uniform block"
            " - although not fatal, it's suboptimal doing that"
         ));
      }
      else if constexpr (RATED.IsDynamicUniform()) {
         // Push a dynamic rate                                         
         constexpr auto rate = RATED.GetDynamicUniformIndex();

         mDynamicUBO[rate].Push();

         if constexpr (RATE == Rate::Renderable) {
            // When pushing PerRenderable state, create new sampler set 
            // and a new geometry set for next SetUniform calls         
            CreateNewSamplerSet();
            CreateNewGeometrySet();
         }

         if constexpr (RATE == Rate::Instance) {
            // Push a new subscriber only on new instance               
            PipeSubscriber newSubscriber = mSubscribers.Last();
            Offset i {};
            for (auto ubo : mRelevantDynamicDescriptors)
               newSubscriber.offsets[i++] = ubo->GetOffset();

            if constexpr (SUBSCRIBE)
               mSubscribers << newSubscriber;
            else {
               ::std::swap(mSubscribers.Last(), newSubscriber);
               return newSubscriber;
            }
         }
      }
      else LANGULUS_ERROR("Unsupported uniform rate to push");
   }

   /// Convert a rate to the corresponding UBO index                          
   ///   @param RATE - the rate to convert                                    
   ///   @return the relevant UBO index                                       
   template<Rate::Enum RATE>
   NOD() auto GetRelevantDynamicUBOIndexOfRate() const noexcept {
      constexpr Rate RATED {Rate::Level};
      constexpr auto r = RATED.GetDynamicUniformIndex();
      Offset rtoi {};
      for (Offset s = 0; s < r; ++s) {
         // Unused dynamic UBOs do not participate in offsets!          
         // Find what's the relevant index for 'r'                      
         if (mDynamicUBO[s].IsValid())
            ++rtoi;
      }
      return rtoi;
   }

   void UpdateUniformBuffers() const;
};
