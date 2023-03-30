///                                                                           
/// Langulus::Module::Vulkan                                                  
/// Copyright(C) 2020 Dimo Markov <langulusteam@gmail.com>                    
///                                                                           
/// Distributed under GNU General Public License v3+                          
/// See LICENSE file, or https://www.gnu.org/licenses                         
///                                                                           
#include "Vulkan.hpp"
#include <iterator>


/// Descriptor constructor                                                    
///   @param producer - the camera producer                                   
///   @param descriptor - the camera descriptor                               
VulkanLayer::VulkanLayer(VulkanRenderer* producer, const Any& descriptor)
   : Graphics {MetaOf<VulkanLayer>(), descriptor} 
   , ProducedFrom {producer, descriptor}
   , mCameras {this}
   , mRenderables {this}
   , mLights {this} {}

/// Create/destroy renderables, cameras, lights                               
///   @param verb - creation verb                                             
void VulkanLayer::Create(Verb& verb) {
   mCameras.Create(verb);
   mRenderables.Create(verb);
   mLights.Create(verb);
}

/// Generate the draw list for the layer                                      
///   @param pipelines - [out] a set of all used pipelines                    
///   @return true if anything renderable was generated                       
bool VulkanLayer::Generate(PipelineSet& pipelines) {
   CompileCameras();
   CompileLevels();
   return pipelines.Merge(mRelevantPipelines);
}

/// Compile the camera transformations                                        
void VulkanLayer::CompileCameras() {
   for (auto& camera : mCameras)
      camera.Compile();
}

/// Compile a single renderable instance, culling it if able                  
/// This will create or reuse a pipeline, capable of rendering it             
///   @param renderable - the renderable to compile                           
///   @param instance - the instance to compile                               
///   @param lod - the lod state to use                                       
///   @return the pipeline if instance is relevant                            
VulkanPipeline* VulkanLayer::CompileInstance(
   const VulkanRenderable* renderable, const A::Instance* instance, LOD& lod
) {
   if (!instance) {
      // No instances, so culling based only on default level           
      if (lod.mLevel != Level::Default)
         return nullptr;
      lod.Transform();
   }
   else {
      // Instance available, so cull                                    
      if (instance->Cull(lod))
         return nullptr;
      lod.Transform(instance->GetModelTransform(lod));
   }

   // Get relevant pipeline                                             
   auto pipeline = renderable->GetOrCreatePipeline(lod, this);
   if (!pipeline)
      return nullptr;

   // Get relevant geometry                                             
   const auto geometry = renderable->GetGeometry(lod);
   if (geometry) {
      pipeline->template
         SetUniform<PerRenderable, Traits::Geometry>(geometry);
   }

   // Get relevant textures                                             
   const auto textures = renderable->GetTexture(lod);
   if (textures) {
      pipeline->template
         SetUniform<PerRenderable, Traits::Texture>(textures);
   }

   // Push uniforms                                                     
   pipeline->template
      SetUniform<PerInstance, Traits::Transform>(lod.mModel);
   return pipeline;
}

/// Compile an entity and all of its children entities                        
/// Used only for hierarchical styled layers                                  
///   @param thing - entity to compile                                        
///   @param lod - the lod state to use                                       
///   @param pipesPerCamera - [out] pipelines used by the hierarchy           
///   @return 1 if something from the hierarchy was rendered                  
Count VulkanLayer::CompileThing(const Thing* thing, LOD& lod, PipelineSet& pipesPerCamera) {
   // Iterate all renderables of the entity, which are part of this     
   // layer - disregard all others                                      
   auto relevantRenderables = thing->GatherUnits<VulkanRenderable, Seek::Here>();

   // Compile the instances associated with these renderables           
   Count renderedInstances {};
   for (auto renderable : relevantRenderables) {
      if (renderable->mInstances.IsEmpty()) {
         // Imagine a default instance                                  
         auto pipeline = CompileInstance(renderable, nullptr, lod);
         if (pipeline) {
            const auto sub = pipeline->PushUniforms<PerInstance, false>();
            pipeline->PushUniforms<PerRenderable, false>();
            pipesPerCamera << pipeline;
            mRelevantPipelines << pipeline;

            auto& s = mSubscribers.Last();
            s.pipeline = pipeline;
            s.sub = sub;
            mSubscribers.New();

            ++mSubscriberCountPerLevel.Last();
            ++mSubscriberCountPerCamera.Last();
            ++renderedInstances;
         }
      }
      else for (auto instance : renderable->mInstances) {
         // Compile each available instance                             
         auto pipeline = CompileInstance(renderable, instance, lod);
         if (pipeline) {
            const auto sub = pipeline->PushUniforms<PerInstance, false>();
            pipeline->PushUniforms<PerRenderable, false>();
            pipesPerCamera << pipeline;
            mRelevantPipelines << pipeline;

            auto& s = mSubscribers.Last();
            s.pipeline = pipeline;
            s.sub = sub;
            mSubscribers.New();

            ++mSubscriberCountPerLevel.Last();
            ++mSubscriberCountPerCamera.Last();
            ++renderedInstances;
         }
      }
   }

   // Nest to children                                                  
   for (auto child : thing->GetChildren())
      renderedInstances += CompileThing(child, lod, pipesPerCamera);

   return renderedInstances > 0;
}

