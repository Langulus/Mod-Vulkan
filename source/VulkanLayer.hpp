///                                                                           
/// Langulus::Module::Vulkan                                                  
/// Copyright(C) 2020 Dimo Markov <langulusteam@gmail.com>                    
///                                                                           
/// Distributed under GNU General Public License v3+                          
/// See LICENSE file, or https://www.gnu.org/licenses                         
///                                                                           
#pragma once
#include "VRAM.hpp"
#include "VulkanCamera.hpp"
#include "VulkanRenderable.hpp"
#include "VulkanLight.hpp"

class MVulkan;

using LevelSet = std::set<Level>;
using CameraSet = std::unordered_set<const CVulkanCamera*>;
using PipelineSet = std::unordered_set<CVulkanPipeline*>;

///                                                                           
///   LAYER COMPONENT                                                         
///                                                                           
/// Manages cameras, renderables and lights in a specific way                 
///                                                                           
class CVulkanLayer : public TProducedFrom<CVulkanRenderer> {
   // List of cameras                                                   
   TFactory<CVulkanCamera> mCameras;
   // List of rendererables                                             
   TFactory<CVulkanRenderable> mRenderables;
   // List of lights                                                    
   TFactory<CVulkanLight> mLights;
   // A cache of relevant pipelines                                     
   PipelineSet mRelevantPipelines;
   // A cache of relevant levels                                        
   LevelSet mRelevantLevels;
   // A cache of relevant cameras                                       
   CameraSet mRelevantCameras;

   // Subscribers, used only for hierarchical styled layers             
   // Otherwise, CVulkanPipeline::Subscriber is used                    
   std::vector<LayerSubscriber> mSubscribers;
   std::vector<pcptr> mSubscriberCountPerLevel;
   std::vector<pcptr> mSubscriberCountPerCamera;

   /// The layer style determines how the scene will be compiled				
   /// Combine these flags to configure the layer to your needs				
   enum Style {
      /// Batched layers are compiled for optimal performance, by grouping	
      /// all similar renderables and drawing them at once						
      /// This is the opposite of hierarchical rendering, because	it			
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

protected:
   Style mStyle = Style::Default;

public:
   REFLECT(CVulkanLayer);
   CVulkanLayer(CVulkanRenderer*);

   PC_VERB(Create);

   bool Generate(PipelineSet&);
   void Render(VkCommandBuffer, const VkRenderPass&, const VkFramebuffer&) const;
   const AWindow* GetWindow() const;

private:
   void CompileCameras();
   pcptr CompileLevelBatched(const mat4&, const mat4&, Level, PipelineSet&);
   pcptr CompileLevelHierarchical(const mat4&, const mat4&, Level, PipelineSet&);
   pcptr CompileEntity(const Entity*, LodState&, PipelineSet&);
   CVulkanPipeline* CompileInstance(CVulkanRenderable*, const AInstance*, LodState&);
   pcptr CompileLevels();
   void RenderBatched(VkCommandBuffer, const VkRenderPass&, const VkFramebuffer&) const;
   void RenderHierarchical(VkCommandBuffer, const VkRenderPass&, const VkFramebuffer&) const;
};
