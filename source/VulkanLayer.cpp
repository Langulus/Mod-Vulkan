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
CVulkanLayer::CVulkanLayer(CVulkanRenderer* producer)
   : AVisualLayer {MetaData::Of<CVulkanLayer>()}
   , TProducedFrom {producer}
   , mCameras {this}
   , mRenderables {this}
   , mLights {this} {
   ClassValidate();
}

/// Create/destroy renderables, cameras, lights                               
///   @param verb - creation verb                                             
void CVulkanLayer::Create(Verb& verb) {
   mCameras.Create(verb);
   mRenderables.Create(verb);
   mLights.Create(verb);
}

/// Generate the draw list for the layer                                      
///   @param relevantPipelines - [out] a set of all used pipelines            
///   @return true if something needs to be rendered                          
bool CVulkanLayer::Generate(PipelineSet& relevantPipelines) {
   CompileCameras();
   auto n = CompileLevels();
   for (auto p : mRelevantPipelines)
      relevantPipelines.insert(p);
   return n > 0;
}

/// Get the window of the renderer                                            
///   @return the window pointers                                             
const AWindow* CVulkanLayer::GetWindow() const {
   return GetProducer()->GetWindow();
}

/// Compile the camera transformations                                        
void CVulkanLayer::CompileCameras() {
   for (auto& camera : mCameras) {
      if (camera.IsClassIrrelevant())
         continue;
      camera.Compile();
   }
}

/// Compile a single instance, culling it if able                             
///   @param renderable - the renderable to compile                           
///   @param instance - the instance to compile                               
///   @param lod - the lod state to use                                       
///   @return the pipeline if instance is relevant                            
CVulkanPipeline* CVulkanLayer::CompileInstance(CVulkanRenderable* renderable, const AInstance* instance, LodState& lod) {
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
      pipeline->SetUniform<RRate::PerRenderable, Traits::Model>(geometry);

   // Get relevant textures                                             
   const auto textures = renderable->GetTextures(lod);
   if (textures)
      pipeline->SetUniform<RRate::PerRenderable, Traits::Texture>(textures);

   // Push uniforms                                                     
   pipeline->SetUniform<RRate::PerInstance, Traits::ModelTransform>(lod.mModel);
   return pipeline;
}

/// Compile an entity and all of its children entities                        
/// Used only for hierarchical styled layers                                  
///   @param entity - entity to compile                                       
///   @param lod - the lod state to use                                       
///   @param pipesPerCamera - [out] pipelines used by the hierarchy           
///   @return 1 if something from the hierarchy was rendered                  
pcptr CVulkanLayer::CompileEntity(const Entity* entity, LodState& lod, PipelineSet& pipesPerCamera) {
   // Iterate all renderables of the entity, which are part of this     
   // layer - disregard all others                                      
   std::vector<CVulkanRenderable*> relevantRenderables;
   for (auto unit : entity->GetUnits()) {
      auto renderable = dynamic_cast<CVulkanRenderable*>(unit);
      if (renderable && !renderable->IsClassIrrelevant() && mRenderables.Owns(renderable))
         relevantRenderables.push_back(renderable);
   }

   // Compile the instances associated with these renderables           
   pcptr renderedInstances = 0;
   for (auto renderable : relevantRenderables) {
      if (renderable->GetInstances().IsEmpty()) {
         // Imagine a default instance                                  
         auto pipeline = CompileInstance(renderable, nullptr, lod);
         if (pipeline) {
            const auto sub = pipeline->PushUniforms<RRate::PerInstance, false>();
            pipeline->PushUniforms<RRate::PerRenderable, false>();
            pipesPerCamera.insert(pipeline);
            mRelevantPipelines.insert(pipeline);
            auto& s = mSubscribers.back();
            s.pipeline = pipeline;
            s.sub = sub;
            mSubscribers.emplace_back();
            ++mSubscriberCountPerLevel.back();
            ++mSubscriberCountPerCamera.back();
            ++renderedInstances;
         }
      }
      else for (auto instance : renderable->GetInstances()) {
         // Compile each available instance                             
         if (instance->IsClassIrrelevant())
            continue;

         auto pipeline = CompileInstance(renderable, instance, lod);
         if (pipeline) {
            const auto sub = pipeline->PushUniforms<RRate::PerInstance, false>();
            pipeline->PushUniforms<RRate::PerRenderable, false>();
            pipesPerCamera.insert(pipeline);
            mRelevantPipelines.insert(pipeline);
            auto& s = mSubscribers.back();
            s.pipeline = pipeline;
            s.sub = sub;
            mSubscribers.emplace_back();
            ++mSubscriberCountPerLevel.back();
            ++mSubscriberCountPerCamera.back();
            ++renderedInstances;
         }
      }
   }

   // Nest to children                                                  
   for (auto& child : entity->GetChildren()) {
      if (child->IsClassIrrelevant())
         continue;
      renderedInstances += CompileEntity(child, lod, pipesPerCamera);
   }

   return renderedInstances > 0;
}