/// Compile a single level's instances hierarchical style                     
///   @param view - the camera view matrix                                    
///   @param projection - the camera projection matrix                        
///   @param level - the level to compile                                     
///   @param pipesPerCamera - [out] pipeline set for the current level only   
///   @return the number of compiled entities                                 
Count VulkanLayer::CompileLevelHierarchical(
   const Matrix4& view, const Matrix4& projection, 
   Level level, PipelineSet& pipesPerCamera
) {
   // Construct view and frustum for culling                            
   LOD lod {level, view, projection};

   // Nest-iterate all children of the layer owner                      
   Count renderedEntities {};
   for (const auto owner : GetOwners())
      renderedEntities += CompileThing(owner, lod, pipesPerCamera);
   
   if (renderedEntities) {
      for (auto pipeline : pipesPerCamera) {
         // Push PerLevel uniforms                                      
         pipeline->SetUniform<PerLevel, Traits::View>(lod.mView);
         pipeline->SetUniform<PerLevel, Traits::Projection>(projection);
         pipeline->SetUniform<PerLevel, Traits::Level>(level);
         pipeline->PushUniforms<PerLevel, false>();
      }

      mSubscriberCountPerLevel.New();

      // Store the negative level in the set, so they're always in      
      // a descending order                                             
      mRelevantLevels << -level;
   }

   return renderedEntities;
}

/// Compile a single level's instances batched style                          
///   @param view - the camera view matrix                                    
///   @param projection - the camera projection matrix                        
///   @param level - the level to compile                                     
///   @param pipesPerCamera - [out] pipeline set for the current level only   
///   @return 1 if anything was rendered, zero otherwise                      
Count VulkanLayer::CompileLevelBatched(
   const Matrix4& view, const Matrix4& projection, 
   Level level, PipelineSet& pipesPerCamera
) {
   // Construct view and frustum   for culling                          
   LOD lod {level, view, projection};

   // Iterate all renderables                                           
   Count renderedInstances {};
   for (const auto& renderable : mRenderables) {
      if (renderable.mInstances.IsEmpty()) {
         auto pipeline = CompileInstance(&renderable, nullptr, lod);
         if (pipeline) {
            pipeline->PushUniforms<PerInstance>();
            pipeline->PushUniforms<PerRenderable>();
            pipesPerCamera << pipeline;
            mRelevantPipelines << pipeline;
            ++renderedInstances;
         }
      }
      else for (auto instance : renderable.mInstances) {
         auto pipeline = CompileInstance(&renderable, instance, lod);
         if (pipeline) {
            pipeline->PushUniforms<PerInstance>();
            pipeline->PushUniforms<PerRenderable>();
            pipesPerCamera << pipeline;
            mRelevantPipelines << pipeline;
            ++renderedInstances;
         }
      }
   }

   if (renderedInstances) {
      for (auto pipeline : pipesPerCamera) {
         // Push PerLevel uniforms if required                          
         pipeline->SetUniform<PerLevel, Traits::View>(lod.mView);
         pipeline->SetUniform<PerLevel, Traits::Projection>(projection);
         pipeline->SetUniform<PerLevel, Traits::Level>(level);
         pipeline->PushUniforms<PerLevel>();

         // Store the negative level in the set, so they're always in   
         // a descending order                                          
         mRelevantLevels << -level;
      }

      return 1;
   }

   return 0;
}

