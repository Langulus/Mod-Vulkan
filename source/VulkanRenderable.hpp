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
///   Vulkan renderable element                                               
///                                                                           
/// Gives things the ability to be drawn to screen. The unit gathers relevant 
/// graphical resources from the context, and generates a graphical pipeline  
/// capable of visualizing it on Refresh()                                    
///                                                                           
struct VulkanRenderable final : A::GraphicsUnit, ProducedFrom<VulkanLayer> {
   LANGULUS(ABSTRACT) false;
   LANGULUS_BASES(A::GraphicsUnit);

protected:
   friend struct VulkanLayer;

   // Precompiled instances and levels, updated on Refresh()            
   TAny<const Unit*> mInstances;
   TRange<Level> mLevelRange;
   Ptr<Unit> mMaterialContent;
   Ptr<Unit> mGeometryContent;
   Ptr<Unit> mTextureContent;
   Ptr<VulkanPipeline> mPredefinedPipeline;

   // Precompiled content, updated on Refresh()                         
   struct {
      Ptr<VulkanGeometry> mGeometry;
      Ptr<VulkanTexture> mTexture;
      Ptr<VulkanPipeline> mPipeline;
   } mLOD[LodState::IndexCount];

   void ResetRenderable();

public:
   VulkanRenderable(VulkanLayer*, const Any&);

   NOD() VulkanRenderer* GetRenderer() const noexcept;
   NOD() VulkanGeometry* GetGeometry(const LodState&) const;
   NOD() VulkanTexture*  GetTextures(const LodState&) const;
   NOD() VulkanPipeline* GetOrCreatePipeline(const LodState&, const VulkanLayer*) const;

   void Refresh();
};