/// Compile a single level's instances hierarchical style                     
///   @param view - the camera view matrix                                    
///   @param projection - the camera projection matrix                        
///   @param level - the level to compile                                     
///   @param pipesPerCamera - [out] pipeline set for the current level only   
///   @return the number of compiled entities                                 
pcptr CVulkanLayer::CompileLevelHierarchical(const mat4& view, const mat4& projection, Level level, PipelineSet& pipesPerCamera) {
   // Construct view and frustum   for culling                          
   LodState lod;
   lod.mLevel = level;
   lod.mView = view;
   lod.mViewInverted = lod.mView.Invert();
   lod.mFrustum = lod.mViewInverted * projection;

   // Nest-iterate all children of the layer owner                      
   pcptr renderedEntities = 0;
   for (auto owner : GetOwners()) {
      if (owner->IsClassIrrelevant())
         continue;
      renderedEntities += CompileEntity(owner, lod, pipesPerCamera);
   }
   
   if (renderedEntities) {
      for (auto pipeline : pipesPerCamera) {
         // Push PerLevel uniforms if required                          
         const auto projectedView = lod.mViewInverted * projection;
         const auto projectedViewInverted = projectedView.Invert();

         pipeline->SetUniform<RRate::PerLevel, Traits::ViewTransform>(lod.mView);
         pipeline->SetUniform<RRate::PerLevel, Traits::ViewTransformInverted>(lod.mViewInverted);
         pipeline->SetUniform<RRate::PerLevel, Traits::ViewProjectTransform>(projectedView);
         pipeline->SetUniform<RRate::PerLevel, Traits::ViewProjectTransformInverted>(projectedViewInverted);
         pipeline->SetUniform<RRate::PerLevel, Traits::Level>(level);
         pipeline->PushUniforms<RRate::PerLevel, false>();
      }

      mSubscriberCountPerLevel.emplace_back();

      // Store the negative level in the set, so they're always in      
      // a descending order                                             
      mRelevantLevels.insert(-level);
   }

   return renderedEntities;
}

/// Compile a single level's instances batched style                          
///   @param view - the camera view matrix                                    
///   @param projection - the camera projection matrix                        
///   @param level - the level to compile                                     
///   @param pipesPerCamera - [out] pipeline set for the current level only   
///   @return 1 if anything was rendered, zero otherwise                      
pcptr CVulkanLayer::CompileLevelBatched(const mat4& view, const mat4& projection, Level level, PipelineSet& pipesPerCamera) {
   // Construct view and frustum   for culling                          
   LodState lod;
   lod.mLevel = level;
   lod.mView = view;
   lod.mViewInverted = lod.mView.Invert();
   lod.mFrustum = lod.mViewInverted * projection;

   // Iterate all renderables                                           
   pcptr renderedInstances = 0;
   for (auto& renderable : mRenderables) {
      if (renderable.IsClassIrrelevant())
         continue;

      if (renderable.GetInstances().IsEmpty()) {
         auto pipeline = CompileInstance(&renderable, nullptr, lod);
         if (pipeline) {
            pipeline->PushUniforms<RRate::PerInstance>();
            pipeline->PushUniforms<RRate::PerRenderable>();
            pipesPerCamera.insert(pipeline);
            mRelevantPipelines.insert(pipeline);
            ++renderedInstances;
         }
      }
      else for (auto instance : renderable.GetInstances()) {
         auto pipeline = CompileInstance(&renderable, instance, lod);
         if (pipeline) {
            pipeline->PushUniforms<RRate::PerInstance>();
            pipeline->PushUniforms<RRate::PerRenderable>();
            pipesPerCamera.insert(pipeline);
            mRelevantPipelines.insert(pipeline);
            ++renderedInstances;
         }
      }
   }

   if (renderedInstances) {
      for (auto pipeline : pipesPerCamera) {
         // Push PerLevel uniforms if required                          
         const auto projectedView = lod.mViewInverted * projection;
         const auto projectedViewInverted = projectedView.Invert();

         pipeline->SetUniform<RRate::PerLevel, Traits::ViewTransform>(lod.mView);
         pipeline->SetUniform<RRate::PerLevel, Traits::ViewTransformInverted>(lod.mViewInverted);
         pipeline->SetUniform<RRate::PerLevel, Traits::ViewProjectTransform>(projectedView);
         pipeline->SetUniform<RRate::PerLevel, Traits::ViewProjectTransformInverted>(projectedViewInverted);
         pipeline->SetUniform<RRate::PerLevel, Traits::Level>(level);
         pipeline->PushUniforms<RRate::PerLevel>();

         // Store the negative level in the set, so they're always in   
         // a descending order                                          
         mRelevantLevels.insert(-level);
      }

      return 1;
   }

   return 0;
}