/// Compile all levels and their instances                                    
///   @return the number of relevant cameras                                  
Count VulkanLayer::CompileLevels() {
   Count renderedCameras {};
   mRelevantLevels.Clear();
   mRelevantPipelines.Clear();

   if (mStyle & Style::Hierarchical) {
      mSubscribers.Clear();
      mSubscribers.New(1);
      mSubscriberCountPerLevel.Clear();
      mSubscriberCountPerLevel.New(1);
      mSubscriberCountPerCamera.Clear();
      mSubscriberCountPerCamera.New(1);
   }

   if (mCameras.IsEmpty()) {
      // No camera, so just render default level on the whole screen    
      PipelineSet pipesPerCamera;
      if (mStyle & Style::Hierarchical)
         CompileLevelHierarchical({}, {}, {}, pipesPerCamera);
      else
         CompileLevelBatched({}, {}, {}, pipesPerCamera);

      if (!pipesPerCamera.IsEmpty()) {
         for (auto pipeline : pipesPerCamera) {
            // Push PerCamera uniforms if required                      
            pipeline->SetUniform<PerCamera, Traits::Projection>(Matrix4 {});
            pipeline->SetUniform<PerCamera, Traits::FOV>(Radians {});
            pipeline->SetUniform<PerCamera, Traits::Size>(GetWindow()->GetSize());
            if (mStyle & Style::Hierarchical)
               pipeline->PushUniforms<PerCamera, false>();
            else
               pipeline->PushUniforms<PerCamera>();
         }

         if (mStyle & Style::Hierarchical)
            mSubscriberCountPerCamera.New(1);
         ++renderedCameras;
      }
   }
   else for (const auto& camera : mCameras) {
      // Iterate all levels per camera                                  
      PipelineSet pipesPerCamera;
      if (mStyle & Style::Multilevel) {
         // Multilevel style - tests all camera-visible levels          
         for (auto level = camera.mObservableRange.mMax; level >= camera.mObservableRange.mMin; --level) {
            const auto view = camera.GetViewTransform(level);
            if (mStyle & Style::Hierarchical)
               CompileLevelHierarchical(view, camera.mProjection, level, pipesPerCamera);
            else
               CompileLevelBatched(view, camera.mProjection, level, pipesPerCamera);
         }
      }
      else if (camera.mObservableRange.Inside(Level::Default)) {
         // Default level style - checks only if camera sees default    
         const auto view = camera.GetViewTransform();
         if (mStyle & Style::Hierarchical)
            CompileLevelHierarchical(view, camera.mProjection, {}, pipesPerCamera);
         else
            CompileLevelBatched(view, camera.mProjection, {}, pipesPerCamera);
      }
      else continue;

      if (!pipesPerCamera.IsEmpty()) {
         for (auto pipeline : pipesPerCamera) {
            // Push PerCamera uniforms if required                      
            pipeline->SetUniform<PerCamera, Traits::Projection>(camera.mProjection);
            pipeline->SetUniform<PerCamera, Traits::FOV>(camera.mFOV);
            pipeline->SetUniform<PerCamera, Traits::Size>(camera.mResolution);
            if (mStyle & Style::Hierarchical)
               pipeline->PushUniforms<PerCamera, false>();
            else
               pipeline->PushUniforms<PerCamera>();
         }

         if (mStyle & Style::Hierarchical)
            mSubscriberCountPerCamera.New(1);
         mRelevantCameras << &camera;
         ++renderedCameras;
      }
   }

   return renderedCameras;
}

/// Render the layer to a specific command buffer and framebuffer             
///   @param config - where to render to                                      
void VulkanLayer::Render(const RenderConfig& config) const {
   if (mStyle & Style::Hierarchical)
      RenderHierarchical(config);
   else
      RenderBatched(config);
}

/// Render the layer to a specific command buffer and framebuffer             
/// This is used only for Batched style layers, and relies on previously      
/// compiled set of pipelines                                                 
///   @param config - where to render to                                      
void VulkanLayer::RenderBatched(const RenderConfig& config) const {
   // Iterate all valid cameras                                         
   TUnorderedMap<const VulkanPipeline*, Count> done;

   if (mRelevantCameras.IsEmpty()) {
      VkViewport viewport {};
      viewport.width = GetProducer()->mResolution[0];
      viewport.height = GetProducer()->mResolution[1];

      VkRect2D scissor {};
      scissor.extent.width = static_cast<uint32_t>(viewport.width);
      scissor.extent.height = static_cast<uint32_t>(viewport.height);

      vkCmdBeginRenderPass(config.mCommands, &config.mPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);
      vkCmdSetViewport(config.mCommands, 0, 1, &viewport);
      vkCmdSetScissor(config.mCommands, 0, 1, &scissor);

      // Iterate all relevant levels                                    
      for (const auto& level : mRelevantLevels) {
         // Draw all subscribers to the pipeline for the current level  
         for (auto pipeline : mRelevantPipelines)
            done[pipeline] = pipeline->RenderLevel(done[pipeline]);

         if (level != mRelevantLevels.Last()) {
            // Clear depth after rendering this level (if not last)     
            const VkClearRect rect {scissor, 0, 1};
            vkCmdClearAttachments(config.mCommands, 1, &config.mDepthSweep, 1, &rect);
         }
      }

      // Main pass ends                                                 
      vkCmdEndRenderPass(config.mCommands);
   }
   else for (const auto& camera : mRelevantCameras) {
      config.mPassBeginInfo.renderArea.extent.width = camera->mResolution[0];
      config.mPassBeginInfo.renderArea.extent.height = camera->mResolution[1];

      vkCmdBeginRenderPass(config.mCommands, &config.mPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);
      vkCmdSetViewport(config.mCommands, 0, 1, &camera->mVulkanViewport);
      vkCmdSetScissor(config.mCommands, 0, 1, &camera->mVulkanScissor);

      // Iterate all relevant levels                                    
      for (const auto& level : mRelevantLevels) {
         // Draw all subscribers to the pipeline for the current level  
         for (auto pipeline : mRelevantPipelines)
            done[pipeline] = pipeline->RenderLevel(done[pipeline]);

         if (level != mRelevantLevels.Last()) {
            // Clear depth after rendering this level (if not last)     
            const VkClearRect rect {camera->mVulkanScissor, 0, 1};
            vkCmdClearAttachments(config.mCommands, 1, &config.mDepthSweep, 1, &rect);
         }
      }

      // Main pass ends                                                 
      vkCmdEndRenderPass(config.mCommands);
   }
}

