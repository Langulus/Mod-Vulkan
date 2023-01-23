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
   : GraphicsUnit {MetaOf<VulkanLayer>(), descriptor} 
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
///   @return true if something needs to be rendered                          
bool VulkanLayer::Generate(PipelineSet& pipelines) {
   CompileCameras();
   auto n = CompileLevels();
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
   const VulkanRenderable* renderable, const Unit* instance, LodState& lod
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
      lod.Transform(instance->GetModelTransform(lod.mLevel));
   }

   // Get relevant pipeline                                             
   auto pipeline = renderable->GetOrCreatePipeline(lod, this);
   if (!pipeline)
      return nullptr;
   pipeline->Initialize();

   // Get relevant geometry                                             
   const auto geometry = renderable->GetGeometry(lod);
   if (geometry)
      pipeline->SetUniform<Rate::Renderable, Traits::Geometry>(geometry);

   // Get relevant textures                                             
   const auto textures = renderable->GetTextures(lod);
   if (textures)
      pipeline->SetUniform<Rate::Renderable, Traits::Texture>(textures);

   // Push uniforms                                                     
   pipeline->SetUniform<Rate::Instance, Traits::Transformation>(lod.mModel);
   return pipeline;
}

/// Compile an entity and all of its children entities                        
/// Used only for hierarchical styled layers                                  
///   @param thing - entity to compile                                        
///   @param lod - the lod state to use                                       
///   @param pipesPerCamera - [out] pipelines used by the hierarchy           
///   @return 1 if something from the hierarchy was rendered                  
Count VulkanLayer::CompileThing(const Thing* thing, LodState& lod, PipelineSet& pipesPerCamera) {
   // Iterate all renderables of the entity, which are part of this     
   // layer - disregard all others                                      
   auto relevantRenderables = thing->GatherUnits<VulkanRenderable, SeekStyle::Here>();

   // Compile the instances associated with these renderables           
   Count renderedInstances {};
   for (auto renderable : relevantRenderables) {
      if (renderable->mInstances.IsEmpty()) {
         // Imagine a default instance                                  
         auto pipeline = CompileInstance(renderable, nullptr, lod);
         if (pipeline) {
            const auto sub = pipeline->PushUniforms<Rate::Instance, false>();
            pipeline->PushUniforms<Rate::Renderable, false>();
            pipesPerCamera << pipeline;
            mRelevantPipelines << pipeline;

            auto& s = mSubscribers.Last();
            s.pipeline = pipeline;
            s.sub = sub;
            mSubscribers.New(1);

            ++mSubscriberCountPerLevel.Last();
            ++mSubscriberCountPerCamera.Last();
            ++renderedInstances;
         }
      }
      else for (auto instance : renderable->mInstances) {
         // Compile each available instance                             
         auto pipeline = CompileInstance(renderable, instance, lod);
         if (pipeline) {
            const auto sub = pipeline->PushUniforms<Rate::Instance, false>();
            pipeline->PushUniforms<Rate::Renderable, false>();
            pipesPerCamera << pipeline;
            mRelevantPipelines << pipeline;

            auto& s = mSubscribers.Last();
            s.pipeline = pipeline;
            s.sub = sub;
            mSubscribers.New(1);

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
   // Construct view and frustum   for culling                          
   LodState lod;
   lod.mLevel = level;
   lod.mView = view;
   lod.mViewInverted = lod.mView.Invert();
   lod.mFrustum = lod.mViewInverted * projection;

   // Nest-iterate all children of the layer owner                      
   Count renderedEntities {};
   for (auto owner : GetOwners())
      renderedEntities += CompileThing(owner, lod, pipesPerCamera);
   
   if (renderedEntities) {
      for (auto pipeline : pipesPerCamera) {
         // Push PerLevel uniforms if required                          
         const auto projectedView = lod.mViewInverted * projection;
         const auto projectedViewInverted = projectedView.Invert();

         pipeline->SetUniform<Rate::Level, Traits::ViewTransform>(lod.mView);
         pipeline->SetUniform<Rate::Level, Traits::ViewTransformInverted>(lod.mViewInverted);
         pipeline->SetUniform<Rate::Level, Traits::ViewProjectTransform>(projectedView);
         pipeline->SetUniform<Rate::Level, Traits::ViewProjectTransformInverted>(projectedViewInverted);
         pipeline->SetUniform<Rate::Level, Traits::Level>(level);
         pipeline->PushUniforms<Rate::Level, false>();
      }

      mSubscriberCountPerLevel.New(1);

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
   LodState lod;
   lod.mLevel = level;
   lod.mView = view;
   lod.mViewInverted = lod.mView.Invert();
   lod.mFrustum = lod.mViewInverted * projection;

   // Iterate all renderables                                           
   Count renderedInstances {};
   for (auto& renderable : mRenderables) {
      if (renderable.mInstances.IsEmpty()) {
         auto pipeline = CompileInstance(&renderable, nullptr, lod);
         if (pipeline) {
            pipeline->PushUniforms<Rate::Instance>();
            pipeline->PushUniforms<Rate::Renderable>();
            pipesPerCamera << pipeline;
            mRelevantPipelines << pipeline;
            ++renderedInstances;
         }
      }
      else for (auto instance : renderable.mInstances) {
         auto pipeline = CompileInstance(&renderable, instance, lod);
         if (pipeline) {
            pipeline->PushUniforms<Rate::Instance>();
            pipeline->PushUniforms<Rate::Renderable>();
            pipesPerCamera << pipeline;
            mRelevantPipelines << pipeline;
            ++renderedInstances;
         }
      }
   }

   if (renderedInstances) {
      for (auto pipeline : pipesPerCamera) {
         // Push PerLevel uniforms if required                          
         const auto projectedView = lod.mViewInverted * projection;
         const auto projectedViewInverted = projectedView.Invert();

         pipeline->SetUniform<Rate::Level, Traits::ViewTransform>(lod.mView);
         pipeline->SetUniform<Rate::Level, Traits::ViewTransformInverted>(lod.mViewInverted);
         pipeline->SetUniform<Rate::Level, Traits::ViewProjectTransform>(projectedView);
         pipeline->SetUniform<Rate::Level, Traits::ViewProjectTransformInverted>(projectedViewInverted);
         pipeline->SetUniform<Rate::Level, Traits::Level>(level);
         pipeline->PushUniforms<Rate::Level>();

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
            pipeline->SetUniform<Rate::Camera, Traits::ProjectTransform>(Matrix4 {});
            pipeline->SetUniform<Rate::Camera, Traits::ProjectTransformInverted>(Matrix4 {});
            pipeline->SetUniform<Rate::Camera, Traits::FOV>(Radians {});
            pipeline->SetUniform<Rate::Camera, Traits::Resolution>(GetWindow()->GetSize());
            if (mStyle & Style::Hierarchical)
               pipeline->PushUniforms<Rate::Camera, false>();
            else
               pipeline->PushUniforms<Rate::Camera>();
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
         const auto view = camera.GetViewTransform(Level::Default);
         if (mStyle & Style::Hierarchical)
            CompileLevelHierarchical(view, camera.mProjection, {}, pipesPerCamera);
         else
            CompileLevelBatched(view, camera.mProjection, {}, pipesPerCamera);
      }
      else continue;

      if (!pipesPerCamera.IsEmpty()) {
         for (auto pipeline : pipesPerCamera) {
            // Push PerCamera uniforms if required                      
            pipeline->SetUniform<Rate::Camera, Traits::ProjectTransform>(camera.mProjection);
            pipeline->SetUniform<Rate::Camera, Traits::ProjectTransformInverted>(camera.mProjectionInverted);
            pipeline->SetUniform<Rate::Camera, Traits::FOV>(camera.mFOV);
            pipeline->SetUniform<Rate::Camera, Traits::Resolution>(camera.mResolution);
            if (mStyle & Style::Hierarchical)
               pipeline->PushUniforms<Rate::Camera, false>();
            else
               pipeline->PushUniforms<Rate::Camera>();
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
      viewport.width = GetProducer()->GetResolution()[0];
      viewport.height = GetProducer()->GetResolution()[1];

      VkRect2D scissor {};
      scissor.extent.width = uint32_t(viewport.width);
      scissor.extent.height = uint32_t(viewport.height);

      vkCmdBeginRenderPass(config.mCommands, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
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
            vkCmdClearAttachments(config.mCommands, 1, &depthsweep, 1, &rect);
         }
      }

      // Main pass ends                                                 
      vkCmdEndRenderPass(config.mCommands);
   }
   else for (const auto camera : mRelevantCameras) {
      renderPassInfo.renderArea.extent.width = camera->mResolution[0];
      renderPassInfo.renderArea.extent.height = camera->mResolution[1];

      vkCmdBeginRenderPass(config.mCommands, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
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
            vkCmdClearAttachments(config.mCommands, 1, &depthsweep, 1, &rect);
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
      viewport.width = GetProducer()->GetResolution()[0];
      viewport.height = GetProducer()->GetResolution()[1];

      VkRect2D scissor {};
      scissor.extent.width = static_cast<uint32_t>(viewport.width);
      scissor.extent.height = static_cast<uint32_t>(viewport.height);

      vkCmdBeginRenderPass(config.mCommands, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
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
            vkCmdClearAttachments(config.mCommands, 1, &depthsweep, 1, &rect);
         }

         subscribersDone += *subscriberCountPerLevel;
         ++subscriberCountPerLevel;
      }

      // Main pass ends                                                 
      vkCmdEndRenderPass(config.mCommands);
   }
   else for (const auto camera : mRelevantCameras) {
      renderPassInfo.renderArea.extent.width = camera->mResolution[0];
      renderPassInfo.renderArea.extent.height = camera->mResolution[1];

      vkCmdBeginRenderPass(config.mCommands, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
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
            vkCmdClearAttachments(config.mCommands, 1, &depthsweep, 1, &rect);
         }

         subscribersDone += *subscriberCountPerLevel;
         ++subscriberCountPerLevel;
      }

      // Main pass ends                                                 
      vkCmdEndRenderPass(config.mCommands);
   }
}
