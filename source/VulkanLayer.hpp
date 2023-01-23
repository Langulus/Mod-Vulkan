///                                                                           
/// Langulus::Module::Vulkan                                                  
/// Copyright(C) 2020 Dimo Markov <langulusteam@gmail.com>                    
///                                                                           
/// Distributed under GNU General Public License v3+                          
/// See LICENSE file, or https://www.gnu.org/licenses                         
///                                                                           
#pragma once
#include "VulkanCamera.hpp"
#include "VulkanRenderable.hpp"
#include "VulkanLight.hpp"
#include "inner/VulkanMemory.hpp"

struct LayerSubscriber {
   const VulkanPipeline* pipeline {};
   PipeSubscriber sub {};
};

struct RenderConfig {
   // Command buffer to render to                                       
   VkCommandBuffer mCommands;
   // Render pass to render to                                          
   const VkRenderPass& mPass;
   // Framebuffer to render to                                          
   const VkFramebuffer& mFrame;
   // Color clear values                                                
   VkClearValue mColorClear {};
   // Depth clear values                                                
   VkClearValue mDepthClear {};
   // Depth sweep                                                       
   VkClearAttachment mDepthSweep {};
   // Pass begin info                                                   
   mutable VkRenderPassBeginInfo mPassBeginInfo {};
};

using LevelSet = TOrderedSet<Level>;
using CameraSet = TUnorderedSet<const VulkanCamera*>;
using PipelineSet = TUnorderedSet<VulkanPipeline*>;


///                                                                           
///   Graphics layer unit                                                     
///                                                                           
/// A logical group of cameras, renderables, and lights, isolated from other  
/// layers. Useful for capsulating a GUI, for example.                        
///                                                                           
struct VulkanLayer : A::Graphics, ProducedFrom<VulkanRenderer> {
   LANGULUS(ABSTRACT) false;
   LANGULUS_BASES(A::Graphics);
   LANGULUS_VERBS(Verbs::Create);

protected:
   friend struct VulkanCamera;
   friend struct VulkanRenderable;
   friend struct VulkanLight;

   // List of cameras                                                   
   TFactory<VulkanCamera> mCameras;
   // List of rendererables                                             
   TFactory<VulkanRenderable> mRenderables;
   // List of lights                                                    
   TFactory<VulkanLight> mLights;
   // A cache of relevant pipelines                                     
   PipelineSet mRelevantPipelines;
   // A cache of relevant levels                                        
   LevelSet mRelevantLevels;
   // A cache of relevant cameras                                       
   CameraSet mRelevantCameras;

   // Subscribers, used only for hierarchical styled layers             
   // Otherwise, VulkanPipeline::Subscriber is used                     
   TAny<LayerSubscriber> mSubscribers;
   TAny<Count> mSubscriberCountPerLevel;
   TAny<Count> mSubscriberCountPerCamera;

   /// The layer style determines how the scene will be compiled              
   /// Combine these flags to configure the layer to your needs               
   enum Style {
      /// Batched layers are compiled for optimal performance, by grouping    
      /// all similar renderables and drawing them at once                    
      /// This is the opposite of hierarchical rendering, because   it        
      /// destroys the order in which renderables appear                      
      /// It is best suited for non-blended depth-tested scenes               
      Batched = 0,

      /// Hierarchical layers preserve the order in which elements occur      
      /// It is the opposite of batched rendering, because structure isn't    
      /// lost. This style is a bit less efficient, but is mandatory for      
      /// rendering UI for example                                            
      Hierarchical = 1,

      /// A multilevel layer supports instances that are not in the default   
      /// human level. It is useful for rendering objects of the size of      
      /// the universe, or of the size of atoms, depending on the camera      
      /// configuration. Works by rendering the biggest levels first,         
      /// working down to the camera's capabilities, clearing the depth       
      /// after each successive level. This way one can seamlessly            
      /// compose infinitely complex scenes. Needless to say, this incurs     
      /// some performance penalty                                            
      Multilevel = 2,

      /// If enabled, will separate light computation on a different pass     
      /// Significantly improves performance on scenes with complex           
      /// lighting and shadowing                                              
      DeferredLights = 4,

      /// If enabled will sort instances by distance to camera, before        
      /// committing them for rendering                                       
      Sorted = 8,

      /// The default visual layer style                                      
      Default = Batched | Multilevel
   };

   Style mStyle = Style::Default;

public:
   VulkanLayer(VulkanRenderer*, const Any&);

   void Create(Verb&);

   bool Generate(PipelineSet&);
   void Render(const RenderConfig&) const;

private:
   void CompileCameras();

   Count CompileLevelBatched(const Matrix4&, const Matrix4&, Level, PipelineSet&);
   Count CompileLevelHierarchical(const Matrix4&, const Matrix4&, Level, PipelineSet&);
   Count CompileThing(const Thing*, LodState&, PipelineSet&);
   VulkanPipeline* CompileInstance(const VulkanRenderable*, const Unit*, LodState&);
   Count CompileLevels();

   void RenderBatched(const RenderConfig&) const;
   void RenderHierarchical(const RenderConfig&) const;
};