/// Compile all levels and their instances                                    
///   @return the number of relevant cameras                                  
pcptr CVulkanLayer::CompileLevels() {
   pcptr renderedCameras = 0;
   mRelevantLevels.clear();
   mRelevantPipelines.clear();

   if (mStyle & Style::Hierarchical) {
      mSubscribers.clear();
      mSubscribers.emplace_back();
      mSubscriberCountPerLevel.clear();
      mSubscriberCountPerLevel.emplace_back();
      mSubscriberCountPerCamera.clear();
      mSubscriberCountPerCamera.emplace_back();
   }

   if (mCameras.IsEmpty()) {
      // No camera, so just render default level on the whole screen    
      PipelineSet pipesPerCamera;
      if (mStyle & Style::Hierarchical)
         CompileLevelHierarchical(mat4::Identity(), mat4::Identity(), Level::Default, pipesPerCamera);
      else
         CompileLevelBatched(mat4::Identity(), mat4::Identity(), Level::Default, pipesPerCamera);

      if (!pipesPerCamera.empty()) {
         for (auto pipeline : pipesPerCamera) {
            // Push PerCamera uniforms if required                      
            pipeline->SetUniform<RRate::PerCamera, Traits::ProjectTransform>(mat4::Identity());
            pipeline->SetUniform<RRate::PerCamera, Traits::ProjectTransformInverted>(mat4::Identity());
            pipeline->SetUniform<RRate::PerCamera, Traits::FOV>(real(0));
            pipeline->SetUniform<RRate::PerCamera, Traits::Resolution>(GetWindow()->GetSize());
            if (mStyle & Style::Hierarchical)
               pipeline->PushUniforms<RRate::PerCamera, false>();
            else
               pipeline->PushUniforms<RRate::PerCamera>();
         }

         if (mStyle & Style::Hierarchical)
            mSubscriberCountPerCamera.emplace_back();
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
            CompileLevelHierarchical(view, camera.GetProjection(), Level::Default, pipesPerCamera);
         else
            CompileLevelBatched(view, camera.GetProjection(), Level::Default, pipesPerCamera);
      }
      else continue;

      if (!pipesPerCamera.empty()) {
         for (auto pipeline : pipesPerCamera) {
            // Push PerCamera uniforms if required                      
            pipeline->SetUniform<RRate::PerCamera, Traits::ProjectTransform>(camera.GetProjection());
            pipeline->SetUniform<RRate::PerCamera, Traits::ProjectTransformInverted>(camera.GetProjectionInverted());
            pipeline->SetUniform<RRate::PerCamera, Traits::FOV>(camera.GetFOV());
            pipeline->SetUniform<RRate::PerCamera, Traits::Resolution>(camera.GetResolution());
            if (mStyle & Style::Hierarchical)
               pipeline->PushUniforms<RRate::PerCamera, false>();
            else
               pipeline->PushUniforms<RRate::PerCamera>();
         }

         if (mStyle & Style::Hierarchical)
            mSubscriberCountPerCamera.emplace_back();
         mRelevantCameras.insert(&camera);
         ++renderedCameras;
      }
   }

   return renderedCameras;
}

