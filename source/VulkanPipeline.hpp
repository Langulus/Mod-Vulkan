///                                                                           
/// Langulus::Module::Vulkan                                                  
/// Copyright(C) 2020 Dimo Markov <langulusteam@gmail.com>                    
///                                                                           
/// Distributed under GNU General Public License v3+                          
/// See LICENSE file, or https://www.gnu.org/licenses                         
///                                                                           
#pragma once
#include "inner/UBO.hpp"
#include <Math/Blend.hpp>


///                                                                           
///   Pipeline subscriber                                                     
///                                                                           
struct PipeSubscriber {
   uint32_t offsets[Rate::DynamicUniformCount] {};
   uint32_t samplerSet {};
   uint32_t geometrySet {};
};


///                                                                           
///   Vulkan pipeline                                                         
///                                                                           
struct VulkanPipeline : A::Graphics, ProducedFrom<VulkanRenderer> {
   LANGULUS(ABSTRACT) false;
   LANGULUS_BASES(A::Graphics);

private:
   using Bindings = TAny<VkDescriptorSetLayoutBinding>;

   void CreateDescriptorLayoutAndSet(const Bindings&, UBOLayout*, VkDescriptorSet*);
   void CreateUniformBuffers();
   void CreateNewSamplerSet();
   void CreateNewGeometrySet();

   TAny<TAny<Trait>> mUniforms;

   // Shaders                                                           
   Ptr<const VulkanShader> mStages[ShaderStage::Counter];
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

   static Construct FromFile(const A::File&);
   static Construct FromMesh(const A::Mesh&);
   static Construct FromImage(const A::Image&);
   static Construct FromCode(const Text&);
   void GenerateShaders(const A::Material&);

public:
   VulkanPipeline(VulkanRenderer*, const Neat&);
   ~VulkanPipeline();

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
   template<Rate RATE, CT::Trait TRAIT, CT::Data DATA>
   void SetUniform(const DATA& value, Offset index = 0) {
      if constexpr (CT::Same<DATA, VulkanTexture>) {
         static_assert(RATE == PerRenderable,
            "Setting a texture requires Rate::Renderable");
         // Set the sampler with the given index                        
         mSamplerUBO[mSubscribers.Last().samplerSet].Set(value, index);
      }
      else if constexpr (CT::Same<DATA, VulkanGeometry>) {
         static_assert(RATE == PerRenderable,
            "Setting a geometry stream requires Rate::Renderable");
         // Set the geometry stream                                     
         mGeometries[mSubscribers.Last().geometrySet] = value;
      }
      else if constexpr (RATE.IsStaticUniform()) {
         // Set a static uniform                                        
         (void)index;
         constexpr auto rate = RATE.GetStaticUniformIndex();
         mStaticUBO[rate].template Set<TRAIT, DATA>(value);
      }
      else if constexpr (RATE.IsDynamicUniform()) {
         // Set a dynamic uniform                                       
         (void)index;
         constexpr auto rate = RATE.GetDynamicUniformIndex();
         mDynamicUBO[rate].template Set<TRAIT, DATA>(value);
      }
      else LANGULUS_ERROR("Unsupported uniform rate");
   }

   /// Push the current samplers and dynamic uniforms, advancing indices      
   ///   @tparam RATE - the rate to push                                      
   ///   @tparam SUBSCRIBE - whether or not to subscribe for batched draw     
   ///   @return the subscriber, if SUBSCRIBE is true and RATE is dynamic     
   template<Rate RATE, bool SUBSCRIBE = true>
   NOD() auto PushUniforms() {
      if constexpr (RATE.IsStaticUniform()) {
         // Pushing static uniforms does nothing                        
         SAFETY(Logger::Warning(
            "Trying to push a static uniform block"
            " - although not fatal, it's suboptimal doing that"
         ));
      }
      else if constexpr (RATE.IsDynamicUniform()) {
         // Push a dynamic rate                                         
         constexpr auto rate = RATE.GetDynamicUniformIndex();

         mDynamicUBO[rate].Push();

         if constexpr (RATE == PerRenderable) {
            // When pushing PerRenderable state, create new sampler set 
            // and a new geometry set for next SetUniform calls         
            CreateNewSamplerSet();
            CreateNewGeometrySet();
         }

         if constexpr (RATE == PerInstance) {
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
   template<Rate RATE>
   NOD() auto GetRelevantDynamicUBOIndexOfRate() const noexcept {
      constexpr auto r = RATE.GetDynamicUniformIndex();
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
