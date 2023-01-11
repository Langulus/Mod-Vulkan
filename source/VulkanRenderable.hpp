///                                                                           
/// Langulus::Module::Vulkan                                                  
/// Copyright(C) 2020 Dimo Markov <langulusteam@gmail.com>                    
///                                                                           
/// Distributed under GNU General Public License v3+                          
/// See LICENSE file, or https://www.gnu.org/licenses                         
///                                                                           
#pragma once
#include "Common.hpp"


///                                                                           
///   THE VULKAN RENDERABLE                                                   
///                                                                           
/// Capsulates an object that can be rendered via a renderer                  
/// This unit is an interface that gathers relevant graphical data about its  
/// owners, and generates pipelines on Refresh(), that are able to visualize  
/// it                                                                        
///                                                                           
class VulkanRenderable : public Unit {
   LANGULUS(PRODUCER) VulkanLayer;

private:
   // Precompiled instances and levels, updated on Refresh()            
   TAny<const AInstance*> mInstances;
   TRange<Level> mLevelRange;
   Ref<AMaterial> mMaterialContent;
   Ref<AGeometry> mGeometryContent;
   Ref<ATexture> mTextureContent;

   Ref<VulkanPipeline> mPredefinedPipeline;

   // Precompiled content, updated on Refresh()                         
   struct {
      Ref<VulkanGeometry> mGeometry;
      Ref<VulkanTexture> mTexture;
      Ref<VulkanPipeline> mPipeline;
   } mLOD[LodState::IndexCount];

   void ResetRenderable();

public:
   VulkanRenderable(VulkanLayer*);
   VulkanRenderable(VulkanRenderable&&) noexcept = default;
   ~VulkanRenderable();

   VulkanRenderable& operator = (VulkanRenderable&&) noexcept = default;

   NOD() auto GetRenderer() noexcept;
   NOD() VulkanGeometry* GetGeometry(const LodState&);
   NOD() VulkanTexture*  GetTextures(const LodState&);
   NOD() VulkanPipeline* GetOrCreatePipeline(const LodState&, const VulkanLayer*);

   void Refresh() override;

   PC_GET(Instances);
   PC_GET(LevelRange);
};