/// Render the layer to a specific command buffer and framebuffer             
///   @param cb - command buffer to render to                                 
///   @param pass - render pass handle                                        
///   @param fb - framebuffer to render to                                    
void CVulkanLayer::Render(VkCommandBuffer cb, const VkRenderPass& pass, const VkFramebuffer& fb) const {
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
void CVulkanLayer::RenderBatched(VkCommandBuffer cb, const VkRenderPass& pass, const VkFramebuffer& fb) const {
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

   // Iterate all valid cameras                                         
   std::unordered_map<const CVulkanPipeline*, pcptr> done;

   if (mRelevantCameras.empty()) {
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

         if (level != *mRelevantLevels.rbegin()) {
            // Clear depth after rendering this level (if not last)     
            const VkClearRect rect {scissor, 0, 1};
            vkCmdClearAttachments(cb, 1, &depthsweep, 1, &rect);
         }
      }

      // Main pass ends                                                 
      vkCmdEndRenderPass(cb);
   }
   else for (const auto camera : mRelevantCameras) {
      renderPassInfo.renderArea.extent.width = uint32_t(camera->GetResolution()[0]);
      renderPassInfo.renderArea.extent.height = uint32_t(camera->GetResolution()[1]);

      vkCmdBeginRenderPass(cb, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
      vkCmdSetViewport(cb, 0, 1, &camera->GetVulkanViewport());
      vkCmdSetScissor(cb, 0, 1, &camera->GetVulkanScissor());

      // Iterate all relevant levels                                    
      for (const auto& level : mRelevantLevels) {
         // Draw all subscribers to the pipeline for the current level  
         for (auto pipeline : mRelevantPipelines) {
            done[pipeline] = pipeline->RenderLevel(done[pipeline]);
         }

         if (level != *mRelevantLevels.rbegin()) {
            // Clear depth after rendering this level (if not last)     
            const VkClearRect rect {camera->GetVulkanScissor(), 0, 1};
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
void CVulkanLayer::RenderHierarchical(VkCommandBuffer cb, const VkRenderPass& pass, const VkFramebuffer& fb) const {
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

   pcptr subscribersDone = 0;
   auto subscriberCountPerLevel = &mSubscriberCountPerLevel[0];

   // Iterate all valid cameras                                         
   if (mRelevantCameras.empty()) {
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
         for (pcptr s = 0; s < *subscriberCountPerLevel; ++s) {
            auto& subscriber = mSubscribers[subscribersDone + s];
            subscriber.pipeline->RenderSubscriber(subscriber.sub);
         }

         if (level != *mRelevantLevels.rbegin()) {
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
      renderPassInfo.renderArea.extent.width = uint32_t(camera->GetResolution()[0]);
      renderPassInfo.renderArea.extent.height = uint32_t(camera->GetResolution()[1]);

      vkCmdBeginRenderPass(cb, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
      vkCmdSetViewport(cb, 0, 1, &camera->GetVulkanViewport());
      vkCmdSetScissor(cb, 0, 1, &camera->GetVulkanScissor());

      // Iterate all relevant levels                                    
      for (const auto& level : mRelevantLevels) {
         // Draw all subscribers to the pipeline for the current level  
         // and camera                                                  
         for (pcptr s = 0; s < *subscriberCountPerLevel; ++s) {
            auto& subscriber = mSubscribers[subscribersDone + s];
            subscriber.pipeline->RenderSubscriber(subscriber.sub);
         }

         if (level != *mRelevantLevels.rbegin()) {
            // Clear depth after rendering this level (if not last)     
            const VkClearRect rect {camera->GetVulkanScissor(), 0, 1};
            vkCmdClearAttachments(cb, 1, &depthsweep, 1, &rect);
         }

         subscribersDone += *subscriberCountPerLevel;
         ++subscriberCountPerLevel;
      }

      // Main pass ends                                                 
      vkCmdEndRenderPass(cb);
   }
}