/// Render the layer to a specific command buffer and framebuffer             
/// This is used only for Hierarchical style layers, and relies on locally    
/// compiled subscribers, rendering them in their respective order            
///   @param config - where to render to                                      
void VulkanLayer::RenderHierarchical(const RenderConfig& config) const {
   Count subscribersDone {};
   auto subscriberCountPerLevel = &mSubscriberCountPerLevel[0];

   // Iterate all valid cameras                                         
   if (mRelevantCameras.IsEmpty()) {
      VkViewport viewport {};
      viewport.width = GetProducer()->mResolution[0];
      viewport.height = GetProducer()->mResolution[1];

      VkRect2D scissor {};
      scissor.extent.width = static_cast<uint32_t>(viewport.width);
      scissor.extent.height = static_cast<uint32_t>(viewport.height);

      vkCmdBeginRenderPass(config.mCommands, &config.mPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);
      vkCmdSetViewport(config.mCommands, 0, 1, &viewport);
      vkCmdSetScissor(config.mCommands, 0, 1, &scissor);

      // Iterate all relevant levels                                    
      for (const auto& level : mRelevantLevels) {
         // Draw all subscribers to the pipeline for the current level  
         for (Count s = 0; s < *subscriberCountPerLevel; ++s) {
            auto& subscriber = mSubscribers[subscribersDone + s];
            subscriber.pipeline->RenderSubscriber(subscriber.sub);
         }

         if (level != mRelevantLevels.Last()) {
            // Clear depth after rendering this level (if not last)     
            const VkClearRect rect {scissor, 0, 1};
            vkCmdClearAttachments(config.mCommands, 1, &config.mDepthSweep, 1, &rect);
         }

         subscribersDone += *subscriberCountPerLevel;
         ++subscriberCountPerLevel;
      }

      // Main pass ends                                                 
      vkCmdEndRenderPass(config.mCommands);
   }
   else for (const auto& camera : mRelevantCameras) {
      config.mPassBeginInfo.renderArea.extent.width = camera->mResolution[0];
      config.mPassBeginInfo.renderArea.extent.height = camera->mResolution[1];

      vkCmdBeginRenderPass(config.mCommands, &config.mPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);
      vkCmdSetViewport(config.mCommands, 0, 1, &camera->mVulkanViewport);
      vkCmdSetScissor(config.mCommands, 0, 1, &camera->mVulkanScissor);

      // Iterate all relevant levels                                    
      for (const auto& level : mRelevantLevels) {
         // Draw all subscribers to the pipeline for the current level  
         // and camera                                                  
         for (Count s = 0; s < *subscriberCountPerLevel; ++s) {
            auto& subscriber = mSubscribers[subscribersDone + s];
            subscriber.pipeline->RenderSubscriber(subscriber.sub);
         }

         if (level != mRelevantLevels.Last()) {
            // Clear depth after rendering this level (if not last)     
            const VkClearRect rect {camera->mVulkanScissor, 0, 1};
            vkCmdClearAttachments(config.mCommands, 1, &config.mDepthSweep, 1, &rect);
         }

         subscribersDone += *subscriberCountPerLevel;
         ++subscriberCountPerLevel;
      }

      // Main pass ends                                                 
      vkCmdEndRenderPass(config.mCommands);
   }
}

/// Get the style of a layer                                                  
///   @return the layer style                                                 
VulkanLayer::Style VulkanLayer::GetStyle() const noexcept {
   return mStyle;
}

/// Get the window of a layer                                                 
///   @return the window interface                                            
const A::Window* VulkanLayer::GetWindow() const noexcept {
   return mProducer->GetWindow();
}
