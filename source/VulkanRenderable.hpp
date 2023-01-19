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
class VulkanRenderable : public A::GraphicsUnit {
   LANGULUS(ABSTRACT) false;
   LANGULUS(PRODUCER) VulkanLayer;
   LANGULUS_BASES(A::GraphicsUnit);
private:
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
   VulkanRenderable(const Any&);

   NOD() auto GetRenderer() noexcept;
   NOD() VulkanGeometry* GetGeometry(const LodState&);
   NOD() VulkanTexture*  GetTextures(const LodState&);
   NOD() VulkanPipeline* GetOrCreatePipeline(const LodState&, const VulkanLayer*);

   void Refresh() override;
};
