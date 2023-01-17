///                                                                           
/// Langulus::Module::Vulkan                                                  
/// Copyright(C) 2020 Dimo Markov <langulusteam@gmail.com>                    
///                                                                           
/// Distributed under GNU General Public License v3+                          
/// See LICENSE file, or https://www.gnu.org/licenses                         
///                                                                           
#include "Vulkan.hpp"
#include <iterator>

//#define PC_REPORT_RENDER_STATISTICS

/// Vulkan renderer construction                                              
///   @param producer - producer of the renderer                              
VulkanLayer::VulkanLayer(VulkanRenderer* producer)
   : AVisualLayer {MetaData::Of<VulkanLayer>()}
   , TProducedFrom {producer}
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
///   @param relevantPipelines - [out] a set of all used pipelines            
///   @return true if something needs to be rendered                          
bool VulkanLayer::Generate(PipelineSet& relevantPipelines) {
   CompileCameras();
   auto n = CompileLevels();
   for (auto p : mRelevantPipelines)
      relevantPipelines << p;
   return n > 0;
}

/// Get the window of the renderer                                            
///   @return the window pointers                                             
const Unit* VulkanLayer::GetWindow() const {
   return GetProducer()->GetWindow();
}

/// Compile the camera transformations                                        
void VulkanLayer::CompileCameras() {
   for (auto& camera : mCameras) {
      if (camera.IsClassIrrelevant())
         continue;
      camera.Compile();
   }
}

/// Compile a single renderable instance, culling it if able                  
/// This will create or reuse a pipeline, capable of rendering it             
///   @param renderable - the renderable to compile                           
///   @param instance - the instance to compile                               
///   @param lod - the lod state to use                                       
///   @return the pipeline if instance is relevant                            
VulkanPipeline* VulkanLayer::CompileInstance(VulkanRenderable* renderable, const Unit* instance, LodState& lod) {
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
      pipeline->SetUniform<RRate::Renderable, Traits::Geometry>(geometry);

   // Get relevant textures                                             
   const auto textures = renderable->GetTextures(lod);
   if (textures)
      pipeline->SetUniform<RRate::Renderable, Traits::Texture>(textures);

   // Push uniforms                                                     
   pipeline->SetUniform<RRate::Instance, Traits::Transformation>(lod.mModel);
   return pipeline;
}

