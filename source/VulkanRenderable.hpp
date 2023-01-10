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
class CVulkanRenderable : public ARenderable, public TProducedFrom<CVulkanLayer> {
   // Precompiled instances and levels, updated on Refresh()            
   TAny<const AInstance*> mInstances;
   TRange<Level> mLevelRange;
   Ref<AMaterial> mMaterialContent;
   Ref<AGeometry> mGeometryContent;
   Ref<ATexture> mTextureContent;

   Ref<CVulkanPipeline> mPredefinedPipeline;

   // Precompiled content, updated on Refresh()                         
   struct {
      Ref<CVulkanGeometry> mGeometry;
      Ref<CVulkanTexture> mTexture;
      Ref<CVulkanPipeline> mPipeline;
   } mLOD[LodState::IndexCount];

   void ResetRenderable();

public:
   REFLECT(CVulkanRenderable);
   CVulkanRenderable(CVulkanLayer*);
   CVulkanRenderable(CVulkanRenderable&&) noexcept = default;
   CVulkanRenderable& operator = (CVulkanRenderable&&) noexcept = default;
   ~CVulkanRenderable();

   NOD() auto GetRenderer() noexcept;
   NOD() CVulkanGeometry* GetGeometry(const LodState&);
   NOD() CVulkanTexture* GetTextures(const LodState&);
   NOD() CVulkanPipeline* GetOrCreatePipeline(const LodState&, const CVulkanLayer*);

   void Refresh() override;

   PC_GET(Instances);
   PC_GET(LevelRange);
};