/// Compile an entity and all of its children entities                        
/// Used only for hierarchical styled layers                                  
///   @param entity - entity to compile                                       
///   @param lod - the lod state to use                                       
///   @param pipesPerCamera - [out] pipelines used by the hierarchy           
///   @return 1 if something from the hierarchy was rendered                  
Count VulkanLayer::CompileThing(const Thing* thing, LodState& lod, PipelineSet& pipesPerCamera) {
   // Iterate all renderables of the entity, which are part of this     
   // layer - disregard all others                                      
   TAny<VulkanRenderable*> relevantRenderables;
   for (auto unit : thing->GetUnits()) {
      auto renderable = dynamic_cast<VulkanRenderable*>(unit);
      if (renderable && !renderable->IsClassIrrelevant() && mRenderables.Owns(renderable))
         relevantRenderables << renderable;
   }

   // Compile the instances associated with these renderables           
   Count renderedInstances {};
   for (auto renderable : relevantRenderables) {
      if (renderable->GetInstances().IsEmpty()) {
         // Imagine a default instance                                  
         auto pipeline = CompileInstance(renderable, nullptr, lod);
         if (pipeline) {
            const auto sub = pipeline->PushUniforms<RRate::Instance, false>();
            pipeline->PushUniforms<RRate::Renderable, false>();
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
      else for (auto instance : renderable->GetInstances()) {
         // Compile each available instance                             
         if (instance->IsClassIrrelevant())
            continue;

         auto pipeline = CompileInstance(renderable, instance, lod);
         if (pipeline) {
            const auto sub = pipeline->PushUniforms<RRate::Instance, false>();
            pipeline->PushUniforms<RRate::Renderable, false>();
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
   for (auto& child : thing->GetChildren()) {
      if (child->IsClassIrrelevant())
         continue;
      renderedInstances += CompileThing(child, lod, pipesPerCamera);
   }

   return renderedInstances > 0;
}

/// Compile a single level's instances hierarchical style                     
///   @param view - the camera view matrix                                    
///   @param projection - the camera projection matrix                        
///   @param level - the level to compile                                     
///   @param pipesPerCamera - [out] pipeline set for the current level only   
///   @return the number of compiled entities                                 
Count VulkanLayer::CompileLevelHierarchical(
   const Matrix4& view, 
   const Matrix4& projection, 
   Level level, 
   PipelineSet& pipesPerCamera
) {
   // Construct view and frustum   for culling                          
   LodState lod;
   lod.mLevel = level;
   lod.mView = view;
   lod.mViewInverted = lod.mView.Invert();
   lod.mFrustum = lod.mViewInverted * projection;

   // Nest-iterate all children of the layer owner                      
   Count renderedEntities {};
   for (auto owner : GetOwners()) {
      if (owner->IsClassIrrelevant())
         continue;

      renderedEntities += CompileThing(owner, lod, pipesPerCamera);
   }
   
   if (renderedEntities) {
      for (auto pipeline : pipesPerCamera) {
         // Push PerLevel uniforms if required                          
         const auto projectedView = lod.mViewInverted * projection;
         const auto projectedViewInverted = projectedView.Invert();

         pipeline->SetUniform<RRate::Level, Traits::ViewTransform>(lod.mView);
         pipeline->SetUniform<RRate::Level, Traits::ViewTransformInverted>(lod.mViewInverted);
         pipeline->SetUniform<RRate::Level, Traits::ViewProjectTransform>(projectedView);
         pipeline->SetUniform<RRate::Level, Traits::ViewProjectTransformInverted>(projectedViewInverted);
         pipeline->SetUniform<RRate::Level, Traits::Level>(level);
         pipeline->PushUniforms<RRate::Level, false>();
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
   const Matrix4& view, 
   const Matrix4& projection, 
   Level level, 
   PipelineSet& pipesPerCamera
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
      if (renderable.IsClassIrrelevant())
         continue;

      if (renderable.GetInstances().IsEmpty()) {
         auto pipeline = CompileInstance(&renderable, nullptr, lod);
         if (pipeline) {
            pipeline->PushUniforms<RRate::Instance>();
            pipeline->PushUniforms<RRate::Renderable>();
            pipesPerCamera << pipeline;
            mRelevantPipelines << pipeline;
            ++renderedInstances;
         }
      }
      else for (auto instance : renderable.GetInstances()) {
         auto pipeline = CompileInstance(&renderable, instance, lod);
         if (pipeline) {
            pipeline->PushUniforms<RRate::Instance>();
            pipeline->PushUniforms<RRate::Renderable>();
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

         pipeline->SetUniform<RRate::Level, Traits::ViewTransform>(lod.mView);
         pipeline->SetUniform<RRate::Level, Traits::ViewTransformInverted>(lod.mViewInverted);
         pipeline->SetUniform<RRate::Level, Traits::ViewProjectTransform>(projectedView);
         pipeline->SetUniform<RRate::Level, Traits::ViewProjectTransformInverted>(projectedViewInverted);
         pipeline->SetUniform<RRate::Level, Traits::Level>(level);
         pipeline->PushUniforms<RRate::Level>();

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
            pipeline->SetUniform<RRate::Camera, Traits::ProjectTransform>(Matrix4 {});
            pipeline->SetUniform<RRate::Camera, Traits::ProjectTransformInverted>(Matrix4 {});
            pipeline->SetUniform<RRate::Camera, Traits::FOV>(Radians {});
            pipeline->SetUniform<RRate::Camera, Traits::Resolution>(GetWindow()->GetSize());
            if (mStyle & Style::Hierarchical)
               pipeline->PushUniforms<RRate::Camera, false>();
            else
               pipeline->PushUniforms<RRate::Camera>();
         }

         if (mStyle & Style::Hierarchical)
            mSubscriberCountPerCamera.New(1);
         ++renderedCameras;
      }
   }
   else for (const auto& camera : mCameras) {
      if (camera.IsClassIrrelevant())
         continue;

      // Iterate all levels per camera                                  
      PipelineSet pipesPerCamera;
      if (mStyle & Style::Multilevel) {
         // Multilevel style - tests all camera-visible levels          
         for (auto level = camera.GetObservableRange().mMax; level >= camera.GetObservableRange().mMin; --level) {
            const auto view = camera.GetViewTransform(level);
            if (mStyle & Style::Hierarchical)
               CompileLevelHierarchical(view, camera.GetProjection(), level, pipesPerCamera);
            else
               CompileLevelBatched(view, camera.GetProjection(), level, pipesPerCamera);
         }
      }
      else if (camera.GetObservableRange().Inside(Level::Default)) {
         // Default level style - checks only if camera sees default    
         const auto view = camera.GetViewTransform(Level::Default);
         if (mStyle & Style::Hierarchical)
            CompileLevelHierarchical(view, camera.GetProjection(), {}, pipesPerCamera);
         else
            CompileLevelBatched(view, camera.GetProjection(), {}, pipesPerCamera);
      }
      else continue;

      if (!pipesPerCamera.IsEmpty()) {
         for (auto pipeline : pipesPerCamera) {
            // Push PerCamera uniforms if required                      
            pipeline->SetUniform<RRate::Camera, Traits::ProjectTransform>(camera.mProjection);
            pipeline->SetUniform<RRate::Camera, Traits::ProjectTransformInverted>(camera.mProjectionInverted);
            pipeline->SetUniform<RRate::Camera, Traits::FOV>(camera.mFOV);
            pipeline->SetUniform<RRate::Camera, Traits::Resolution>(camera.mResolution);
            if (mStyle & Style::Hierarchical)
               pipeline->PushUniforms<RRate::Camera, false>();
            else
               pipeline->PushUniforms<RRate::Camera>();
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
///   @param cb - command buffer to render to                                 
///   @param pass - render pass handle                                        
///   @param fb - framebuffer to render to                                    
void VulkanLayer::Render(VkCommandBuffer cb, const VkRenderPass& pass, const VkFramebuffer& fb) const {
   if (mStyle & Style::Hierarchical)
      RenderHierarchical(cb, pass, fb);
   else
      RenderBatched(cb, pass, fb);
}

/// Render the layer to a specific command buffer and framebuffer             
/// This is used only for Batched style layers, and relies on previously      
/// compiled set of pipelines                                                 
///   @param cb - command buffer to render to                                 
///   @param pass - render pass handle                                        
///   @param fb - framebuffer to render to                                    
void VulkanLayer::RenderBatched(VkCommandBuffer cb, const VkRenderPass& pass, const VkFramebuffer& fb) const {
   VkClearValue clearValues[2];
   clearValues[0].color = {{0.0f, 0.0f, 0.0f, 1.0f}};
   clearValues[1].depthStencil = {1.0f, 0};

   VkClearAttachment depthsweep {};
   depthsweep.colorAttachment = VK_ATTACHMENT_UNUSED;
   depthsweep.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
   depthsweep.clearValue.depthStencil = clearValues[1].depthStencil;

   VkRenderPassBeginInfo renderPassInfo {};
   renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
   renderPassInfo.renderPass = pass;
   renderPassInfo.framebuffer = fb;
   renderPassInfo.renderArea.offset = {0, 0};
   renderPassInfo.renderArea.extent.width = uint32_t(GetProducer()->GetResolution()[0]);
   renderPassInfo.renderArea.extent.height = uint32_t(GetProducer()->GetResolution()[1]);
   renderPassInfo.clearValueCount = 2;
   renderPassInfo.pClearValues = static_cast<const VkClearValue*>(clearValues);

   // Iterate all valid cameras                                         
   TUnorderedMap<const VulkanPipeline*, Count> done;

   if (mRelevantCameras.IsEmpty()) {
      VkViewport viewport {};
      viewport.width = GetProducer()->GetResolution()[0];
      viewport.height = GetProducer()->GetResolution()[1];

      VkRect2D scissor {};
      scissor.extent.width = uint32_t(viewport.width);
      scissor.extent.height = uint32_t(viewport.height);

      vkCmdBeginRenderPass(cb, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
      vkCmdSetViewport(cb, 0, 1, &viewport);
      vkCmdSetScissor(cb, 0, 1, &scissor);

      // Iterate all relevant levels                                    
      for (const auto& level : mRelevantLevels) {
         // Draw all subscribers to the pipeline for the current level  
         for (auto pipeline : mRelevantPipelines) {
            done[pipeline] = pipeline->RenderLevel(done[pipeline]);
         }

         if (level != mRelevantLevels.Last()) {
            // Clear depth after rendering this level (if not last)     
            const VkClearRect rect {scissor, 0, 1};
            vkCmdClearAttachments(cb, 1, &depthsweep, 1, &rect);
         }
      }

      // Main pass ends                                                 
      vkCmdEndRenderPass(cb);
   }
   else for (const auto camera : mRelevantCameras) {
      renderPassInfo.renderArea.extent.width = camera->mResolution[0];
      renderPassInfo.renderArea.extent.height = camera->mResolution[1];

      vkCmdBeginRenderPass(cb, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
      vkCmdSetViewport(cb, 0, 1, &camera->mVulkanViewport);
      vkCmdSetScissor(cb, 0, 1, &camera->mVulkanScissor);

      // Iterate all relevant levels                                    
      for (const auto& level : mRelevantLevels) {
         // Draw all subscribers to the pipeline for the current level  
         for (auto pipeline : mRelevantPipelines) {
            done[pipeline] = pipeline->RenderLevel(done[pipeline]);
         }

         if (level != mRelevantLevels.Last()) {
            // Clear depth after rendering this level (if not last)     
            const VkClearRect rect {camera->mVulkanScissor, 0, 1};
            vkCmdClearAttachments(cb, 1, &depthsweep, 1, &rect);
         }
      }

      // Main pass ends                                                 
      vkCmdEndRenderPass(cb);
   }
}

/// Render the layer to a specific command buffer and framebuffer             
/// This is used only for Hierarchical style layers, and relies on locally    
/// compiled subscribers, rendering them in their respective order            
///   @param cb - command buffer to render to                                 
///   @param pass - render pass handle                                        
///   @param fb - framebuffer to render to                                    
void VulkanLayer::RenderHierarchical(VkCommandBuffer cb, const VkRenderPass& pass, const VkFramebuffer& fb) const {
   VkClearValue clearValues[2];
   clearValues[0].color = {{0.0f, 0.0f, 0.0f, 1.0f}};
   clearValues[1].depthStencil = {1.0f, 0};

   VkClearAttachment depthsweep = {};
   depthsweep.colorAttachment = VK_ATTACHMENT_UNUSED;
   depthsweep.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
   depthsweep.clearValue.depthStencil = clearValues[1].depthStencil;

   VkRenderPassBeginInfo renderPassInfo = {};
   renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
   renderPassInfo.renderPass = pass;
   renderPassInfo.framebuffer = fb;
   renderPassInfo.renderArea.offset = {0, 0};
   renderPassInfo.renderArea.extent.width = uint32_t(GetProducer()->GetResolution()[0]);
   renderPassInfo.renderArea.extent.height = uint32_t(GetProducer()->GetResolution()[1]);
   renderPassInfo.clearValueCount = 2;
   renderPassInfo.pClearValues = static_cast<const VkClearValue*>(clearValues);

   Count subscribersDone {};
   auto subscriberCountPerLevel = &mSubscriberCountPerLevel[0];

   // Iterate all valid cameras                                         
   if (mRelevantCameras.IsEmpty()) {
      VkViewport viewport {};
      viewport.width = GetProducer()->GetResolution()[0];
      viewport.height = GetProducer()->GetResolution()[1];

      VkRect2D scissor {};
      scissor.extent.width = uint32_t(viewport.width);
      scissor.extent.height = uint32_t(viewport.height);

      vkCmdBeginRenderPass(cb, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
      vkCmdSetViewport(cb, 0, 1, &viewport);
      vkCmdSetScissor(cb, 0, 1, &scissor);

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
            vkCmdClearAttachments(cb, 1, &depthsweep, 1, &rect);
         }

         subscribersDone += *subscriberCountPerLevel;
         ++subscriberCountPerLevel;
      }

      // Main pass ends                                                 
      vkCmdEndRenderPass(cb);
   }
   else for (const auto camera : mRelevantCameras) {
      renderPassInfo.renderArea.extent.width = camera->mResolution[0];
      renderPassInfo.renderArea.extent.height = camera->mResolution[1];

      vkCmdBeginRenderPass(cb, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
      vkCmdSetViewport(cb, 0, 1, &camera->mVulkanViewport);
      vkCmdSetScissor(cb, 0, 1, &camera->mVulkanScissor);

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
            vkCmdClearAttachments(cb, 1, &depthsweep, 1, &rect);
         }

         subscribersDone += *subscriberCountPerLevel;
         ++subscriberCountPerLevel;
      }

      // Main pass ends                                                 
      vkCmdEndRenderPass(cb);
   }
}
